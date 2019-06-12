    uint16_t tVoc;
    ATMO_SGPI2C_GetTVoc(&tVoc);
    ATMO_CreateValueInt(out, tVoc);
    return ATMO_Status_Success;
