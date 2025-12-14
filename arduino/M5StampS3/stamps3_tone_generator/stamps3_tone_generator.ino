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

// 波形生成 -------------------------------------------------
void CreateWaveTask(void *pvParameters) {
  WaveGenerator* pGenerator = reinterpret_cast<WaveGenerator*>(pvParameters);
  while (1) {
    uint32_t data;
    xTaskNotifyWait(0, 0, &data, portMAX_DELAY);

    pGenerator->CreateWave(true);
  }
}

unsigned long pressTime = 0;
unsigned long msgTime = 0;
char msg[256];
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
    neopixelWrite(NEOPIXEL_PIN, 1, br, 1);
    float vol = 0.0f;
    float note = 0.0f;
    if (isMidiPlaying) {
      float bc = (playingBC - 10) / 117.0f;
      currentBC += (bc - currentBC) * 0.5f;
      float bend = (pitchBend / 8192.0f);
      if (bend < -2.0f) bend = -2.0f;
      if (bend > 2.0f) bend = 2.0f;
      currentBend += (bend - currentBend) * 0.2f;
      vol = currentBC;
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
      note = 61 + value;
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
        }
        else if (dt > 100*1000) {
          // change instrument
          msgTime = t;
          sprintf(msg, "TEST");
        }
        pressTime = 0;
      }
    }
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

  neopixelWrite(NEOPIXEL_PIN, 0, 0, 100);

  auto cfg = M5.config();
  M5.begin(cfg);

  Wire.begin(I2CPIN_SDA, I2CPIN_SCL);
  Wire.setClock(400*1000); // 400kHz
  ssd1306_init();

  clearBuffer();
  DrawString("OTOONE", 2, 2);
  ssd1306_update();
  delay(1000);

  neopixelWrite(NEOPIXEL_PIN, 1, 1, 1);
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
  prev_enc = ReadEnc();
  attachInterrupt(13, Enc, CHANGE);
  attachInterrupt(14, Enc, CHANGE);
  pinMode(MIDI_IN_PIN, INPUT);
  xTaskCreatePinnedToCore(UpdateTask, "UpdateTask", 2048, NULL, 2, NULL, 0);
  loopTime = micros();
}

// メイン ---------------------------------------------------
void loop() {
    clearBuffer();
    fillRect(2, 32+2, (int)(120.0f*currentVol), 14, true);
    fillRect(2, 32+25, 120, 2, true);
    int x = 64 + (int)(60.0f * currentBend);
    fillRect(x-5, 32+20, 5, 14, true);
    if (msgTime > 0) {
      DrawString(msg, 2, 0);
      if (msgTime + 1000*1000 < micros()) {
        msgTime = 0;
      }
    }
    else {
      char s[32];
      sprintf(s, "%1.1f, %1.1f", currentVol, currentBend);
      DrawString(s, 2, 0);
    }
    ssd1306_update();
  delay(10);
}
