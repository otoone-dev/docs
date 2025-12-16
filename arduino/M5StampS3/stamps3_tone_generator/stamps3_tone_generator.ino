#include <Arduino.h>
#include <Wire.h>
#include "wave_generator.h"
#include <cstdint>
#include "midi.h"
#include <FastLED.h>
#include <M5Unified.h>

Menu menu;
WaveGenerator generator;
// タイマー割り込み用
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
#define QUEUE_LENGTH 1
TaskHandle_t taskHandle;
hw_timer_t* timer = NULL;
QueueHandle_t xQueue;
unsigned long loopTime;
volatile float currentNote = 61.0f;
volatile float currentBC = 0.0f;
volatile float currentBend = 0.0f;
volatile float currentVol = 0.0f;
volatile bool isPlaying = false;
volatile bool isPressed = false;

#define SOUND_TWOPWM
#define I2CPIN_SDA (11)
#define I2CPIN_SCL (12)
#define SCREEN_WIDTH (128)
#define SCREEN_HEIGHT (32)

#define LED_PIN   (21)
#define NUM_LEDS  (1)
static CRGB leds[NUM_LEDS];
void setLed(CRGB color) {
  // change RGB to GRB
  uint8_t t = color.r;
  color.r = color.g;
  color.g = t;
  leds[0] = color;
  FastLED.show();
}

#define PIN_BUTTON 0    // 本体ボタンの使用端子（G0）
#define PIN_OUTPUT 43   // 外部LED
#define PIN_LED    21   // 本体フルカラーLEDの使用端子（G21）
#define NUM_LEDS   1    // 本体フルカラーLEDの数

#define MIDI_OUT_PIN 41
#define MIDI_IN_PIN 42

// 変数宣言
bool state = false;     // 本体ボタン状態格納用
bool led_value = false; // LED状態格納用
int32_t recv_cnt = 0;

// ロータリエンコーダータスク -------------------------------------------------
static const int enc_step[16] = {
   0,-1, 1, 0,
   1, 0, 0,-1,
  -1, 0, 0, 1,
   0, 1,-1, 0 
};
volatile int value = 0;
volatile int mode = 0;
volatile int prev_enc = 0;
volatile int enc_p = 0;
int ReadEnc() {
  return (((int)digitalRead(13)) << 1) | (int)digitalRead(14);
}
void Enc() {
  int d = ReadEnc();
  enc_p += enc_step[(prev_enc << 2) | d];
  prev_enc = d;
  if (enc_p <= -4) {
    enc_p = 0;
    value--;
  }
  else if (enc_p >= 4) {
    enc_p = 0;
    value++;
  }
}

volatile unsigned long pressTime = 0;
volatile unsigned long msgTime = 0;
volatile bool shortPressed = false;
volatile bool longPressed = false;
volatile int menu_value = 0;
// 更新 -------------------------------------------------
void UpdateTask(void *pvParameters) {
  while (1) {
    unsigned long t0 = micros();
    float td = (t0 - loopTime)/1000.0f;
    loopTime = t0;
  
    while (Serial2.available()) {
      uint8_t d = Serial2.read();
      MidiReceive(d);
    }
  
    int br = 10 + (int)(currentVol * 245.0f);
    setLed(CRGB(1, br, 1));
    float vol = 0.0f;
    float note = 0.0f;
    if (isMidiPlaying) {
      float bc = (playingBC - 10) / 117.0f;
      currentBC += (bc - currentBC) * 0.5f;
      float bend = (pitchBend / 8192.0f);
      if (bend < -2.0f) bend = -2.0f;
      if (bend > 2.0f) bend = 2.0f;
      currentBend += (bend - currentBend) * 0.2f;
      vol = currentBC * currentBC
      ;
      if (vol < 0.0f) {
        vol = 0.0f;
      }
      if (vol > 1.0f) {
        vol = 1.0f;
      }
      note = playingNote;
      if (note < 2) note = 2;
      if (note > 125) note = 125;
    }
    else {
      note = 69;
    }
    currentNote += (note - currentNote) * 0.9f;
    if (digitalRead(15) == LOW) vol = 0.2f;
    currentVol = vol;
    generator.requestedVolume = vol;
    generator.Tick(currentNote + currentBend * 2.0f, td);

    unsigned long t = micros();
    if (digitalRead(15) == LOW) {
      if (pressTime == 0) {
        pressTime = t;
      }
    }
    else 
    {
      if (pressTime > 0) {
        unsigned long dt = t - pressTime;
        if (dt > 1000*1000) {
          // got to menu
          longPressed = true;
        }
        else if (dt > 100*1000) {
          shortPressed = true;
        }
        pressTime = 0;
      }
    }
    delay(10);
  }
}

