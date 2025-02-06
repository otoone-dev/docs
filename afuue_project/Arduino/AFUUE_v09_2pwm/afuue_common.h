#pragma once

#include <WiFi.h>
#include <Wire.h>
#include <Preferences.h>
#include <M5Unified.h>

//#define AFUUE_VER (1090) // 8Bit-DAC ADC-Direct    (AFUUE2 First)
#define AFUUE_VER (1100) // 16Bit-PWM ADC-MCP3425  (AFUUE2 Second)
//#define AFUUE_VER (1120) // 16Bit-PWM LPS33        (AFUUE2 Test)
//#define AFUUE_VER (1110) // 16Bit-PWM ADCx2        (AFUUE2R First)
//#define AFUUE_VER (1130) // 16Bit-PWM LPS33x2      (AFUUE2R Second)

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

#if (AFUUE_VER == 1090)
// AFUUE2 初代
#define _M5STICKC_H_
#define ENABLE_ADC

#elif (AFUUE_VER == 1100)
// AFUUE2 改良版
#define _M5STICKC_H_
#define SOUND_TWOPWM
#define ENABLE_MCP3425

#elif (AFUUE_VER == 1110)
// AFUUE2R 初代
#define _STAMPS3_H_
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#define SOUND_TWOPWM
#define ENABLE_ADC
#define ENABLE_LIP

#elif (AFUUE_VER == 1120)
// AFUUE2 LPS33 Test
#define _M5STICKC_H_
#define SOUND_TWOPWM
#define ENABLE_LPS33
#include <Arduino_LPS22HB.h>

#elif (AFUUE_VER == 1130)
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
#define HAS_IMU
#define HAS_DISPLAY
#define DISPLAY_WIDTH (240) // M5StickC Plus
#define DISPLAY_HEIGHT (135)
//#define DISPLAY_WIDTH (128) // M5AtomS3
//#define DISPLAY_HEIGHT (128)
#ifdef SOUND_TWOPWM
#define PWMPIN_LOW (0)
#define PWMPIN_HIGH (26)
#else
#define DACPIN (DAC2) // 26
#endif

#define LEDPIN (10)
#define ADCPIN (ADC2) // 36
#define ENABLE_RTC (1)
#define ENABLE_MIDI (0)
#endif //--------------

#ifdef _STAMPS3_H_
// STAMPS3 -------------
#define HAS_LED
#ifdef SOUND_TWOPWM
#define PWMPIN_LOW (39)
#define PWMPIN_HIGH (40)
#else
#define DACPIN (39)
#endif
#ifdef ENABLE_LIP
#define ADCPIN (11)
#define ADCPIN2 (12)
#endif //ENABLE_LIP
#define ENABLE_RTC (0)
#define ENABLE_MIDI (1)
#define MIDI_IN_PIN (42)
#define MIDI_OUT_PIN (41)
#endif //--------------

#if 0
// ESP32 Devkit -------------
#define DACPIN (DAC1) // 25
#define LEDPIN (33) // ESP32-Devkit
#define ENABLE_RTC (0)
#define ENABLE_MIDI (0)
#endif //--------------


#ifdef ENABLE_MCP3425
#define MCP3425_ADDR (0x68)
#endif

#ifdef ENABLE_LPS33
#define LPS33_ADDR (0x5C)
#endif



#define CLOCK_DIVIDER (80)
#define TIMER_ALARM (40)
//#define SAMPLING_RATE (20000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*50) = 20kHz
#define SAMPLING_RATE (25000.0f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*40) = 25kHz
//#define SAMPLING_RATE (32000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (50*50) = 32kHz
//#define SAMPLING_RATE (44077.13f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (55*33) = 44kHz
//#define SAMPLING_RATE (48019.2f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (49*34) = 48kHz

//#define ENABLE_COMMUNICATETOOL (0)
#define ENABLE_BLE_MIDI (0)
#define ENABLE_SERIALOUTPUT (0)

extern volatile uint8_t waveOutH;
extern volatile uint8_t waveOutL;
extern TaskHandle_t taskHandle;
extern int baseNote;

extern bool IsStickC();
extern bool IsStampS3();
extern bool IsAtomS3();
extern bool HasLED();
extern bool HasDisplay();
extern bool HasImu();

extern void SerialPrintLn(const char* text);
extern void SetLedColor(int r, int g, int b);
extern void DrawCenterString(const char* str, int x, int y, int fontId);
