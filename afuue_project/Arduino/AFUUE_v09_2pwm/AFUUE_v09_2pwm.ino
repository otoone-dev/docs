#include "afuue_common.h"
#include "afuue.h"

// 音声出力 16bit PWM
volatile uint8_t waveOutH = 0;
volatile uint8_t waveOutL = 0;

// AFUUE 処理本体
Afuue afuue;

//-------------------------------------
void SerialPrintLn(const char* text) {
#if ENABLE_SERIALOUTPUT
  Serial.println(text);
#endif
}

//-------------------------------------
void SetLedColor(int r, int g, int b) {
#ifdef HAS_LED
  neopixelWrite(GPIO_NUM_21, r, g, b);
#endif
}

//-------------------------------------
void DrawCenterString(const char* str, int x, int y, int fontId) {
#ifdef HAS_DISPLAY
  M5.Lcd.drawCenterString(str, x, y, fontId);
#endif
}

//-------------------------------------
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  btStop();
  WiFi.mode(WIFI_OFF); 

  afuue.Initialize();
}

//-------------------------------------
void loop() {
  afuue.Loop();
}

