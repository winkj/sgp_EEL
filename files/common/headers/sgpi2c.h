#ifndef _ATMO_SGPI2C_H_
#define _ATMO_SGPI2C_H_

#include "../app_src/atmosphere_platform.h"
#include "../i2c/i2c.h"

typedef enum {
    ATMO_SGPI2C_Status_Success              = 0x00u,  // Common - Operation was successful
    ATMO_SGPI2C_Status_Fail                 = 0x01u,  // Common - Operation failed
    ATMO_SGPI2C_Status_Initialized          = 0x02u,  // Common - Peripheral already initialized
    ATMO_SGPI2C_Status_Invalid              = 0x03u,  // Common - Invalid operation or result
    ATMO_SGPI2C_Status_NotSupported         = 0x04u,  // Common - Feature not supported by platform
} ATMO_SGPI2C_Status_t;

typedef struct {
    ATMO_DriverInstanceHandle_t i2cDriverInstance;
} ATMO_SGPI2C_Config_t;

/**
 * Initialize SGPI2C Driver
 *
 * @param[in] config - Device configuration (optional)
 */
ATMO_SGPI2C_Status_t ATMO_SGPI2C_Init(ATMO_SGPI2C_Config_t *config);

/**
 * Set basic device configuration
 *
 * @param[in] config
 */
ATMO_SGPI2C_Status_t ATMO_SGPI2C_SetConfiguration(const ATMO_SGPI2C_Config_t *config);

/**
 * Get device configuration
 *
 * @param[out] config
 */
ATMO_SGPI2C_Status_t ATMO_SGPI2C_GetConfiguration(ATMO_SGPI2C_Config_t *config);

/**
 * total VOC compounds in ppb
 *
 * @param[out] total VOC
 */
ATMO_SGPI2C_Status_t ATMO_SGPI2C_GetTVoc(uint16_t *tVoc);

/**
 * CO2 equivalent in ppm
 *
 * @param[out] co2 equvalent
 */
ATMO_SGPI2C_Status_t ATMO_SGPI2C_GetCo2eq(uint16_t *co2eq);

#endif
