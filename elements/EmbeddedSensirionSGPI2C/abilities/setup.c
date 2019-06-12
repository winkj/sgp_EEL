	ATMO_SGPI2C_Config_t config;
	config.i2cDriverInstance = ATMO_PROPERTY(undefined, i2cInstance);

	return ( ATMO_SGPI2C_Init(&config) == ATMO_SGPI2C_Status_Success ) ? ATMO_Status_Success : ATMO_Status_Fail;
