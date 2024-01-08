/*
 * https://qazsedcftf.blogspot.com/2018/12/esp32arduinoi2cmcp23017.html
 * より拝借
 */

#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#define ENABLE_MCP23017 (1)

#define PIN_I2C_SCL (33)
#define PIN_I2C_SDA (32)

Adafruit_MCP23X17 mcp[4];
TwoWire myWire(1);

void setupPins(int id) {
  mcp[id].begin_I2C(MCP23XXX_ADDR + id, &myWire);
  for (int i = 0; i < 16; i++) {
    mcp[id].pinMode(i, INPUT_PULLUP);
  }
}

void setupMCP23017() {
  myWire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  myWire.setClock(100 * 1000);
  setupPins(0);
  setupPins(1);
  setupPins(2);
  setupPins(3);
}

uint16_t readMCP23017(int id) {
  uint16_t gpioA = mcp[id].readGPIO(0);
  uint16_t gpioB = mcp[id].readGPIO(1);
  return (gpioB << 8) | gpioA;
}
