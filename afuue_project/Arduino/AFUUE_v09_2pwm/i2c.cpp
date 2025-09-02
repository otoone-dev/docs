#include "afuue_common.h"
#include "i2c.h"

//---------------------------------
bool I2C_ReadBytes(uint8_t addr, uint8_t reg, uint8_t* values, uint8_t length)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false); // repeated start
  Wire.requestFrom(addr,length);
  while(Wire.available() != length) {
    delay(1);
  }; // wait until bytes are ready
  for(size_t x=0;x<length;x++)
  {
    values[x] = Wire.read();
  }
  uint8_t ret = Wire.endTransmission();
  return (ret == 0);
}

//---------------------------------
bool I2C_WriteBytes(uint8_t addr, uint8_t reg, uint8_t *values, uint8_t length)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(values,length);
  uint8_t ret = Wire.endTransmission();
  return (ret == 0);
}
