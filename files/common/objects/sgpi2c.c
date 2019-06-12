// This is an adaption of:
//   https://github.com/Sensirion/arduino-ess/blob/master/sensirion_ess.cpp
//
// Based on IR Thermoclick example
//
// Author: Johannes Winkelmann, jw@smts.ch
//

#include "sgpi2c.h"

#define SGP_ADDR (0x58)
#define SGP_CMD_LENGTH (2)
#define SGP30_DATA_LENGTH (6)

#define SGP_UPDATE_THRESHOLD_MS 800

#define SGP_MEASUREMENT_DELAY 50

static uint64_t sLastSGPUpdateMs = 0;

static uint16_t sCo2eqCache = 0;
static uint16_t sTVocCache = 0;

typedef struct {
    ATMO_SGPI2C_Config_t config;
    bool configured;
} ATMO_SGPI2C_Priv_Config;

static ATMO_SGPI2C_Priv_Config _ATMO_SGPI2C_config;

static ATMO_I2C_Peripheral_t _ATMO_SGPI2C_i2cConfig = {
    .operatingMode = ATMO_I2C_OperatingMode_Master,
    .baudRate = ATMO_I2C_BaudRate_Standard_Mode
};

ATMO_SGPI2C_Status_t ATMO_SGPI2C_InitSGP_Internal();
ATMO_SGPI2C_Status_t ATMO_SGPI2C_updateSGP_Internal();


ATMO_SGPI2C_Status_t ATMO_SGPI2C_Init(ATMO_SGPI2C_Config_t *config)
{
    if (config) { // Did the user supply a configuration?
        ATMO_SGPI2C_SetConfiguration(config);
    } else {
        _ATMO_SGPI2C_config.configured = false;
    }

    if (ATMO_SGPI2C_InitSGP_Internal() != ATMO_SGPI2C_Status_Success) {
      return ATMO_SGPI2C_Status_Fail;
    }

    // initial calls to update functions to set lastUpdate variables
    ATMO_SGPI2C_updateSGP_Internal();

    return ATMO_SGPI2C_Status_Success;
}

ATMO_SGPI2C_Status_t ATMO_SGPI2C_SetConfiguration(const ATMO_SGPI2C_Config_t *config)
{
    if (config == NULL) {
        return ATMO_SGPI2C_Status_Fail;
    }

    if (ATMO_I2C_SetConfiguration(config->i2cDriverInstance, &_ATMO_SGPI2C_i2cConfig) != ATMO_I2C_Status_Success) {
        return ATMO_SGPI2C_Status_Fail;
    }
    memcpy( &_ATMO_SGPI2C_config.config, config, sizeof(ATMO_SGPI2C_Config_t) );
    _ATMO_SGPI2C_config.configured = true;

    return ATMO_SGPI2C_Status_Success;
}

ATMO_SGPI2C_Status_t ATMO_SGPI2C_GetConfiguration(ATMO_SGPI2C_Config_t *config)
{
    if (config == NULL || !_ATMO_SGPI2C_config.configured) {
        return ATMO_SGPI2C_Status_Fail;
    }

    memcpy(config, &_ATMO_SGPI2C_config.config, sizeof(ATMO_SGPI2C_Config_t));
    return ATMO_SGPI2C_Status_Success;
}

ATMO_SGPI2C_Status_t ATMO_SGPI2C_ReadData_Internal(uint8_t addr, uint8_t* cmd, uint16_t cmdLength,
          uint8_t* data, uint16_t dataLength, uint16_t measurementDelay)
{
  ATMO_I2C_Status_t readStatus;
  readStatus = ATMO_I2C_MasterWrite(_ATMO_SGPI2C_config.config.i2cDriverInstance,
                                    addr, cmd, cmdLength,
                                    NULL, 0,
                                    0);
  if (readStatus != ATMO_I2C_Status_Success) {
      return ATMO_SGPI2C_Status_Fail;
  }

  ATMO_DelayMillisecondsNonBlock(measurementDelay);

  readStatus = ATMO_I2C_MasterRead(_ATMO_SGPI2C_config.config.i2cDriverInstance,
                                  addr, NULL, 0, data, dataLength, 0);
  if (readStatus != ATMO_I2C_Status_Success) {
      return ATMO_SGPI2C_Status_Fail;
  }

  return ATMO_SGPI2C_Status_Success;
}

