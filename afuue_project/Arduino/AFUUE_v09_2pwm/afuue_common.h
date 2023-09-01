#ifndef AFUUE_COMMON_H
#define AFUUE_COMMON_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <WiFi.h>
#include <Wire.h>
#include <Preferences.h>
//#include <nvs.h>

//#define AFUUE_VER (109) // 8Bit-DAC ADC-Direct
#define AFUUE_VER (110) // 16Bit-PWM ADC-MCP3425

#if (AFUUE_VER == 110)
#define SOUND_TWOPWM
#define ENABLE_MCP3425
#endif

#define CORE0 (0)
#define CORE1 (1)

#include <M5Unified.h>
#define _M5STICKC_H_ (1)

#ifdef _M5STICKC_H_
// M5StickC Plus ------------
#define _M5DISPLAY_H_
#define ENABLE_IMU
//#define ENABLE_SPEAKERHAT
#ifdef SOUND_TWOPWM
#define PWMPIN_LOW (0)
#define PWMPIN_HIGH (26)
#else
#define DACPIN (DAC2) // 26
#endif

#define LEDPIN (10) // M5StickC
#define ADCPIN (ADC2) // 36
#define ENABLE_RTC

#else
// ESP32 Devkit -------------
#define DACPIN (DAC1) // 25
#define LEDPIN (33) // ESP32-Devkit

#define ENABLE_ADXL345
#ifdef ENABLE_ADXL345
#define ADXL345_DEVICE (0x53)    // ADXL345 device address
#define ADXL345_RESO (0.0039f) // 3.9mG
#define ADXL345_POWER_CTL 0x2d
#define ADXL345_DATAX0 0x32
#endif
//---------------------------
#endif

#define ENABLE_BMP180 (0)
#ifdef ENABLE_MCP3425
#define MCP3425_ADDR (0x68)
#endif
#define ENABLE_CHORD_PLAY (0)
#define ENABLE_COMMUNICATETOOL (0)
#define KEYDATACHECK (0)
#define ENABLE_SERIALOUTPUT (0)//(KEYDATACHECK && 1)
#define ENABLE_BLE_MIDI (0)
#define ENABLE_MIDI (0)
#define ENABLE_FMSG (0)
#define ENABLE_ANALOGREAD_GPIO32 (0)

extern int baseNote;
extern void SerialPrintLn(char* text);

#endif // AFUUE_COMMON_H
