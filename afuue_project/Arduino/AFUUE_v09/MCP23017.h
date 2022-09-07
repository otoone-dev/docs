/*
 * https://qazsedcftf.blogspot.com/2018/12/esp32arduinoi2cmcp23017.html
 * より拝借
 */

#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#define ENABLE_MCP23017

#if 1
Adafruit_MCP23X17 mcp;

bool setupMCP23017() {
  bool ret = mcp.begin_I2C();
  if (ret == false) {
    return ret;
  }
  for (int i = 0; i < 16; i++) {
    mcp.pinMode(i, INPUT_PULLUP);
  }
  return ret;
}

uint16_t readMCP23017() {
  uint16_t gpioA = mcp.readGPIO(0);
  uint16_t gpioB = mcp.readGPIO(1);
  return (gpioB << 8) | gpioA;
}

#else
byte MCP23017  = 0x20 ;       // slave address (to determine address, setup A0,A1,A2 hardware pins)
byte IODIRA    = 0x00 ;       // IODIRA Register Address
byte IODIRB    = 0x01 ;       // IODIRB Register Address
byte GPPUA     = 0x0C ;       // GPPUA Register Address
byte GPPUB     = 0x0D ;       // GPPUA Register Address
byte GPIOA     = 0x12 ;       // GPIOA Register Address
byte GPIOB     = 0x13 ;       // GPIOA Register Address

void I2C_BWR(byte TGT, byte REGADR, byte DATA) {
  Wire.beginTransmission(TGT);         // transmit to device #TGT 
  Wire.write(REGADR);                  // send REG address (REGADR)
  Wire.write(DATA);                    // send write data (DATA)
  Wire.endTransmission();              // end transmit
}
 
byte I2C_BRD(byte TGT,byte REGADR) {
  byte data = 0 ;
  Wire.beginTransmission(TGT);         //  transmit to device #TGT
  Wire.write(REGADR);                  //  send REG address (REGADR)
  Wire.endTransmission();              //  end transmit
  Wire.requestFrom(TGT, 1);            // request 1 byte from slave device #TGT
  data = Wire.read();                  // receive 1 byte 
   
  return data ;
}

void setupMCP23017() {
  I2C_BWR(MCP23017,GPIOA,0x00) ;    // set all IOA : LOW
  I2C_BWR(MCP23017,GPIOB,0x00) ;    // set all IOB : LOW
  // set direction IOA,IOB
  I2C_BWR(MCP23017,IODIRA,0xFF) ;   // set all IOA : input
  I2C_BWR(MCP23017,IODIRB,0xFF) ;   // set all IOB : input
  I2C_BWR(MCP23017,GPPUA,0xFF) ;    // set all IOA : PullUp
  I2C_BWR(MCP23017,GPPUB,0xFF) ;    // set all IOA : PullUp
  delay(100);
}

uint16_t readMCP23017() {
  uint16_t dataA, dataB ;
  dataA = (uint16_t)I2C_BRD(MCP23017,GPIOA);
  dataB = (uint16_t)I2C_BRD(MCP23017,GPIOB);
  return (uint16_t)((dataB << 8) || dataA);
}

#endif
