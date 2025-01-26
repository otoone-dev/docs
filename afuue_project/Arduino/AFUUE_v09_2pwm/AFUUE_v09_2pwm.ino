#include "afuue_common.h"
#include "wave_generator.h"
#include "key_system.h"
#include "sensors.h"
#include "i2c.h"
#include "menu.h"
#include "afuue_midi.h"

#include <cfloat>

static M5Canvas canvas(&M5.Lcd);
static Menu menu(&canvas);

static WaveInfo waveInfo;
static WaveGenerator generator(&waveInfo);
static KeySystem key;
static Sensors sensors;

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
#define QUEUE_LENGTH 1
QueueHandle_t xQueue;
TaskHandle_t taskHandle;
static bool enablePlay = false;

static float maximumVolume = 0.0f;
static float targetNote = 60.0f;
static float currentNote = 60.0f;
static float startNote = 60.0f;
static float keyTimeMs = 0.0f;
static float keySenseTimeMs = 50.0f;
static int attackSoftness = 0;
static float forcePlayTime = 0.0f;
static float noiseVolume = 0.0f;
static float noteOnTimeMs = 0.0f;
static float noteOnAccX, noteOnAccY, noteOnAccZ;
const float noiseLevel = 0.5f;
const float NOISETIME_LENGTH = 50.0f; //ms

//-------------------------------------
void SerialPrintLn(const char* text) {
#if ENABLE_SERIALOUTPUT
  Serial.println(text);
#endif
}

//-------------------------------------
void SetLedColor(int r, int g, int b) {
#ifdef _STAMPS3_H_
    neopixelWrite(GPIO_NUM_21, r, g, b);
#endif
}

//-------------------------------------
void DrawCenterString(const char* str, int x, int y, int fontId) {
#ifdef _M5STICKC_H_
  M5.Lcd.drawCentreString(str, x, y, fontId);
#endif
}

//-------------------------------------
#if 0
void SerialThread(void *pvParameters) {
    while (1) {
      char s[32];
      sprintf(s, "%d,%d,%d,%d", key.GetKeyData(),(int)(accx*100), (int)(accy*100), (int)(accz*100));
      Serial.println(s);
      delay(50);
    }
}
#endif

//-------------------------------------
void SynthesizerThread(void *pvParameters) {
  unsigned long loopTime = micros();
  while (1) {
    unsigned long t0 = micros();
    float td = (t0 - loopTime)/1000.0f;
    loopTime = t0;

#if ENABLE_MIDI || ENABLE_BLE_MIDI
    if (menu.isMidiEnabled) {
      MIDI_Exec();
    } else
#endif
    {
      // ノイズ
      if (generator.requestedVolume < 0.001f) {
        noteOnTimeMs = 0.0f;
        noteOnAccX = sensors.accx;
        noteOnAccY = sensors.accy;
        noteOnAccZ = sensors.accz;
      }
      else {
        noteOnTimeMs += td;
      }
      float n = (NOISETIME_LENGTH - noteOnTimeMs) / NOISETIME_LENGTH;
      if (n < 0.0f) n = 0.0f;
      if (n > 1.0f) n = 1.0f;

      generator.Tick(currentNote + sensors.bendNoteShift);
    }
    unsigned long t1 = micros();
    int wait = 5;
    if (t1 > t0) {
      wait = 5-(int)((t1 - t0)/1000);
      if (wait < 1) wait = 1;
    }
    delay(wait);
  }
}

//-------------------------------------
void GetMenuParams() {
    waveInfo.lowPassP = menu.lowPassP * 0.1f;
    waveInfo.lowPassR = menu.lowPassR;
    waveInfo.lowPassQ = menu.lowPassQ * 0.1f;
    if (waveInfo.lowPassQ > 0.0f) {
      waveInfo.lowPassIDQ = 1.0f / (2.0f * waveInfo.lowPassQ);
    }
    else {
      waveInfo.lowPassIDQ = 0.0f;
    }
    waveInfo.fineTune = menu.fineTune;
    waveInfo.baseNote = 61 + menu.transpose;
    waveInfo.portamentoRate = 1 - (menu.portamentoRate * 0.01f);
    waveInfo.delayRate = menu.delayRate * 0.01f;

    attackSoftness = menu.attackSoftness * 0.01f;
    keySenseTimeMs = menu.keySense;
    sensors.breathSenseRate = menu.breathSense;
    sensors.breathZero = menu.breathZero;
    sensors.isLipSensorEnabled = menu.isLipSensorEnabled;
}


