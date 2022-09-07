#ifndef AFUUE_COMMON_H
#define AFUUE_COMMON_H

#ifdef _M5STICKC_H_
#include <M5StickCPlus.h>
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <WiFi.h>
#include <Wire.h>
#include <Preferences.h>
//#include <nvs.h>

#define AFUUE_VER (109)

#define CORE0 (0)
#define CORE1 (1)

#ifdef _M5STICKC_H_
// M5StickC Plus ------------
#define ENABLE_IMU
//#define ENABLE_SPEAKERHAT
#define DACPIN (DAC2) // 26
#define LEDPIN (10) // M5StickC
#include "MCP23017.h"

#else
// ESP32 Devkit -------------
#define DACPIN (DAC1) // 25
#define LEDPIN (33) // ESP32-Devkit

#define ENABLE_ADXL345
#ifdef ENABLE_ADXL345
#define ADXL345_DEVICE (0x53)    // ADXL345 device address
#define ADXL345_RESO (0.0039) // 3.9mG
#define ADXL345_POWER_CTL 0x2d
#define ADXL345_DATAX0 0x32
#endif
//---------------------------
#endif

#define ENABLE_BMP180 (0)
#define ENABLE_CHORD_PLAY (0)
#define ENABLE_COMMUNICATETOOL (0)
#define KEYDATACHECK (0)
#define ENABLE_SERIALOUTPUT (1)//(KEYDATACHECK && 1)
#define ENABLE_BLE_MIDI (0)
#define ENABLE_MIDI (0)
#define ENABLE_FMSG (0)
#define ENABLE_ANALOGREAD_GPIO32 (0)

extern int baseNote;
extern void SerialPrintLn(char* text);

#endif // AFUUE_COMMON_H
