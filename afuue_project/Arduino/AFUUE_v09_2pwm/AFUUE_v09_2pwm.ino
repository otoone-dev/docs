#include "afuue_common.h"
#include "wave_generator.h"
#include "key_system.h"
#include "i2c.h"
#include "menu.h"
#include "afuue_midi.h"

#include <cfloat>

static M5Canvas canvas(&M5.Lcd);
static Menu menu(&canvas);
static Preferences pref;

volatile WaveInfo waveInfo;
static WaveGenerator waveGenerator(&waveInfo);
static KeySystem key;

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
#define QUEUE_LENGTH 1
QueueHandle_t xQueue;
TaskHandle_t taskHandle;
volatile bool enablePlay = false;

static float maximumVolume = 0.0f;
volatile float targetNote = 60.0f;
volatile float currentNote = 60.0f;
volatile float startNote = 60.0f;
static float keyTimeMs = 0.0f;
  float keySenseTimeMs = 50.0f;
  int attackSoftness = 0;
static float forcePlayTime = 0.0f;
volatile int defaultPressureValue = 0;
volatile int defaultPressureValue2 = 0;
volatile int pressureValue = 0;
volatile int pressureValue2 = 0;
static float breathSenseRate = 300.0f;
static float breathZero = 0.0f;
static int bendCounter = 0;
const float BENDRATE = -1.0f;
const float BENDDOWNTIME_LENGTH = 100.0f; //ms
volatile float bendNoteShift = 0.0f;
volatile float bendDownTime = 0.0f;
volatile float bendVolume = 0.0f;
volatile float noiseVolume = 0.0f;
volatile float noteOnTimeMs = 0.0f;
const float noiseLevel = 0.5f;
const float NOISETIME_LENGTH = 50.0f; //ms

volatile float accx, accy, accz;
volatile float noteOnAccX, noteOnAccY, noteOnAccZ;

//-------------------------------------
void SerialPrintLn(const char* text) {
#if ENABLE_SERIALOUTPUT
  Serial.println(text);
#endif
}

//-------------------------------------
static int usePreferencesDepth = 0;
void BeginPreferences() {
  usePreferencesDepth++;
  if (usePreferencesDepth > 1) return;
  if (timer) timerAlarmDisable(timer);
  delay(10);
  pref.begin("Afuue/Settings", false);
}