//-------------------------------------
void TickThread(void *pvParameters) {
  unsigned long loopTime = 0;
  while (1) {
    unsigned long t0 = micros();
    float td = (t0 - loopTime)/1000.0f;
    loopTime = t0;  

    //気圧センサー
    sensors.Update();
    float blow = sensors.GetBlowPower();

    generator.requestedVolume += (blow - generator.requestedVolume) * (1.0f - attackSoftness);
    //キー操作や加速度センサー
    key.UpdateKeys();
#ifdef _M5STICKC_H_
    sensors.UpdateAcc();
#endif
    sensors.BendExec(td, blow, key.IsBendKeysDown());

    float n = key.GetNoteNumber(waveInfo.baseNote);
    if (forcePlayTime > 0.0f) {
      generator.requestedVolume = 0.1f;
      forcePlayTime -= td;
    } else {
      if (targetNote != n) {
        keyTimeMs = 0.0f;
        startNote = currentNote;
        targetNote = n;
      }
    }
    if (generator.requestedVolume < 0.001f) {
      currentNote = targetNote;
      keyTimeMs = 1000.0f;
    }

    // キー押されてもしばらくは反応しない処理（ピロ音防止）
    if (keyTimeMs < 1000.0f) {
      keyTimeMs += td;
      if (keyTimeMs >= keySenseTimeMs) {
        keyTimeMs = 1000.0f;
        currentNote = targetNote;
      }
      else {
        if (noiseVolume < 0.05f) {
          noiseVolume = 0.05f; //キー切り替え中の謎のノイズ
        }
      }
    }

    unsigned long t1 = micros();
    int wait = 8;
    if (t1 > t0) {
      wait = 8-(int)((t1 - t0)/1000);
      if (wait < 1) wait = 1;
    }
    delay(wait);
  }
}

//-------------------------------------
void MenuThread(void *pvParameters) {
  while (1) {
    key.UpdateMenuKeys(menu.isEnabled);

#ifdef _M5STICKC_H_
    bool result = menu.Update(key.GetKeyPush(), pressureValue);
#endif
#ifdef _STAMPS3_H_
    bool result = menu.Update2R(&waveInfo, &key);
#endif
    if (result) {
      menu.BeginPreferences(); {
        if (menu.factoryResetRequested) {
          //出荷時状態に戻す
          M5.Lcd.fillScreen(BLACK);
          menu.DrawString("FACTORY RESET", 10, 10);
          menu.ClearAllFlash(); // CLEAR ALL FLASH MEMORY
          delay(2000);
          ESP.restart();
        }
        generator.currentWaveTable = menu.waveData.GetWaveTable(menu.waveIndex);
        menu.SavePreferences();
      } menu.EndPreferences();
    }
    if (menu.isEnabled == false && !result) {
      // perform mode
    } else {
      // menu mode or menu changed
      GetMenuParams();
      if (menu.forcePlayNote >= 0) {
        forcePlayTime = menu.forcePlayTime;
        currentNote = menu.forcePlayNote;
        Serial.printf("play note=%1.1f, time=%1.1f\n", currentNote, forcePlayTime);
        menu.forcePlayNote = -1;
      }
    }
    delay(50);
  }
}

//-------------------------------------
#if ENABLE_MIDI || ENABLE_BLE_MIDI
void MIDI_Exec() {
  int note = (int)currentNote;
  int v = (int)(generator.requestedVolume * 127);
  if (playing == false) {
    if (v > 0) {
      SetLedColor(100, 100, 100);
      AFUUEMIDI_NoteOn(note, v);
    }
  } else {
    if (v <= 0) {
        AFUUEMIDI_NoteOff();
    } else {
      if (prevNoteNumber != note) {
        AFUUEMIDI_NoteOn(note, v);
      }
      AFUUEMIDI_BreathControl(v);
      int16_t b = 0;
      if (menu.isLipSensorEnabled) {
        b = static_cast<int>(sensors.bendNoteShift * 8000);
      }
      AFUUEMIDI_PicthBendControl(b);
    }
  }
  unsigned long t = millis();
  if ((t < activeNotifyTime) || (activeNotifyTime + 200 < t)) {
    activeNotifyTime = t;
    AFUUEMIDI_ActiveNotify();
  }
}
#endif

//---------------------------------
void createWaveTask(void *pvParameters) {
  while (1) {
    uint32_t data;
    xTaskNotifyWait(0, 0, &data, portMAX_DELAY);

    generator.CreateWave(enablePlay && !menu.isMidiEnabled);
  }
}

