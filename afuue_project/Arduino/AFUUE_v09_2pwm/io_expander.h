/*
 * https://qazsedcftf.blogspot.com/2018/12/esp32arduinoi2cmcp23017.html
 * より拝借
 */
#pragma once
#include <cstdint>

extern bool setupMCP23017();
extern uint16_t readMCP23017();
extern int setupPCF8574();
extern uint16_t readPCF8574();
extern int setupIOExpander();
extern uint16_t readFromIOExpander();
