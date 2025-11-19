#include <Arduino.h>
#include <Wire.h>
#include "wave_generator.h"
#include <cstdint>
#include "ssd1306.h"
#include "ascii_fonts.h"
#include "midi.h"

WaveInfo waveInfo;
WaveGenerator generator(&waveInfo);
WaveSettings waveSettings;
// タイマー割り込み用
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
#define QUEUE_LENGTH 1
TaskHandle_t taskHandle;
hw_timer_t* timer = NULL;
QueueHandle_t xQueue;
unsigned long loopTime;

#define SOUND_TWOPWM
#define I2CPIN_SDA (11)
#define I2CPIN_SCL (12)
#define SCREEN_WIDTH (128)
#define SCREEN_HEIGHT (32)

#include <M5Unified.h>
#define NEOPIXEL_PIN (21)

#define PIN_BUTTON 0    // 本体ボタンの使用端子（G0）
#define PIN_OUTPUT 43   // 外部LED
#define PIN_LED    21   // 本体フルカラーLEDの使用端子（G21）
#define NUM_LEDS   1    // 本体フルカラーLEDの数

#define MIDI_OUT_PIN 41
#define MIDI_IN_PIN 42

// 変数宣言
bool state = false;     // 本体ボタン状態格納用
bool led_value = false; // LED状態格納用
std::string msg[3] = {"MIDI Ready", "", ""};
int32_t recv_cnt = 0;

// ロータリエンコーダータスク -------------------------------------------------
volatile int value = 0;
volatile int mode = 0;
volatile bool d13 = HIGH;
volatile bool d14 = HIGH;
void Enc() {
  uint8_t d = ((uint8_t)d14) << 1 | d13;
  switch (mode) {
    case 0:
    {
      if (d == 0b10) {
        mode = 1; // +
      }
      else if (d == 0b01) {
        mode = 11; // -
      }
    }
    break;
    case 1: // 10
    {
      if (d == 0b00) {
        mode = 2;
      }
      else if (d == 0b11) {
        mode = 0;
      }
    }
    break;
    case 2: // 00
    {
      if (d == 0b01) {
        mode = 3;
      }
      else if (d == 0b10) {
        mode = 1;
      }
    }
    break;
    case 3: // 01
    if (d == 0b11) {
      value++;
      mode = 0;
    }
    else if (d == 0b10) {
      mode = 1;
    }
    break;

    case 11: // 01
    {
      if (d == 0b00) {
        mode = 12;
      }
      else if (d == 0b11) {
        mode = 0;
      }
    }
    break;
    case 12: // 00
    {
      if (d == 0b10) 
      {
        mode = 13;
      }
      else if (d == 0b01) {
        mode = 11;
      }
    }
    break;
    case 13: // 10
    {
      if (d == 0b11) 
      {
        value--;
        mode = 0;
      }
      else if (d == 0b00) {
        mode = 12;
      }
    }
    break;
  }
}
void Enc13() {
  d13 = digitalRead(13);
  Enc();
}
void Enc14() {
  d14 = digitalRead(14);
  Enc();
}

volatile int32_t testCnt = 0;
//------------------
void Test() {
  testCnt++;
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
  
    int br = (value % 10) * 10;
    neopixelWrite(NEOPIXEL_PIN, 1, 1, br);
    generator.requestedVolume = ((digitalRead(15) == LOW) ? 0.2f : 0.0f);
    generator.Tick(61 + value, td);

    delay(10);
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
  waveInfo.ApplyFromWaveSettings(waveSettings);
  
  //USBSerial.begin(115200);        // USBシリアル通信初期化
  //USBSerial.println("S3");
  Serial.begin(115200);
  Serial.println("S3");

  Serial2.begin(31250, SERIAL_8N1, MIDI_IN_PIN, MIDI_OUT_PIN);

  // 入出力端子設定
  pinMode(PIN_BUTTON, INPUT);   // 本体ボタン（入力）（INPUT_PULLUPでプルアップ指定）
  pinMode(PIN_OUTPUT, OUTPUT);  // 外付けLED（出力）

  neopixelWrite(NEOPIXEL_PIN, 100, 100, 100);

  auto cfg = M5.config();
  M5.begin(cfg);

  Wire.begin(I2CPIN_SDA, I2CPIN_SCL);
  ssd1306_init();

  clearBuffer();
  DrawString("OTOONE", 2, 2);
  ssd1306_update();
  delay(1000);

    generator.Initialize();

    xQueue = xQueueCreate(QUEUE_LENGTH, sizeof(int8_t));
    xTaskCreatePinnedToCore(CreateWaveTask, "createWaveTask", 16384, &generator, configMAX_PRIORITIES, &taskHandle, 0);

    timer = timerBegin(0, CLOCK_DIVIDER, true); // Events Run On は Core1 想定
    timerAttachInterrupt(timer, &OnTimer, false);
    timerAlarmWrite(timer, TIMER_ALARM, true);
    timerAlarmEnable(timer);
    

  pinMode(13, INPUT);
  pinMode(14, INPUT);
  pinMode(15, INPUT);
  attachInterrupt(13, Enc13, CHANGE);
  attachInterrupt(14, Enc14, CHANGE);
  pinMode(MIDI_IN_PIN, INPUT);
  xTaskCreatePinnedToCore(UpdateTask, "UpdateTask", 2048, NULL, 2, NULL, 0);
  loopTime = micros();

  midiMsg = "READY";
}
// メイン ---------------------------------------------------
void loop() {
  clearBuffer();
  DrawString(midiMsg.c_str(), 2, 2);
  ssd1306_update();
  delay(1);
}