void EndPreferences() {
  usePreferencesDepth--;
  if (usePreferencesDepth == 0) {
    pref.end();  
    delay(10);
    if (timer) timerAlarmEnable(timer);
  }
  if (usePreferencesDepth < 0) usePreferencesDepth = 0;
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
void UpdateAcc() {
  float ax = 0.0f;
  float ay = 0.0f;
  float az = 1.0f;
#ifdef _M5STICKC_H_
  //M5.IMU.getAccelData(&ax,&ay,&az);
#endif
  accx += (ax - accx) * 0.3f;
  accy += (ay - accy) * 0.3f;
  accz += (az - accz) * 0.3f;
}

//---------------------------------
void BendExec(float td, float vol, bool bendKeysDown) {
#ifdef ENABLE_ADC2
    if (menu.isLipSensorEnabled) {
      float b2 = (pressureValue2 - (defaultPressureValue2 + breathZero)) / breathSenseRate;
      if (b2 < 0.0f) b2 = 0.0f;
      if (b2 > vol) b2 = vol;
      float bendNoteShiftTarget = 0.0f;
      if (vol > 0.0001f) {
        bendNoteShiftTarget = -1.0f + ((vol - b2) / vol)*1.2f;
        if (bendNoteShiftTarget > 0.0f) {
          bendNoteShiftTarget = 0.0f;
        }
      }
      else {
        bendNoteShiftTarget = 0.0f;
      }
      bendNoteShift += (bendNoteShiftTarget - bendNoteShift) * 0.5f;
    }
#else
  // LowC + Eb down -------
  if (bendKeysDown) {
  //if ((keyLowC == LOW) && (keyEb == LOW)) {
    bendDownTime += td;
    if (bendDownTime > BENDDOWNTIME_LENGTH) {
      bendDownTime = BENDDOWNTIME_LENGTH;
    }
  }
  else {
    if (bendDownTime > 0.0f) {
      bendDownTime = -bendDownTime;
    }
    else {
      bendDownTime += td;
      if (bendDownTime > 0.0f) {
        bendDownTime = 0.0f;
      }
    }
  }
  float bend = 0.0f;
  if (bendDownTime > 0.0f) {
    bend = bendDownTime / BENDDOWNTIME_LENGTH;
    bendVolume = bend;
  }
  else if (bendDownTime < 0.0f) {
    bend = -(bendDownTime / BENDDOWNTIME_LENGTH);
    bendVolume = bend;
  }
  bendNoteShift = bend * BENDRATE;
#endif
}

//-------------------------------------
#if 0
void SerialThread(void *pvParameters) {
    while (1) {
      char s[32];
      sprintf(s, "%d,%d,%d,%d", GetKeyData(),(int)(accx*100), (int)(accy*100), (int)(accz*100));
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
      if (waveGenerator.requestedVolume < 0.001f) {
        noteOnTimeMs = 0.0f;
        noteOnAccX = accx;
        noteOnAccY = accy;
        noteOnAccZ = accz;
      }
      else {
        noteOnTimeMs += td;
      }
      float n = (NOISETIME_LENGTH - noteOnTimeMs) / NOISETIME_LENGTH;
      if (n < 0.0f) n = 0.0f;
      if (n > 1.0f) n = 1.0f;

      waveGenerator.Tick(currentNote + bendNoteShift);
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
    breathSenseRate = menu.breathSense;
    breathZero = menu.breathZero;
}

#ifdef ENABLE_LPS33
//-------------------------------------
// todo: アドレス違いに対応すべし
bool InitPressureLPS33(int side) {
  return BARO.begin();
}

//-------------------------------------
// todo: アドレス違いに対応すべし
int32_t GetPressureValueLPS33(int side) {
  int d = static_cast<int32_t>(BARO.readPressure() * 40960.0f);
  return d >> 1;
}
#endif

//-------------------------------------
#define PRESSURE_AVERAGE_COUNT (20)
#ifdef ENABLE_ADC
int GetPressureValueADC(int index) {
  int averaged = 0;
  int average_count = 0;
  while (average_count == 0) {
    for (int i = 0; i < PRESSURE_AVERAGE_COUNT; i++) {
#ifdef ENABLE_ADC2
      if (index == 1) {
        //averaged += analogRead(ADCPIN2);
        int value;
        if (adc2_get_raw(ADC2_CHANNEL_1, ADC_WIDTH_BIT_12, &value) == ESP_OK) {
          averaged += value;
          average_count++;
        }
      }
      else
#endif
      {
        //averaged += analogRead(ADCPIN);
        int value;
        if (adc2_get_raw(ADC2_CHANNEL_0, ADC_WIDTH_BIT_12, &value) == ESP_OK) {
          averaged += value;
          average_count++;
        }
      }
      delayMicroseconds(100);
    }
  }
  return averaged / average_count;//PRESSURE_AVERAGE_COUNT;
}
#endif

//-------------------------------------
int GetPressureValue(int index) {
#ifdef ENABLE_ADC2
  // ADC x 2
#ifdef ENABLE_LPS33
    return GetPressureValueLPS33(index);
#else
    return GetPressureValueADC(index);
#endif
#else
  // ADC x 1
#ifdef ENABLE_MCP3425
    Wire.requestFrom(MCP3425_ADDR, 2);
    return (Wire.read() << 8 ) | Wire.read();
#endif
#ifdef ENABLE_LPS33
    return GetPressureValueLPS33(0);
#endif
#ifdef ENABLE_ADC
    return GetPressureValueADC(0);
#endif
  // ----
#endif
}

//-------------------------------------
void TickThread(void *pvParameters) {
  unsigned long loopTime = 0;
  while (1) {
    unsigned long t0 = micros();
    float td = (t0 - loopTime)/1000.0f;
    loopTime = t0;  

    //気圧センサー
    pressureValue = GetPressureValue(0);
#ifdef ENABLE_ADC2
    if (menu.isLipSensorEnabled) {
      //気圧センサー (ピッチベンド用)
      pressureValue2 = GetPressureValue(1);
    }
#endif
    int defPressure = defaultPressureValue + breathZero;
#ifdef ENABLE_MCP3425
    float vol = (pressureValue - defPressure) / ((2047.0f-breathSenseRate)-defPressure); // 0 - 1
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    vol = pow(vol,2.0f) * (1.0f-bendVolume);
#else
#ifdef ENABLE_LPS33
    float vol = (pressureValue - defPressure) / 70000.0f; // 0 - 1
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    vol = pow(vol,2.0f);
#else
    float vol = (pressureValue - defPressure) / breathSenseRate; // 0 - 1
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    vol = pow(vol,2.0f);
#endif
#endif
    waveGenerator.requestedVolume += (vol - waveGenerator.requestedVolume) * (1.0f - attackSoftness);
    //キー操作や加速度センサー
#ifdef _M5STICKC_H_
    key.UpdateKeys();
    UpdateAcc();
#else
    key.UpdateKeys();
#endif
    BendExec(td, vol, key.IsBendKeysDown());

    float n = key.GetNoteNumber(waveInfo.baseNote);
    if (forcePlayTime > 0.0f) {
      waveGenerator.requestedVolume = 0.1f;
      forcePlayTime -= td;
    } else {
      if (targetNote != n) {
        keyTimeMs = 0.0f;
        startNote = currentNote;
        targetNote = n;
      }
    }
    if (waveGenerator.requestedVolume < 0.001f) {
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
    {
      key.UpdateMenuKeys(menu.isEnabled);

#ifdef _M5STICKC_H_
      bool result = menu.Update(pref, key.GetKeyPush(), pressureValue);
#endif
#ifdef _STAMPS3_H_
      bool result = menu.Update2R(pref, &waveInfo, &key);
#endif
      if (result) {
        BeginPreferences(); {
          if (menu.factoryResetRequested) {
            //出荷時状態に戻す
            M5.Lcd.fillScreen(BLACK);
            menu.DrawString("FACTORY RESET", 10, 10);
            pref.clear(); // CLEAR ALL FLASH MEMORY
            delay(2000);
            ESP.restart();
          }
          waveGenerator.currentWaveTable = menu.waveData.GetWaveTable(menu.waveIndex);
          menu.SavePreferences(pref);
        } EndPreferences();
        GetMenuParams();
      }
      if (menu.isEnabled == false) {
        // perform mode
      } else {
        // menu mode
        GetMenuParams();
        if (menu.forcePlayNote >= 0) {
          forcePlayTime = menu.forcePlayTime;
          currentNote = menu.forcePlayNote;
          Serial.printf("play note=%1.1f, time=%1.1f\n", currentNote, forcePlayTime);
          menu.forcePlayNote = -1;
        }
      }
    }
    delay(50);
  }
}

//-------------------------------------
#if ENABLE_MIDI || ENABLE_BLE_MIDI
void MIDI_Exec() {
  int note = (int)currentNote;
  int v = (int)(waveGenerator.requestedVolume * 127);
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
        b = static_cast<int>(bendNoteShift * 8000);
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

    waveGenerator.CreateWave(enablePlay && !menu.isMidiEnabled);
  }
}

//---------------------------------
void IRAM_ATTR onTimer(){
  portENTER_CRITICAL_ISR(&timerMux);
#ifdef SOUND_TWOPWM
  ledcWrite(0, waveGenerator.outL);
  ledcWrite(1, waveGenerator.outH);
#else
  dacWrite(DACPIN, waveGenerator.outH);
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

#ifdef SOUND_TWOPWM
  pinMode(PWMPIN_LOW, OUTPUT);
  pinMode(PWMPIN_HIGH, OUTPUT);
  ledcSetup(0, 156250, 8); // 156,250Hz, 8Bit(256段階)
  ledcAttachPin(PWMPIN_LOW, 0);
  ledcWrite(0, 0);
  ledcSetup(1, 156250, 8); // 156,250Hz, 8Bit(256段階)
  ledcAttachPin(PWMPIN_HIGH, 1);
  ledcWrite(1, 0);
#else
  pinMode(DACPIN, OUTPUT);
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

#ifdef ENABLE_MCP3425
  DrawCenterString("Check BrthSensor...", 67, 120, 2);
  Wire.beginTransmission(MCP3425_ADDR);
  int error = Wire.endTransmission();
  if (error != 0) {
    DrawCenterString("[ NG ]", 67, 135, 2);
    i2cError = true;
  }
  Wire.beginTransmission(MCP3425_ADDR);
  //Wire.write(0b10010111); // 0,1:Gain(1x,2x,4x,8x) 2,3:SampleRate(0:12bit,1:14bit,2:16bit) 4:Continuous 5,6:NotDefined 7:Ready
  Wire.write(0b10010011); // 0,1:Gain(1x,2x,4x,8x) 2,3:SampleRate(0:12bit,1:14bit,2:16bit) 4:Continuous 5,6:NotDefined 7:Ready
  Wire.endTransmission();
#else
#ifdef ENABLE_LPS33
  bool success = InitPressureLPS33(0);
#ifdef ENABLE_ADC2
  success &= InitPressureLPS33(1);
#endif
  if (!success) {
    i2cError = true;
  }
#else
  pinMode(ADCPIN, INPUT);
#endif
#endif

#ifdef _M5STICKC_H_
  gpio_pulldown_dis(GPIO_NUM_25);
  gpio_pullup_dis(GPIO_NUM_25);
#endif
  analogSetAttenuation(ADC_0db);
  analogReadResolution(12); // 4096
#ifdef _STAMPS3_H_
  adc2_config_channel_atten(ADC2_CHANNEL_0, ADC_ATTEN_DB_0);
  adc2_config_channel_atten(ADC2_CHANNEL_1, ADC_ATTEN_DB_0);
#endif

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

  int pressure = 0;
  int lowestPressure = 0;
#ifdef ENABLE_LPS33
  lowestPressure = 2000;
  delay(1000);
  defaultPressureValue = GetPressureValue(0) + lowestPressure;
#ifdef ENABLE_ADC2
  defaultPressureValue2 = GetPressureValue(1) + lowestPressure;
#endif
#else
  for (int i = 0; i < 10; i++) {
    pressure += GetPressureValue(0);
    delay(30);
  }
  defaultPressureValue = (pressure / 10) + lowestPressure;
#ifdef ENABLE_ADC2
  pressure = 0;
  for (int i = 0; i < 10; i++) {
    pressure += GetPressureValue(1);
    delay(30);
  }
  defaultPressureValue2 = (pressure / 10) + lowestPressure;
#endif
#endif
  SerialPrintLn("pressure sensor begin");

  BeginPreferences(); {
    menu.Initialize(pref);
  } EndPreferences();

  GetMenuParams();
  SerialPrintLn("menu begin");

  waveGenerator.Initialize(menu);
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
  int br = (int)(245 * waveGenerator.requestedVolume) + 10;
  if (!menu.isLipSensorEnabled) {
    SetLedColor(0, br, 0);
  }
  else {
      float r = -bendNoteShift;
      SetLedColor((int)(br * (1 - r)), 0, (int)(br * r));
  }
  delay(50);
#else
  delay(5000);
#endif
}