// 波形生成 -------------------------------------------------
void CreateWaveTask(void *pvParameters) {
  WaveGenerator* pGenerator = reinterpret_cast<WaveGenerator*>(pvParameters);
  while (1) {
    uint32_t data;
    xTaskNotifyWait(0, 0, &data, portMAX_DELAY);

    pGenerator->CreateWave(true);
  }
}

//---------------------------------
void IRAM_ATTR OnTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  uint16_t w = waveOut[(waveOutWriteIdx + WAVEOUT_BUFFERSIZE - 1) % WAVEOUT_BUFFERSIZE];
#ifdef SOUND_TWOPWM
  ledcWrite(0, w & 0xFF);
  ledcWrite(1, w >> 8);
#else
  dacWrite(DACPIN, w >> 8);
#endif

  BaseType_t higherPriorityTaskWoken = pdFALSE;
  xTaskNotifyFromISR(taskHandle, 0, eNoAction, &higherPriorityTaskWoken);
  portEXIT_CRITICAL_ISR(&timerMux);
}

// 初期設定 -------------------------------------------------
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  FastLED.addLeds<WS2811, LED_PIN, RGB>(leds, NUM_LEDS);
  setLed(CRGB::Red);
  
  //USBSerial.begin(115200);        // USBシリアル通信初期化
  //USBSerial.println("S3");
  Serial.begin(115200);
  Serial.println("S3");

  Serial2.begin(31250, SERIAL_8N1, MIDI_IN_PIN, MIDI_OUT_PIN);

  // 入出力端子設定
  pinMode(PIN_BUTTON, INPUT);   // 本体ボタン（入力）（INPUT_PULLUPでプルアップ指定）
  pinMode(PIN_OUTPUT, OUTPUT);  // 外付けLED（出力）

  Wire.begin(I2CPIN_SDA, I2CPIN_SCL);
  Wire.setClock(400*1000); // 400kHz

  menu.Initialize();
  menu.SetWaveIndex(0);
  setLed(CRGB::Blue);
  menu.DisplayMessage("OTOONE");
  delay(1000);
  
  setLed(CRGB::Green);
    generator.Initialize(&menu.waveData);
    WaveInfo* pInfo = generator.GetWaveInfo();
    pInfo->ApplyFromWaveSettings(menu.currentWaveSettings);

    xQueue = xQueueCreate(QUEUE_LENGTH, sizeof(int8_t));
    xTaskCreatePinnedToCore(CreateWaveTask, "createWaveTask", 16384, &generator, configMAX_PRIORITIES, &taskHandle, 0);

    timer = timerBegin(0, CLOCK_DIVIDER, true); // Events Run On は Core1 想定
    timerAttachInterrupt(timer, &OnTimer, false);
    timerAlarmWrite(timer, TIMER_ALARM, true);
    timerAlarmEnable(timer);
    

  pinMode(13, INPUT);
  pinMode(14, INPUT);
  pinMode(15, INPUT);
  prev_enc = ReadEnc();
  attachInterrupt(13, Enc, CHANGE);
  attachInterrupt(14, Enc, CHANGE);
  pinMode(MIDI_IN_PIN, INPUT);
  xTaskCreatePinnedToCore(UpdateTask, "UpdateTask", 2048, NULL, 2, NULL, 0);
  loopTime = micros();
}

// メイン ---------------------------------------------------
void loop() {
  bool nextBtn = (value > menu_value);
  bool prevBtn = (value < menu_value);
  menu_value = value;
  bool ret = menu.Update(shortPressed, longPressed, nextBtn, prevBtn);
  shortPressed = false;
  longPressed = false;
  if (ret) {
    menu.BeginPreferences(); {
#if 0
      if (menu.factoryResetRequested) {
        //出荷時状態に戻す
        M5.Lcd.fillScreen(BLACK);
        menu.DrawString("FACTORY RESET", 10, 10);
        menu.ClearAllFlash(); // CLEAR ALL FLASH MEMORY
        delay(2000);
        ESP.restart();
      }
#endif
      setLed(CRGB::Blue);
      generator.currentWaveTable = menu.waveData.GetWaveTable(menu.waveIndex);
      generator.GetWaveInfo()->ApplyFromWaveSettings(menu.currentWaveSettings);
      const char* name = menu.waveData.GetWaveName(menu.waveIndex);
      menu.DisplayMessage(name);
      delay(500);
      menu.SavePreferences();
    } menu.EndPreferences();
  }
  menu.Display(currentVol, currentBend);
  delay(10);
}
