#ifndef AFUUE_COMMON_H
#define AFUUE_COMMON_H

#include <M5StickCPlus.h>
#include <WiFi.h>
#include <Wire.h>
#include <Preferences.h>
//#include <nvs.h>

#define AFUUE_VER (107)

#define CORE0 (0)
#define CORE1 (1)

#define ENABLE_ADXL345 (0)
#if ENABLE_ADXL345
#define ADXL345_DEVICE (0x53)    // ADXL345 device address
#define ADXL345_RESO (0.0039) // 3.9mG
#define ADXL345_POWER_CTL 0x2d
#define ADXL345_DATAX0 0x32
#endif

#define ENABLE_IMU (1)

#define ENABLE_CHORD_PLAY (0)
#define ENABLE_COMMUNICATETOOL (0)
#define KEYDATACHECK (0)
#define ENABLE_SERIALOUTPUT (KEYDATACHECK && 1)
#define ENABLE_BLE_MIDI (0)
#define ENABLE_MIDI (0)
#define ENABLE_FMSG (0)
#define ENABLE_ANALOGREAD_GPIO32 (0)

extern int baseNote;
extern void SerialPrintLn(char* text);

#endif