//---------------------------------
void IRAM_ATTR onTimer(){
  portENTER_CRITICAL_ISR(&timerMux);
#ifdef SOUND_TWOPWM
  ledcWrite(0, generator.outL);
  ledcWrite(1, generator.outH);
#else
  dacWrite(DACPIN, generator.outH);
#endif

  BaseType_t higherPriorityTaskWoken = pdFALSE;
  xTaskNotifyFromISR(taskHandle, 0, eNoAction, &higherPriorityTaskWoken);
  portEXIT_CRITICAL_ISR(&timerMux);
}

//-------------------------------------
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  btStop();

  WiFi.mode(WIFI_OFF); 

#if ENABLE_MIDI || ENABLE_BLE_MIDI
  menu.isUSBMidiMounted = AFUUEMIDI_Initialize();
  if (menu.isUSBMidiMounted) {
    menu.isMidiEnabled = true;
  }
#endif

#ifdef _M5STICKC_H_
  M5.Lcd.setBrightness(255);
  M5.Lcd.setRotation(0);
  M5.Lcd.fillScreen(TFT_WHITE);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  delay(300);
  digitalWrite(LEDPIN, LOW);
  delay(300);
  digitalWrite(LEDPIN, HIGH);
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setBrightness(127);
#ifdef ENABLE_IMU
  //M5.IMU.Init();
  //M5.IMU.SetAccelFsr(M5.IMU.AFS_2G);
#endif
#endif // _M5STICKC_H_

#ifdef _STAMPS3_H_
  SetLedColor(10, 10, 10);
  delay(300);
  SetLedColor(0, 0, 0);
  delay(300);
#endif

#if ENABLE_SERIALOUTPUT
  Serial.begin(115200);
  SerialPrintLn("------");
#endif

  Wire.begin();
  SerialPrintLn("wire begin");

  int keyInitResult = key.Initialize();

  bool i2cError = false;
#ifdef _M5STICKC_H_
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(10, 10);
  DrawCenterString("Check IOExpander...", 67, 80, 2);
  if (keyInitResult < 0) {
    char s[16];
    sprintf(s, "[ NG ] %d", keyInitResult);
    DrawCenterString(s, 67, 95, 2);
    i2cError = true;
  }
#endif
  SerialPrintLn("key begin");

  DrawCenterString("Check BrthSensor...", 67, 120, 2);
  bool sensorOK = sensors.Initialize();
  if (!sensorOK) {
    DrawCenterString("[ NG ]", 67, 135, 2);
    i2cError = true;
  }

  if (i2cError) {
    for(;;) {
#ifdef _M5STICKC_H_
      digitalWrite(LEDPIN, HIGH);
      delay(200);
      digitalWrite(LEDPIN, LOW);
      delay(500);
#endif
#ifdef _STAMPS3_H_
      SetLedColor(60, 30, 0);
      delay(300);
      SetLedColor(0, 60, 30);
      delay(300);
#endif
      delay(500);
    }
  }

  SerialPrintLn("pressure sensor begin");

  menu.Initialize();

  GetMenuParams();
  SerialPrintLn("menu begin");

  generator.Initialize(menu);
  SerialPrintLn("wave generator begin");

  SerialPrintLn("setup done");

#ifdef _M5STICKC_H_
  M5.Lcd.fillScreen(TFT_BLACK);
  menu.Display();
#endif

  if (!menu.isUSBMidiMounted) {
    xQueue = xQueueCreate(QUEUE_LENGTH, sizeof(int8_t));
    xTaskCreatePinnedToCore(createWaveTask, "createWaveTask", 16384, NULL, configMAX_PRIORITIES, &taskHandle, CORE0);

    timer = timerBegin(0, CLOCK_DIVIDER, true);
    timerAttachInterrupt(timer, &onTimer, false);
    timerAlarmWrite(timer, TIMER_ALARM, true);
    timerAlarmEnable(timer);
    SerialPrintLn("timer begin");
    menu.SetTimer(timer);
  }

  xTaskCreatePinnedToCore(SynthesizerThread, "SynthesizerThread", 2048, NULL, 2, NULL, CORE0);

  xTaskCreatePinnedToCore(TickThread, "TickThread", 2048, NULL, 3, NULL, CORE0);
  xTaskCreatePinnedToCore(MenuThread, "MenuThread", 4096, NULL, 1, NULL, CORE0);

  enablePlay = true;
}

//-------------------------------------
void loop() {
  // Core0 は波形生成に専念している, loop は Core1
#ifdef _STAMPS3_H_
  int br = (int)(245 * generator.requestedVolume) + 10;
  if (!menu.isLipSensorEnabled) {
    SetLedColor(0, br, 0);
  }
  else {
      float r = -sensors.bendNoteShift;
      SetLedColor((int)(br * (1 - r)), 0, (int)(br * r));
  }
  delay(50);
#else
  delay(5000);
#endif
}