uint8_t ATMO_SGPI2C_CheckCrc_Internal(const uint8_t* data, uint8_t len)
{
    // adapted from SHT21 sample code from http://www.sensirion.com/en/products/humidity-temperature/download-center/

    uint8_t crc = 0xff;
    uint8_t byteCtr;
    for (byteCtr = 0; byteCtr < len; ++byteCtr) {
        crc ^= (data[byteCtr]);
        for (uint8_t bit = 8; bit > 0; --bit) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

// - SGP
ATMO_SGPI2C_Status_t ATMO_SGPI2C_InitSGP_Internal()
{
  uint8_t cmd[SGP_CMD_LENGTH] = { 0x20, 0x03 }; // init iaq
  ATMO_I2C_Status_t readStatus;
  readStatus = ATMO_I2C_MasterWrite(_ATMO_SGPI2C_config.config.i2cDriverInstance,
                                    SGP_ADDR, cmd, SGP_CMD_LENGTH,
                                    NULL, 0,
                                    0);
  if (readStatus != ATMO_I2C_Status_Success) {
      return ATMO_SGPI2C_Status_Fail;
  }

  return ATMO_SGPI2C_Status_Success;
}


ATMO_SGPI2C_Status_t ATMO_SGPI2C_updateSGP_Internal()
{
  uint64_t now = ATMO_PLATFORM_UptimeMs();
  if ((now - sLastSGPUpdateMs) < SGP_UPDATE_THRESHOLD_MS) {
    // reuse cached values
    return ATMO_SGPI2C_Status_Success;
  }
  sLastSGPUpdateMs = now;

  if (!_ATMO_SGPI2C_config.configured) {
      return ATMO_SGPI2C_Status_Fail;
  }

  uint8_t data[SGP30_DATA_LENGTH] = { 0 };
  uint8_t cmd[SGP_CMD_LENGTH] = { 0x20, 0x08 };
  if (ATMO_SGPI2C_ReadData_Internal(SGP_ADDR, cmd,
                                    SGP_CMD_LENGTH, data,
                                    SGP30_DATA_LENGTH,
                                    SGP_MEASUREMENT_DELAY) != ATMO_SGPI2C_Status_Success) {
    return ATMO_SGPI2C_Status_Fail;
  }

  if (ATMO_SGPI2C_CheckCrc_Internal(data+0, 2) != data[2] ||
      ATMO_SGPI2C_CheckCrc_Internal(data+3, 2) != data[5]) {
    return ATMO_SGPI2C_Status_Fail;
  }
  sTVocCache = (uint16_t)(data[3] << 8) | data[4];
  sCo2eqCache = (uint16_t)(data[0] << 8) | data[1];

  return ATMO_SGPI2C_Status_Success;
}

ATMO_SGPI2C_Status_t ATMO_SGPI2C_GetTVoc(uint16_t *tVoc)
{
  if (ATMO_SGPI2C_updateSGP_Internal() != ATMO_SGPI2C_Status_Success) {
    return ATMO_SGPI2C_Status_Fail;
  }
  *tVoc = sTVocCache;
  return ATMO_SGPI2C_Status_Success;
}

ATMO_SGPI2C_Status_t ATMO_SGPI2C_GetCo2eq(uint16_t *co2eq)
{
  if (ATMO_SGPI2C_updateSGP_Internal() != ATMO_SGPI2C_Status_Success) {
    return ATMO_SGPI2C_Status_Fail;
  }
  *co2eq = sCo2eqCache;
  return ATMO_SGPI2C_Status_Success;
}
