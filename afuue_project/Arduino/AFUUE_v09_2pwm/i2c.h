#pragma once

extern bool I2C_ReadBytes(uint8_t addr, uint8_t reg, uint8_t* values, uint8_t length);
extern bool I2C_WriteBytes(uint8_t addr, uint8_t reg, uint8_t *values, uint8_t length);
