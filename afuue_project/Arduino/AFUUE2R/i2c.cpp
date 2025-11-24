#include "afuue_common.h"
#include "i2c.h"

//---------------------------------
char I2C_ReadBytes(uint8_t addr, uint8_t* values, uint8_t length)
{
  char x;

  Wire.beginTransmission(addr);
  Wire.write(values[0]);
  bool err = Wire.endTransmission();
  if (err == 0)
  {
    Wire.requestFrom(addr,length);
    while(Wire.available() != length) ; // wait until bytes are ready
    for(x=0;x<length;x++)
    {
      values[x] = Wire.read();
    }
    return(1);
  }
  return(0);
}

//---------------------------------
char I2C_WriteBytes(unsigned char addr, unsigned char *values, char length)
{
  char x;
  
  Wire.beginTransmission(addr);
  Wire.write(values,length);
  bool err = Wire.endTransmission();
  if (err == 0)
    return(1);
  else
    return(0);
}
