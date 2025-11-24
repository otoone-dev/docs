#include "afuue.h"
#include "afuue_midi.h"

// タイマー割り込み用
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
#define QUEUE_LENGTH 1
TaskHandle_t taskHandle;

//---------------------------------
void Afuue::Initialize() {
#if ENABLE_MIDI
  menu.isUSBMidiMounted = afuueMidi.Initialize();
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
    xTaskCreatePinnedToCore(CreateWaveTask, "createWaveTask", 16384, this, configMAX_PRIORITIES, &taskHandle, CORE0);

    timer = timerBegin(0, CLOCK_DIVIDER, true); // Events Run On は Core1 想定
    timerAttachInterrupt(timer, &OnTimer, false);
    timerAlarmWrite(timer, TIMER_ALARM, true);
    timerAlarmEnable(timer);
    SerialPrintLn("timer begin");
    menu.SetTimer(timer);
  }

  xTaskCreatePinnedToCore(UpdateTask, "UpdateTask", 2048, this, 2, NULL, CORE0);
  xTaskCreatePinnedToCore(ControlTask, "ControlTask", 2048, this, 3, NULL, CORE0);
  xTaskCreatePinnedToCore(MenuTask, "MenuTask", 4096, this, 1, NULL, CORE0);

  enablePlay = true;
}

//-------------------------------------
void Afuue::GetMenuParams() {
  waveInfo.ApplyFromWaveSettings(menu.currentWaveSettings);

  attackNoiseLevel = menu.waveData.GetWaveAttackNoiseLevel(menu.waveIndex);
  keySenseTimeMs = menu.keySense;
  sensors.breathSenseRate = menu.breathSense;
  sensors.breathZero = menu.breathZero;
  sensors.isLipSensorEnabled = menu.isLipSensorEnabled;
}

//-------------------------------------
void Afuue::Update(float td) {
  float reqVolume = generator.requestedVolume;
#if ENABLE_MIDI
  if (menu.isMidiEnabled) {
    afuueMidi.Update((int)currentNote, reqVolume, sensors.isLipSensorEnabled, sensors.bendNoteShift + volumeDropNoteShift);
  } else
#endif
  {
    // ノイズ
    if (reqVolume < 0.001f) {
      noteOnTimeMs = 0.0f;
      noteOnAccX = sensors.accx;
      noteOnAccY = sensors.accy;
      noteOnAccZ = sensors.accz;
    }
    else {
      noteOnTimeMs += td;
    }
    float n = (ATTACKNOISETIME_LENGTH - noteOnTimeMs) / ATTACKNOISETIME_LENGTH;
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    generator.noiseVolume = n * attackNoiseLevel;

    generator.Tick(currentNote + sensors.bendNoteShift + volumeDropNoteShift, td);
  }
}

//-------------------------------------
void Afuue::Control(float td) {
  //気圧センサー
  sensors.Update();
  float blow = sensors.GetBlowPower();

  generator.requestedVolume += (blow - generator.requestedVolume) * (1.0f - waveInfo.attackSoftness);

  if (waveInfo.pitchDropPos > 0.0f && generator.requestedVolume < waveInfo.pitchDropPos) {
    // 音量によるピッチダウン
    volumeDropNoteShift = (1.0f-(generator.requestedVolume / waveInfo.pitchDropPos)) * waveInfo.pitchDropLevel;
  }
  else {
    volumeDropNoteShift = 0.0f;
  }

  //キー操作や加速度センサー
  key.UpdateKeys();
  sensors.UpdateAcc();
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
  }
}

//-------------------------------------
void Afuue::MenuExec() {
  key.UpdateMenuKeys(menu.isEnabled);

  bool result;
  if (HasDisplay()) {
    result = menu.Update(key.GetKeyPush(), sensors.GetPressureValue(0));
  }
  else {
    result = menu.Update2R(&waveInfo, &key);
  }
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

  if (HasLED()) {
    int br = (int)(245 * generator.requestedVolume) + 10;
    if (!menu.isLipSensorEnabled) {
      SetLedColor(0, br, 0);
    }
    else {
        float r = -sensors.bendNoteShift;
        SetLedColor((int)(br * (1 - r)), 0, (int)(br * r));
    }
  }
}

//-------------------------------------
#if 0
void Afuue::SerialTask(void *pvParameters) {
  Afuue* system = reinterpret_cast<Afuue*>(pvParameters);
  KeySystem& key = system->GetKey();
  Sensors& sensors = system->GetSensors();

  while (1) {
    char s[32];
    sprintf(s, "%d,%d,%d,%d", key.GetKeyData(),(int)(sensors.accx*100), (int)(sensors.accy*100), (int)(sensors.accz*100));
    Serial.println(s);
    delay(50);
  }
}
#endif

//-------------------------------------
void Afuue::UpdateTask(void *pvParameters) {
  Afuue* system = reinterpret_cast<Afuue*>(pvParameters);

  unsigned long loopTime = micros();
  while (1) {
    unsigned long t0 = micros();
    float td = (t0 - loopTime)/1000.0f;
    loopTime = t0;

    system->Update(td);
    
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
void Afuue::ControlTask(void *pvParameters) {
  Afuue* system = reinterpret_cast<Afuue*>(pvParameters);

  unsigned long loopTime = 0;
  while (1) {
    unsigned long t0 = micros();
    float td = (t0 - loopTime)/1000.0f;
    loopTime = t0;  

    system->Control(td);

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
void Afuue::MenuTask(void *pvParameters) {
  Afuue* system = reinterpret_cast<Afuue*>(pvParameters);
  while (1) {
    system->MenuExec();
    delay(50);
  }
}

//-------------------------------------
void Afuue::CreateWaveTask(void *pvParameters) {
  Afuue* system = reinterpret_cast<Afuue*>(pvParameters);
  WaveGenerator& generator = system->GetGenerator();
  while (1) {
    uint32_t data;
    xTaskNotifyWait(0, 0, &data, portMAX_DELAY);
    generator.CreateWave(system->IsEnablePlay() && !system->GetMenu().isMidiEnabled);
  }
}

//---------------------------------
void IRAM_ATTR Afuue::OnTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
#ifdef SOUND_TWOPWM
  ledcWrite(0, waveOutL);
  ledcWrite(1, waveOutH);
#else
  ledcWrite(0, waveOutL);
  ledcWrite(1, waveOutH);
#endif

  BaseType_t higherPriorityTaskWoken = pdFALSE;
  xTaskNotifyFromISR(taskHandle, 0, eNoAction, &higherPriorityTaskWoken);
  portEXIT_CRITICAL_ISR(&timerMux);
}
