#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Preferences.h>
#include <M5Unified.h>

//#define AFUUE_VER (1090) // 8Bit-DAC ADC-Direct    (AFUUE2 First)
//#define AFUUE_VER (1100) // 16Bit-PWM ADC-MCP3425  (AFUUE2 Second)
//#define AFUUE_VER (1120) // 16Bit-PWM LPS33        (AFUUE2 Gen3)
//#define AFUUE_VER (1110) // 16Bit-PWM ADCx2        (AFUUE2R First)
//#define AFUUE_VER (1130) // 16Bit-PWM LPS33x2      (AFUUE2R Gen2)
#define AFUUE_VER (1140)  // 16Bit-PWM ADC-MCP3425  (AFUUE2R Gen2 Lite)

/*
Arduino IDE 2.3.4

M5Stack board manager 3.2.0
esp32 board manager 2.0.17

M5Stack library 0.4.6
M5Unified library 0.2.7
Adafruit_MCP23017 library 2.3.2
Adafruit_PCF8574 library 1.1.1
MIDI Library 5.0.2

Adafruit TinyUSB Library 3.1.3 (AFUUE2R に必要。バージョンが違うとビルドできない可能性大)
*/

/*
AFUUE2R は USB デバイスとして動作させるために USB-MODE:USB-OTG などを設定する必要がある。書き込み時はFuncボタンを押しながら USB ケーブルを PC に接続する。
AFUUE2R lite  は M5AtomS3 lite の横にあるボタンを押しながら PC に接続する。

Arduino IDE の設定
USB CDC On Boot  : "Enabled"
USB DFU On Boot  : "Disabled"
Events Run On    : Core1
JTAG Adapter     : "Disabled"
Arduino Runs On  : Core1
Events Run On    : Core1
USB Firmware MSC On Boot: "Disabled"
Upload Mode      : "UART0/Hardware CDC"
USB Mode         : "USB-OTG (TinyUSB)"

https://qiita.com/tomoto335/items/d20aa668a62ad49cda36
USB-MIDI についてはこちらの記事を参考にさせていただきました
*/

#define M5STICKC_PLUS (0)
#define M5STAMP_S3 (1)
#define M5ATOM_S3 (2)

#if (AFUUE_VER == 1090)
// AFUUE2 初代
#define MAINUNIT (M5STICKC_PLUS)
#define ENABLE_ADC

#elif (AFUUE_VER == 1100)
// AFUUE2 改良版
#define MAINUNIT (M5STICKC_PLUS)
#define SOUND_TWOPWM
#define USE_MCP3425

#elif (AFUUE_VER == 1110)
// AFUUE2R 初代
#define MAINUNIT (M5STAMP_S3)
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#define USE_MIDI
#define SOUND_TWOPWM
#define USE_INTERNALADC
#define HAS_LIPSENSOR

#elif (AFUUE_VER == 1120)
// AFUUE2 LPS33 Test
#define MAINUNIT (M5STICKC_PLUS)
#define SOUND_TWOPWM
#define USE_LPS33
#include <Arduino_LPS22HB.h>

#elif (AFUUE_VER == 1130)
// AFUUE2R Gen2
#define MAINUNIT (M5ATOM_S3)
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#define USE_MIDI
#define SOUND_TWOPWM
#define USE_LPS33
#define HAS_LIPSENSOR
#define LEDPIN (8)
#define I2CPIN_SDA (38)
#define I2CPIN_SCL (39)
#define TOUCHPIN (2)

#elif (AFUUE_VER == 1140)
// AFUUE2R Gen2 Lite
#define MAINUNIT (M5ATOM_S3)
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#define USE_MIDI
#define SOUND_TWOPWM
#define USE_MCP3425
#define LEDPIN (8)
#define I2CPIN_SDA (38)
#define I2CPIN_SCL (39)
#define TOUCHPIN (2)

#endif

#define CORE0 (0)
#define CORE1 (1)

#if (MAINUNIT == M5STICKC_PLUS)
// M5StickC Plus ------------
#define HAS_IMU
#define HAS_RTC
#define HAS_INTERNALBATTERY
#define HAS_DISPLAY
#define HAS_IOEXPANDER
#define DISPLAY_WIDTH (240)  // M5StickC Plus
#define DISPLAY_HEIGHT (135)
//#define DISPLAY_WIDTH (128) // M5AtomS3
//#define DISPLAY_HEIGHT (128)
#ifdef SOUND_TWOPWM
#define PWMPIN_LOW (0)
#define PWMPIN_HIGH (26)
#else
#define DACPIN (DAC2)  // 26
#endif

#define LEDPIN (10)
#define ADCPIN (ADC2)  // 36

#elif (MAINUNIT == M5STAMP_S3)
// STAMPS3 -------------
#define NEOPIXEL_PIN (21)
#ifdef SOUND_TWOPWM
#define PWMPIN_LOW (39)
#define PWMPIN_HIGH (40)
#else
#define DACPIN (39)
#endif
#ifdef HAS_LIPSENSOR
#define ADCPIN (11)
#define ADCPIN2 (12)
#endif  //HAS_LIPSENSOR
#define MIDI_IN_PIN (42)
#define MIDI_OUT_PIN (41)

#elif (MAINUNIT == M5ATOM_S3)
#define NEOPIXEL_PIN (35)
#define HAS_IOEXPANDER
#define PWMPIN_LOW (5)
#define PWMPIN_HIGH (6)
#define MIDI_IN_PIN (9)  // not use
#define MIDI_OUT_PIN (7)

// for M5AtomS3 (not Lite)
//#define HAS_DISPLAY
//#define DISPLAY_WIDTH (128)
//#define DISPLAY_HEIGHT (128)

#endif  //--------------

#ifdef USE_MIDI
#include <MIDI.h>
#endif

#ifdef USE_MCP3425
#define MCP3425_ADDR (0x68)
#endif

#ifdef USE_LPS33
#define LPS33_ADDR (0x5C)
#endif


#define CLOCK_DIVIDER (80)
#define TIMER_ALARM (40)
//#define SAMPLING_RATE (20000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*50) = 20kHz
#define SAMPLING_RATE (25000.0f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*40) = 25kHz
//#define SAMPLING_RATE (32000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (50*50) = 32kHz
//#define SAMPLING_RATE (44077.13f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (55*33) = 44kHz
//#define SAMPLING_RATE (48019.2f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (49*34) = 48kHz

//#define USE_BLE_MIDI
//#define USE_SERIALOUTPUT

extern volatile uint8_t waveOutH;
extern volatile uint8_t waveOutL;
extern TaskHandle_t taskHandle;
extern int baseNote;

extern void SerialPrintLn(const char* text);
extern void SetLedColor(int r, int g, int b);
extern void DrawCenterString(const char* str, int x, int y, int fontId);
