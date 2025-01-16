#ifndef AFUUE_COMMON_H
#define AFUUE_COMMON_H

#include <WiFi.h>
#include <Wire.h>
#include <Preferences.h>
#include <M5Unified.h>

//#define AFUUE_VER (109) // 8Bit-DAC ADC-Direct    (AFUUE2 First)
#define AFUUE_VER (110) // 16Bit-PWM ADC-MCP3425  (AFUUE2 Second)
//#define AFUUE_VER (112)   // 16Bit-PWM LPS33      (AFUUE2 Test)
//#define AFUUE_VER (111) // 16Bit-PWM ADCx2        (AFUUE2R First)
//#define AFUUE_VER (113)   // 16Bit-PWM LPS33      (AFUUE2R Second)

/*
Arduino IDE 2.3.4

M5Stack library 0.4.6
M5Unified library 0.2.0
Adafruit_MCP23017 library 2.3.2
Adafruit_PCF8574 library 1.1.1

Adafruit TinyUSB Library 3.1.3 (AFUUE2R に必要。バージョンが違うとビルドできない可能性大)
*/

/*
AFUUE2R は USB デバイスとして動作させるために USB-MODE:USB-OTG などを設定する必要がある。書き込み時はFuncボタンを押しながら USB ケーブルを PC に接続する。

Arduino IDE の設定
USB CDC On Boot  : "Enabled"
USB DFU On Boot  : "Disabled"
JTAG Adapter     : "Disabled"
USB Firmware MSC On Boot: "Disabled"
Upload Mode      : "UART0/Hardware CDC"
USB Mode         : "USB-OTG (TinyUSB)"

https://qiita.com/tomoto335/items/d20aa668a62ad49cda36
USB-MIDI についてはこちらの記事を参考にさせていただきました
*/

#if (AFUUE_VER == 109)
// AFUUE2 初代
#define _M5STICKC_H_
#define ENABLE_ADC

#elif (AFUUE_VER == 110)
// AFUUE2 改良版
#define _M5STICKC_H_
#define SOUND_TWOPWM
#define ENABLE_MCP3425

#elif (AFUUE_VER == 111)
// AFUUE2R 初代
#define _STAMPS3_H_
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#define SOUND_TWOPWM
#define ENABLE_ADC
#define ENABLE_ADC2

#elif (AFUUE_VER == 112)
// AFUUE2 LPS33 Test
#define _M5STICKC_H_
#define SOUND_TWOPWM
#define ENABLE_LPS33
#include <Arduino_LPS22HB.h>

#elif (AFUUE_VER == 113)
// AFUUE2R 改良版
#define _STAMPS3_H_
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#define SOUND_TWOPWM
#define ENABLE_LPS33
#endif

#define CORE0 (0)
#define CORE1 (1)

#ifdef _M5STICKC_H_
// M5StickC Plus ------------
#define _M5DISPLAY_H_
#define ENABLE_IMU
#ifdef SOUND_TWOPWM
#define PWMPIN_LOW (0)
#define PWMPIN_HIGH (26)
#else
#define DACPIN (DAC2) // 26
#endif

#define LEDPIN (10) // M5StickC
#define ADCPIN (ADC2) // 36
#define ENABLE_RTC
#endif //--------------

#ifdef _STAMPS3_H_
// STAMPS3 -------------
#ifdef SOUND_TWOPWM
#define PWMPIN_LOW (39)
#define PWMPIN_HIGH (40)
#else
#define DACPIN (39)
#endif
#ifdef ENABLE_ADC2
#define ADCPIN (11)
#define ADCPIN2 (12)
#endif //ENABLE_ADC2
#define MIDI_IN_PIN (42)
#define MIDI_OUT_PIN (41)
#endif //--------------

#if 0
// ESP32 Devkit -------------
#define DACPIN (DAC1) // 25
#define LEDPIN (33) // ESP32-Devkit

#define ENABLE_ADXL345
#ifdef ENABLE_ADXL345
#define ADXL345_DEVICE (0x53)    // ADXL345 device address
#define ADXL345_RESO (0.0039f) // 3.9mG
#define ADXL345_POWER_CTL 0x2d
#define ADXL345_DATAX0 0x32
#endif //ENABLE_ADXL345
#endif //--------------


#ifdef ENABLE_MCP3425
#define MCP3425_ADDR (0x68)
#endif

#ifdef ENABLE_LPS33
#define LPS33_ADDR (0x5C)
#endif


//#define ENABLE_COMMUNICATETOOL (0)
#define ENABLE_BLE_MIDI (0)
#define ENABLE_MIDI (1)
#define ENABLE_SERIALOUTPUT (1)//(0 && !ENABLE_MIDI)

extern int baseNote;
extern void SerialPrintLn(const char* text);

#endif // AFUUE_COMMON_H
