#pragma once

extern char I2C_ReadBytes(uint8_t addr, uint8_t* values, uint8_t length);
extern char I2C_WriteBytes(unsigned char addr, unsigned char *values, char length);
