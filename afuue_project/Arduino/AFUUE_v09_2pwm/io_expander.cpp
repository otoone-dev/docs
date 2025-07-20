#include "io_expander.h"

#include <Wire.h>
#include <Adafruit_MCP23X17.h> // 2.3.2
#include <Adafruit_PCF8574.h>  // 1.1.1

Adafruit_MCP23X17 mcp;
Adafruit_PCF8574 pcf0, pcf1;
static int expanderType = -1;

bool setupMCP23017() {
  bool ret = mcp.begin_I2C();
  if (ret == false) {
    return false;
  }
  for (int i = 0; i < 16; i++) {
    mcp.pinMode(i, INPUT_PULLUP);
  }
  return true;
}

uint16_t readMCP23017() {
  uint16_t gpioA = mcp.readGPIO(0);
  uint16_t gpioB = mcp.readGPIO(1);
  return (gpioB << 8) | gpioA;
}

int setupPCF8574() {
  bool ret0 = pcf0.begin(0x38);
  bool ret1 = pcf1.begin(0x39);
  int ret = 0;
  if (ret0 == false) {
    ret |= 1;
  }
  if (ret1 == false) {
    ret |= 2;
  }
  if (ret == 0) {
    for (uint8_t p=0; p<8; p++) {
      pcf0.pinMode(p, INPUT_PULLUP);
      pcf1.pinMode(p, INPUT_PULLUP);
    }
  }
  return ret;
}

uint16_t readPCF8574() {
  uint16_t ret0 = (uint16_t)pcf0.digitalReadByte();
  uint16_t ret1 = (uint16_t)pcf1.digitalReadByte();
  return (ret1 << 8) | ret0;
}

int setupIOExpander() {
  expanderType = -1;
  if (setupMCP23017()) {
    expanderType = 1;
    return 1;
  }
  int ret = setupPCF8574();
  if (ret == 0) {
    expanderType = 2;
    return 2;
  }
  return -ret; // 1:MCP23017, 2:PCF8574, -1 or -2 :PCF8574 error , -3:no IOExpander detected
}

uint16_t readFromIOExpander() {
  switch (expanderType) {
    case 1:
      return readMCP23017();
    break;
    case 2:
      return readPCF8574();
    break;
  }
  return 0;
}
