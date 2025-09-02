#pragma once

// sda = 0 or 1, odr (0x50=75Hz, 0x40=50Hz, 0x30=25Hz, 0x20=10Hz, 0x10=1Hz, 0x00=OFF)
extern int32_t lps33_init(uint8_t sda, uint8_t odr = 0x50);
extern bool lps33_readPressure(uint8_t sda, float* pPressure);
extern int32_t read_debugValue(int side);
