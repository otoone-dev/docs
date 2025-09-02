#include "afuue_common.h"
#include "lps33.h"
#include "i2c.h"

#define LPS33_ADDR (0x5C)

#define LPS33_WHOAMI_REG (0x0F)
#define LPS33_WHOAMI_RET (0xB1)
#define LPS33_REG_CTRL (0x10)
#define LPS33_REG_OUT_XL (0x28)

//------------------
int32_t lps33_init(uint8_t sda, uint8_t odr)
{
#if 0
  uint8_t reg = LPS33_WHOAMI_REG;
  uint8_t ret = 0;
  if (!I2C_WriteBytes(sda + LPS33_ADDR, &reg, 1)) {
    return 1;
  }
  if (!I2C_ReadBytes(sda + LPS33_ADDR, &ret, 1)) {
    return 2;
  }
  if (ret != LPS33_WHOAMI_RET) {
    return 3;
  }
#endif

  uint8_t ctrl_data[1] = { odr }; // BDU=0
  if (!I2C_WriteBytes(sda + LPS33_ADDR, LPS33_REG_CTRL, ctrl_data, 1)) {
    return 4;
  }
  return 0;
}

//------------------
static int32_t debgugValue[2] = { 0, 0};
bool lps33_readPressure(uint8_t sda, float* pPressure) {
  uint8_t data[3];
  if (!I2C_ReadBytes(sda + LPS33_ADDR, LPS33_REG_OUT_XL, data, 3)) {
    return false;
  }
  uint32_t pressure = ((((uint32_t)data[2]) << 16) | (((uint32_t)data[1]) << 8) | data[0]);
  if (pressure & 0x00800000) {
    pressure |= 0xFF000000;
  }
    debgugValue[sda]= (int32_t)pressure;

    *pPressure = ((int32_t)pressure) / 4096.0f;
  return true;
}

//------------------
int32_t read_debugValue(int side) {
  return debgugValue[side];
}
