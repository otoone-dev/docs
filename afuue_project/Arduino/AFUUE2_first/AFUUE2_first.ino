#include "afuue_common.h"
#include "afuue.h"

// 音声出力 16bit PWM
volatile uint8_t waveOutH = 0;
volatile uint8_t waveOutL = 0;

// AFUUE 処理本体
Afuue afuue;

//-------------------------------------
bool HasLED() {
  return (M5.getBoard() == m5::board_t::board_M5StampS3 || M5.getBoard() == m5::board_t::board_M5AtomS3Lite);
}

bool HasDisplay() {
  return (M5.getBoard() == m5::board_t::board_M5StickCPlus || M5.getBoard() == m5::board_t::board_M5StickCPlus2 || M5.getBoard() == m5::board_t::board_M5AtomS3);
}

bool HasImu() {
  return (M5.getBoard() == m5::board_t::board_M5StickCPlus || M5.getBoard() == m5::board_t::board_M5StickCPlus2 || M5.getBoard() == m5::board_t::board_M5AtomS3);
}

//-------------------------------------
void SerialPrintLn(const char* text) {
#if ENABLE_SERIALOUTPUT
  Serial.println(text);
#endif
}

//-------------------------------------
void SetLedColor(int r, int g, int b) {
  if (HasLED()) {
    neopixelWrite(GPIO_NUM_21, r, g, b);
  }
}

//-------------------------------------
void DrawCenterString(const char* str, int x, int y, int fontId) {
  if (HasDisplay()) {
    M5.Lcd.drawCenterString(str, x, y, fontId);
  }
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
  delay(5000);
}

