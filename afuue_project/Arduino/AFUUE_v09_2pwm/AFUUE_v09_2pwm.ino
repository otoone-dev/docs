#include "afuue_common.h"
#include "i2c.h"
//#include "communicate.h"
#include "menu.h"
#include "wavePureSin.h"
#include "afuue_midi.h"

#ifdef _M5STICKC_H_
#include "io_expander.h"
#endif

#include <cfloat>

#if ENABLE_MIDI || ENABLE_BLE_MIDI
volatile bool midiEnabled = false;
#endif
volatile bool isUSBMidiMounted = false;

static Menu menu;
static Preferences pref;
const char* version = "AFUUE ver1.1.0.0";
const uint8_t commVer1 = 0x16;  // 1.6
const uint8_t commVer2 = 0x02;  // protocolVer = 2

hw_timer_t * timer = NULL;
#define QUEUE_LENGTH 1
QueueHandle_t xQueue;
TaskHandle_t taskHandle;
volatile bool enablePlay = false;

volatile const float* currentWaveTable = NULL;
volatile float phase = 0.0f;
volatile float currentWaveLevel = 0.0f;
#define DELAY_BUFFER_SIZE (8000)
volatile float delayBuffer[DELAY_BUFFER_SIZE];
volatile int delayPos = 0;

volatile const float* sinTable = NULL;
volatile const float* tanhTable = NULL;

int baseNote = 61; // 49 61 73
float fineTune = 440.0f;
volatile float delayRate = 0.15f;
float portamentoRate = 0.99f;
float keySenseTimeMs = 50.0f;
float breathSenseRate = 300.0f;
float breathZero = 0.0f;
const float NOTESHIFTTIME_LENGTH = 10.0f;
volatile float lowPassP = 0.1f;
volatile float lowPassR = 5.0f;
volatile float lowPassQ = 0.5f;
volatile float lowPassIDQ = 1.0f / (2 * lowPassQ);
volatile float lp_a0 = 1.0f;
volatile float lp_a1 = 0.0f;
volatile float lp_a2 = 0.0f;
volatile float lp_b0 = 0.0f;
volatile float lp_b1 = 0.0f;
volatile float lp_b2 = 0.0f;

#define CLOCK_DIVIDER (80)
#define TIMER_ALARM (40)
//#define SAMPLING_RATE (20000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*50) = 20kHz
#define SAMPLING_RATE (25000.0f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*40) = 25kHz
//#define SAMPLING_RATE (32000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (50*50) = 32kHz
//#define SAMPLING_RATE (48019.2f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (49*34) = 48kHz
static float currentWavelength = 0.0f;
volatile float currentWavelengthTickCount = 0.0f;
volatile float requestedVolume = 0.0f;
volatile float waveShift = 0.0f;
volatile uint8_t outH = 0;
volatile uint8_t outL = 0;
volatile float lowPassValue = 0.0f;
static float maximumVolume = 0.0f;
volatile float targetNote = 60.0f;
volatile float currentNote = 60.0f;
volatile float startNote = 60.0f;
static float keyTimeMs = 0.0f;
static float forcePlayTime = 0.0f;
const float FORCEPLAYTIME_LENGTH = 200.0f; //ms
volatile int defaultPressureValue = 0;
volatile int defaultPressureValue2 = 0;
volatile int pressureValue = 0;
volatile int pressureValue2 = 0;
static int ctrlMode = 0;
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

static bool keyLowC = HIGH;
static bool keyEb = HIGH;
static bool keyD = HIGH;
static bool keyE = HIGH;
static bool keyF = HIGH;
static bool keyLowCs = HIGH;
static bool keyGs = HIGH;
static bool keyG = HIGH;
static bool keyA = HIGH;
static bool keyB = HIGH;
static bool octDown = HIGH;
static bool octUp = HIGH;
#ifdef _STAMPS3_H_
static int funcDownCount = 0;
#endif

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

//---------------------------------
float InteropL(volatile const float* table, int tableCount, float p) {
  float v = (tableCount - FLT_MIN) *p;
  int t = (int)v;
  v -= t;
  float w0 = table[t];
  t = (t + 1) % tableCount;
  float w1 = table[t];
  return (w0 * (1.0f - v)) + (w1 * v);
}

//---------------------------------
float InteropC(volatile const float* table, int tableCount, float p) {
  float v = (tableCount - FLT_MIN)*p;
  int t = (int)v;
  v -= t;
  float w0 = table[t];
  t = (t + 1) % (tableCount + 1);
  float w1 = table[t];
  return (w0 * (1.0f - v)) + (w1 * v);
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

//--------------------------
float CalcFrequency(float note) {
  return fineTune * pow(2, (note - (69.0f-12.0f))/12.0f);
}

//---------------------------------
void UpdateKeys(uint16_t mcpKeys) {
#ifdef _M5STICKC_H_
  keyLowC = ((mcpKeys & 0x0001) != 0);
  keyEb = ((mcpKeys & 0x0002) != 0);
  keyD = ((mcpKeys & 0x0004) != 0);
  keyE = ((mcpKeys & 0x0008) != 0);
  keyF = ((mcpKeys & 0x0010) != 0);
  keyLowCs = ((mcpKeys & 0x0200) != 0);
  keyGs = ((mcpKeys & 0x0400) != 0);
  keyG = ((mcpKeys & 0x0800) != 0);
  keyA = ((mcpKeys & 0x1000) != 0);
  keyB = ((mcpKeys & 0x2000) != 0);
  octDown = ((mcpKeys & 0x4000) != 0);
  octUp = ((mcpKeys & 0x8000) != 0);
#else
#ifdef _STAMPS3_H_
  keyLowC = digitalRead(1);
  keyEb = digitalRead(2);
  keyD = digitalRead(3);
  keyE = digitalRead(4);
  keyF = digitalRead(5);
  keyLowCs = digitalRead(6);
  keyGs =digitalRead(7);
  keyG = digitalRead(8);
  keyA = digitalRead(9);
  keyB = digitalRead(10);
  octDown = digitalRead(44);
  octUp = digitalRead(46);
#else
  keyLowC = digitalRead(16);
  keyEb = digitalRead(17);
  keyD = digitalRead(5);
  keyE = digitalRead(18);
  keyF = digitalRead(19);
  keyLowCs = digitalRead(13);
  keyGs =digitalRead(12);
  keyG = digitalRead(14);
  keyA = digitalRead(27);
  keyB = digitalRead(26);
  octDown = digitalRead(23);
  octUp = digitalRead(3) && digitalRead(4); // before1.5:RXD0 after1.6:GPIO4
#endif
#endif
}

//---------------------------------
float GetNoteNumber() {
  int b = 0;
  if (keyLowC == LOW) b |= (1 << 7);
  if (keyD == LOW) b |= (1 << 6);
  if (keyE == LOW) b |= (1 << 5);
  if (keyF == LOW) b |= (1 << 4);
  if (keyG == LOW) b |= (1 << 3);
  if (keyA == LOW) b |= (1 << 2);
  if (keyB == LOW) b |= (1 << 1);

  float n = 0.0f;

  // 0:C 2:D 4:E 5:F 7:G 9:A 11:B 12:HiC
  int g = (b & 0b00001110);  // keyG, keyA, keyB の組み合わせ
  
  if ((g == 0b00001110) || (g == 0b00001100)) {      // [*][*][*] or [*][*][ ]
    if ((b & 0b11110000) == 0b11110000) {
      n = 0.0f; // C
      if (keyEb == LOW) n = 2.0f; // D
    }
    else if ((b & 0b11110000) == 0b01110000) {
      n = 2.0f; // D
    }
    else if ((b & 0b01110000) == 0b00110000) {
      n = 4.0f; // E
      //if (b & 0b10000000) n -= 1.0f;
    }
    else if ((b & 0b00110000) == 0b00010000) {
      n = 5.0f; // F
      if (b & 0b10000000) n -= 2.0f;
      //if (b & 0b01000000) n -= 1.0f;
    }
    else if ((b & 0b00010000) == 0) {
      n = 7.0f; // G
    if (b & 0b10000000) n -= 2.0f;
      //if (b & 0b01000000) n -= 1.0f;
      if (b & 0b00100000) n -= 1.0f;
    }
    if ((b & 0b00000010) == 0) {
      n += 12.0f;
    }
  }
  else if (g == 0b00000110) { // [ ][*][*]
    n = 9.0f; // A
    if (b & 0b10000000) n -= 2.0f;
    //if (b & 0b01000000) n -= 1.0f;
    if (b & 0b00100000) n -= 1.0f;
    if (b & 0b00010000) n += 1.0f; // SideBb 相当
  }
  else if (g == 0b00000010) { // [ ][ ][*]
    n = 11.0f; // B
    if (b & 0b10000000) n -= 2.0f;
    //if (b & 0b01000000) n -= 1.0f;
    if (b & 0b00100000) n -= 1.0f; // Flute Bb
    if (b & 0b00010000) n -= 1.0f;
  }
  else if (g == 0b00000100) { // [ ][*][ ]
    n = 12.0f; // HiC
    if (b & 0b10000000) n -= 2.0f;
    if (b & 0b01000000) n += 2.0f; // HiD トリル用
    if (b & 0b00100000) n -= 1.0f;
    if (b & 0b00010000) n -= 1.0f;
  }
  else if (g == 0b00001000) { // [*][ ][ ]
    n = 20.0f; // HiA
    if (b & 0b10000000) n -= 2.0f;
    if (b & 0b01000000) n += 2.0f;
    if (b & 0b00100000) n -= 1.0f;
    if (b & 0b00010000) n -= 1.0f;
  }
  else if (g == 0b00001010) { // [*][ ][*]
    n = 10.0f; // A#
    if (b & 0b10000000) n -= 2.0f;
    if (b & 0b01000000) n += 2.0f;
    if (b & 0b00100000) n -= 1.0f;
    if (b & 0b00010000) n -= 1.0f;
  }
  else if (g == 0b00000000) { // [ ][ ][ ]
    n = 13.0f; // HiC#
    //if (b & 0b10000000) n -= 0.5f;
    if (b & 0b01000000) n += 2.0f; // HiEb トリル用
    if (b & 0b00100000) n -= 1.0f;
    if (b & 0b00010000) n -= 1.0f;
  }

  if ((keyEb == LOW)&&(keyLowC != LOW)) n += 1.0f; // #
  if (keyGs == LOW) n += 1.0f; // #
  if (keyLowCs == LOW) n -= 1.0f; // b

  if (octUp == LOW) n += 12.0f;
  else if (octDown == LOW) n -= 12.0f;

  float bnote = baseNote - 13; // C
  return bnote + n;
}

//---------------------------------
uint16_t GetKeyData() {
  uint16_t keyData = 0;
  if (keyLowC == LOW) keyData |= (1 << 0);
  if (keyEb == LOW) keyData |= (1 << 1);
  if (keyD == LOW) keyData |= (1 << 2);
  if (keyE == LOW) keyData |= (1 << 3);
  if (keyF == LOW) keyData |= (1 << 4);
  if (keyLowCs == LOW) keyData |= (1 << 5);
  if (keyGs == LOW) keyData |= (1 << 6);
  if (keyG == LOW) keyData |= (1 << 7);
  if (keyA == LOW) keyData |= (1 << 8);
  if (keyB == LOW) keyData |= (1 << 9);
  if (octDown == LOW) keyData |= (1 << 10);
  if (octUp == LOW) keyData |= (1 << 11);
#ifdef _STAMPS3_H_
  if (digitalRead(0) == LOW) keyData |= (1 << 12);
#endif
  return keyData;
}

//---------------------------------
void BendExec(float td, float vol) {
#ifdef ENABLE_ADC2
    int bendMode = ctrlMode % 2;
    if (bendMode > 0) {
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
      if (bendMode == 1) {
        bendNoteShift += (bendNoteShiftTarget - bendNoteShift) * 0.5f;
      }
    }
#else
  // LowC + Eb down -------
  if ((keyLowC == LOW) && (keyEb == LOW)) {
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
  unsigned long mloop = micros();
  while (1) {
    unsigned long m0 = micros();
    float td = (m0 - mloop)/1000.0f;
    mloop = m0;

    // ノイズ
    if (requestedVolume < 0.001f) {
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

    // 現在の再生周波数から１サンプルあたりのフェーズの進みを計算
    float wavelength = CalcFrequency(currentNote + bendNoteShift);
    currentWavelength += (wavelength - currentWavelength) * portamentoRate;
    currentWavelengthTickCount = (currentWavelength / SAMPLING_RATE);

    // ローパスパラメータの更新
    if (lowPassQ > 0.0f) {
      float a = (tanh(lowPassR*(requestedVolume-lowPassP)) + 1.0f) * 0.5f;
      float lp = 100.0f + 20000.0f * a;
      if (lp > 12000.0f) {
        lp = 12000.0f;
      }
      lowPassValue += (lp - lowPassValue) * 0.8f;

      float omega = lowPassValue / SAMPLING_RATE;
      float alpha = InteropL(sinTable, 1024, omega) * lowPassIDQ;
      float cosv = InteropL(sinTable, 1024, omega + 0.25f);
      float one_minus_cosv = 1.0f - cosv;
      lp_a0 =  1.0f + alpha;
      lp_a1 = -2.0f * cosv;
      lp_a2 =  1.0f - alpha;
      lp_b0 = one_minus_cosv * 0.5f;
      lp_b1 = one_minus_cosv;
      lp_b2 = lp_b0;
    }

    unsigned long m1 = micros();
    int wait = 5;
    if (m1 > m0) {
      wait = 5-(int)((m1 - m0)/1000);
      if (wait < 1) wait = 1;
    }
    delay(wait);
  }
}

//-------------------------------------
void GetMenuParams() {
    lowPassP = menu.lowPassP / 10.0f;
    lowPassR = menu.lowPassR;
    lowPassQ = menu.lowPassQ / 10.0f;
    if (lowPassQ > 0.0f) {
      lowPassIDQ = 1.0f / (2.0f * lowPassQ);
    }
    else {
      lowPassIDQ = 0.0f;
    }
#ifdef _M5STICKC_H_
    fineTune = menu.fineTune;
    baseNote = 61 + menu.transpose;
    portamentoRate = 1 - (menu.portamentoRate * 0.01f);
    delayRate = menu.delayRate * 0.01f;
#endif
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
    for (int i = 0; i < PRESSURE_AVERAGE_COUNT; i++) {
#ifdef ENABLE_ADC2
      if (index == 1) {
        averaged += analogRead(ADCPIN2);
      }
      else
#endif
      {
        averaged += analogRead(ADCPIN);
      }
    }
    return averaged / PRESSURE_AVERAGE_COUNT;
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
    unsigned long t = micros();
    float td = (t - loopTime)/1000.0f;
    loopTime = t;  

    //気圧センサー
    pressureValue = GetPressureValue(0);
#ifdef ENABLE_ADC2
    //気圧センサー (ピッチベンド用)
    pressureValue2 = GetPressureValue(1);
#endif
    int defPressure = defaultPressureValue + breathZero;
#ifdef ENABLE_MCP3425
    float vol = (pressureValue - defPressure) / ((2047.0f-breathSenseRate)-defPressure); // 0 - 1
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    requestedVolume = pow(vol,2.0f) * (1.0f-bendVolume);
#else
#ifdef ENABLE_LPS33
    float vol = (pressureValue - defPressure) / 70000.0f; // 0 - 1
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    requestedVolume = pow(vol,2.0f);// * (1.0f-bendVolume);
#else
    float vol = (pressureValue - defPressure) / breathSenseRate; // 0 - 1
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    requestedVolume = pow(vol,2.0f) * (1.0f-bendVolume);
#endif
#endif
    //キー操作や加速度センサー
#ifdef _M5STICKC_H_
    uint16_t mcpKeys = readFromIOExpander();
    UpdateKeys(mcpKeys);
    UpdateAcc();
#else
    UpdateKeys(0);
    if (octDown == LOW && octUp == LOW) {
      requestedVolume = 0.7f;
    }
#endif
    BendExec(td, vol);

    float n = GetNoteNumber();
    if (forcePlayTime > 0.0f) {
      requestedVolume = 0.02f;
      forcePlayTime -= td;
    } else {
      if (targetNote != n) {
        keyTimeMs = 0.0f;
        startNote = currentNote;
        targetNote = n;
      }
    }
    if (requestedVolume < 0.001f) {
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

#if ENABLE_MIDI || ENABLE_BLE_MIDI
    if (midiEnabled) {
      MIDI_Exec();
    }
#endif
    delay(5);
  }
}

//-------------------------------------
void MenuThread(void *pvParameters) {
  uint16_t keyCurrent = 0;
  int keyRepeatCount = 0;

  while (1) {
    uint16_t keyData = GetKeyData();
    uint16_t keyPush = (keyData ^ keyCurrent) & keyData;
    if ((menu.isEnabled) && (keyData != 0) && (keyData == keyCurrent)) {
      // メニュー用キーリピート
      keyRepeatCount++;
      if (keyRepeatCount > 3) {
        if (keyRepeatCount % 2 == 0) {
          keyPush = keyData;
        }
      }
    }
    else {
      keyRepeatCount = 0;
    }
    keyCurrent = keyData;

    {
      if (menu.Update(pref, keyPush, pressureValue)) {
        BeginPreferences(); {
          if (menu.factoryResetRequested) {
            //出荷時状態に戻す
            M5.Lcd.fillScreen(BLACK);
            menu.DrawString("FACTORY RESET", 10, 10);
            pref.clear(); // CLEAR ALL FLASH MEMORY
            delay(2000);
            ESP.restart();
          }
          currentWaveTable = menu.waveData.GetWaveTable(menu.waveIndex);
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
          forcePlayTime = FORCEPLAYTIME_LENGTH;
          currentNote = menu.forcePlayNote;
          menu.forcePlayNote = -1;
        }
      }
    }
#ifdef _STAMPS3_H_
    {
      static int pgNumHigh = 0;
      static int pgNumLow = 0;
      bool keyLowC_Push = ((keyPush & (1 << 0)) != 0);
      bool keyEb_Push = ((keyPush & (1 << 1)) != 0);
      bool keyD_Push = ((keyPush & (1 << 2)) != 0);
      bool keyE_Push = ((keyPush & (1 << 3)) != 0);
      bool keyF_Push = ((keyPush & (1 << 4)) != 0);
      bool keyG_Push = ((keyPush & (1 << 7)) != 0);
      bool keyA_Push = ((keyPush & (1 << 8)) != 0);
      bool keyB_Push = ((keyPush & (1 << 9)) != 0);
      bool keyDown_Push = ((keyPush & (1 << 10)) != 0);
      bool keyUp_Push = ((keyPush & (1 << 11)) != 0);
      bool func_Push = ((keyPush & (1 << 12)) != 0);
      bool func_Down = ((keyData & (1 << 12)) != 0);

      if ((keyLowCs == LOW) && (keyGs == LOW)) {
        // Config
        if (keyF_Push) {
          // Change wave index or MIDI:Send Program Change
#if ENABLE_MIDI || ENABLE_BLE_MIDI
          if (midiEnabled) 
          {
            int pgNum = pgNumHigh * 10 + pgNumLow;
            if (pgNum > 0) {
              AFUUEMIDI_NoteOff();
              delay(500);
              AFUUEMIDI_ProgramChange(pgNum-1);
              pgNumHigh = 0;
              pgNumLow = 0;
            }
          }
          else
#endif
          {
            menu.SetNextWave();
            BeginPreferences(); {
              currentWaveTable = menu.waveData.GetWaveTable(menu.waveIndex);
              menu.SavePreferences(pref);
            } EndPreferences();
            currentNote = baseNote-1;
            forcePlayTime = FORCEPLAYTIME_LENGTH;
          }
        }
        else if (keyD_Push) {
          // Transpose--
          if (keyE == LOW) {
            baseNote = 61;
          } else {
            baseNote--;
            if (baseNote < 61-12) baseNote = 61-12;
          }
          currentNote = baseNote-1;
#if ENABLE_MIDI || ENABLE_BLE_MIDI
          if (midiEnabled) {
            AFUUEMIDI_NoteOn(currentNote, 10);
            delay(baseNote == 61 ? 400 : 200);
            AFUUEMIDI_NoteOff();
          }
          else
#endif
          {
            forcePlayTime = baseNote == 61 ? FORCEPLAYTIME_LENGTH*2 : FORCEPLAYTIME_LENGTH;
          }
        }
        else if (keyE_Push) {
          // Transpose++
          if (keyD == LOW) {
            baseNote = 61;
          } else {
            baseNote++;
            if (baseNote > 61+12) baseNote = 61+12;
          }
          currentNote = baseNote-1;
#if ENABLE_MIDI || ENABLE_BLE_MIDI
          if (midiEnabled) {
            AFUUEMIDI_NoteOn(currentNote, 10);
            delay(baseNote == 61 ? 400 : 200);
            AFUUEMIDI_NoteOff();
          }
          else
#endif
          {
            forcePlayTime = baseNote == 61 ? FORCEPLAYTIME_LENGTH*2 : FORCEPLAYTIME_LENGTH;
          }
        }
        else if (keyG_Push) {
          // LowPassQ (Resonance)
          bool ret = menu.SetNextLowPassQ();
          forcePlayTime = FORCEPLAYTIME_LENGTH;
          if (ret) {
            forcePlayTime = FORCEPLAYTIME_LENGTH * 2;
          }
          BeginPreferences(); {
            menu.SavePreferences(pref); // 記憶する
          } EndPreferences();
          GetMenuParams();
        }
        else if (keyA_Push) {
          // Breath sensitivity
          menu.breathSense -= 50;
          if (menu.breathSense < 150) {
            menu.breathSense = 350;
          }
          forcePlayTime = FORCEPLAYTIME_LENGTH;
          if (menu.breathSense == 250) {
              forcePlayTime = FORCEPLAYTIME_LENGTH * 2;
          }
          BeginPreferences(); {
            menu.SavePreferences(pref); // 記憶する
          } EndPreferences();
          GetMenuParams();
        }
        else if (keyB_Push) {
          // Change MIDI Breath Control Mode
          AFUUEMIDI_ChangeBreathControlMode();
        }
        else if (keyDown_Push) {
          // Fine Tune
          switch ((int)fineTune) {
            default:
              fineTune = 440.0f;
              break;
            case 440:
              fineTune = 442.0f;
              break;
            case 442:
              fineTune = 438.0f;
              break;
          }
          forcePlayTime = fineTune == 440 ? FORCEPLAYTIME_LENGTH*2 : FORCEPLAYTIME_LENGTH;
        }
        else if (keyUp_Push) {
          // Delay Rate
          switch ((int)(delayRate*100)) {
            default:
              delayRate = 0.15f;
              break;
            case 14:
            case 15:
              delayRate = 0.3f;
              break;
            case 29:
            case 30:
              delayRate = 0.0f;
              break;
          }
          forcePlayTime = delayRate == 0.15f ? FORCEPLAYTIME_LENGTH*2 : FORCEPLAYTIME_LENGTH;
        }
        else if (keyLowC_Push) {
          // MIDI: Add Program Number +1
          pgNumLow  = (pgNumLow + 1) % 10;
        }
        else if (keyEb_Push) {
          // MIDI: Add Program Number +10
          pgNumHigh  = (pgNumHigh + 1) % 10;
        }
      }
      else {
        pgNumHigh = 0;
        pgNumLow = 0;
      }
      if (func_Push) {
        ctrlMode = (ctrlMode + 1) % 4; // 0:normal, 1:lip-bend, 2:MIDI-normal, 3:MIDI-lip-bend
#if ENABLE_MIDI || ENABLE_BLE_MIDI
        if (midiEnabled) {
          AFUUEMIDI_NoteOff();
        }
        midiEnabled = (ctrlMode >= 2) || isUSBMidiMounted;
#endif
        currentNote = baseNote-1;
        forcePlayTime = FORCEPLAYTIME_LENGTH;
      }
      if (func_Down) {
        funcDownCount++;
        if (funcDownCount > 20*4) {
          forcePlayTime = FORCEPLAYTIME_LENGTH;
        }
        if (funcDownCount >= 20*5) {
          BeginPreferences(); {
            pref.clear(); // CLEAR ALL FLASH MEMORY
          } EndPreferences();
          ESP.restart();
        }
      }
      else {
        funcDownCount = 0;
      }
    }
#endif
    delay(50);
  }
}

//-------------------------------------
#if ENABLE_MIDI || ENABLE_BLE_MIDI
void MIDI_Exec() {
  int note = (int)currentNote;
  int v = (int)(requestedVolume * 127);
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
      if (ctrlMode%2 == 1) {
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


//-------------------------------------
//float sigmoid(float value) {
//  return 1.0f / (1.0f + exp(-value));
//}

//-------------------------------------
float LowPass(float value) { 
	static float in1  = 0.0f;
	static float in2  = 0.0f;
	static float out1 = 0.0f;
	static float out2 = 0.0f;

  float lp = (lp_b0 * value + lp_b1 * in1  + lp_b2 * in2 - lp_a1 * out1 - lp_a2 * out2) / lp_a0;

  in2  = in1;
  in1  = value;
  out2 = out1;
  out1 = lp;
  return lp;
}

//-------------------------------------
uint16_t CreateWave() {
    // 波形を生成する
    phase += currentWavelengthTickCount;
    if (phase >= 1.0f) phase -= 1.0f;

    float g = InteropL(currentWaveTable, 256, phase) / 32767.0f;

#if 0
    //ノイズ
    float n = (static_cast<float>(rand()) / RAND_MAX);
    g = (g * (1.0f-noiseVolume) + 32767.0f * n * noiseVolume);
#endif
#if 1
    if (lowPassQ > 0.0f) {
      g = LowPass(g);
    }
#endif

    float e = g * requestedVolume + delayBuffer[delayPos];;

    if ( (-0.00002f < e) && (e < 0.00002f) ) {
      e = 0.0f;
    }
    delayBuffer[delayPos] = e * delayRate;
    delayPos = (delayPos + 1) % DELAY_BUFFER_SIZE;

    e *= 32000.0f;
    if (e < -32700.0f) e = -32700.0f;
    else if (e > 32700.0f) e = 32700.0f;

    return static_cast<uint16_t>(e + 32767.0f);
}

//---------------------------------
void IRAM_ATTR onTimer(){
#ifdef SOUND_TWOPWM
      ledcWrite(0, outL);
      ledcWrite(1, outH);
#else
      dacWrite(DACPIN, outH);
#endif

  int8_t data;
  xQueueSendFromISR(xQueue, &data, 0); // キューを送信
}

//---------------------------------
void task(void *pvParameters) {
  while (1) {
    int8_t data;
    xQueueReceive(xQueue, &data, portMAX_DELAY); // キューを受信するまで待つ

    if (enablePlay && !midiEnabled) {
      uint16_t dac = CreateWave();
      outH = (dac >> 8) & 0xFF; // HHHH HHHH LLLL LLLL
      outL = dac & 0xFF;
    }
    else {
      outH = 0;
      outL = 0;
    }
  }
}

//-------------------------------------
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

#if ENABLE_MIDI || ENABLE_BLE_MIDI
  isUSBMidiMounted = AFUUEMIDI_Initialize();
  if (isUSBMidiMounted) {
    midiEnabled = true;
  }
#endif
  //bat_per_inclination = 100.0F/(bat_percent_max_vol-bat_percent_min_vol);
  //bat_per_intercept = -1 * bat_percent_min_vol * bat_per_inclination;

#ifdef _M5STICKC_H_
  M5.Lcd.setBrightness(255);
  M5.Lcd.setRotation(0);
  M5.Lcd.fillScreen(WHITE);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  delay(300);
  digitalWrite(LEDPIN, LOW);
  delay(300);
  digitalWrite(LEDPIN, HIGH);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
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
  ledcSetup(0, 156250, 8); // 156250Hz, 8Bit(256段階)
  ledcAttachPin(PWMPIN_LOW, 0);
  ledcWrite(0, 0);
  ledcSetup(1, 156250, 8); // 156250Hz, 8Bit(256段階)
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
  WiFi.mode(WIFI_OFF); 

  bool i2cError = false;

#ifdef _M5STICKC_H_
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(10, 10);
  DrawCenterString("Check IOExpander...", 67, 80, 2);
  int retIOEXP = setupIOExpander();
  if (retIOEXP < 0) {
    char s[16];
    sprintf(s, "[ NG ] %d", retIOEXP);
    DrawCenterString(s, 67, 95, 2);
    i2cError = true;
  }
#else
#ifdef _STAMPS3_H_
  const int keyPortList[14] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 ,9, 10, 44, 46 };
  for (int i = 0; i < 14; i++) {
    pinMode(keyPortList[i], INPUT_PULLUP); // INPUT FOR SWITCHES
  }
#else
  const int keyPortList[13] = { 13, 12, 14, 27, 26, 16, 17, 5,18, 19, 23, 3, 4 };
  for (int i = 0; i < 13; i++) {
    pinMode(keyPortList[i], INPUT_PULLUP); // INPUT FOR SWITCHES
  }
#endif
#endif

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

  BeginPreferences(); {
    menu.Initialize(pref);
  } EndPreferences();
  
  GetMenuParams();
  sinTable = menu.waveData.GetSinTable();
  tanhTable = menu.waveData.GetTanhTable();

  currentWaveTable = menu.waveData.GetWaveTable(menu.waveIndex);
  for (int i = 0; i < DELAY_BUFFER_SIZE; i++) {
    delayBuffer[i] = 0.0f;
  }
  SerialPrintLn("setup done");

#ifdef _M5STICKC_H_
  menu.Display();
#endif

  if (!isUSBMidiMounted) {
    xQueue = xQueueCreate(QUEUE_LENGTH, sizeof(int8_t));
    xTaskCreateUniversal(task, "createWaveTask", 16384, NULL, 3, &taskHandle, CORE0);

    timer = timerBegin(0, CLOCK_DIVIDER, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, TIMER_ALARM, true);
    timerAlarmEnable(timer);
    SerialPrintLn("timer begin");
  }

  xTaskCreatePinnedToCore(SynthesizerThread, "SynthesizerThread", 2048, NULL, 5, NULL, CORE1);

  xTaskCreatePinnedToCore(TickThread, "TickThread", 2048, NULL, 6, NULL, CORE1);
  xTaskCreatePinnedToCore(MenuThread, "MenuThread", 2048, NULL, 23, NULL, CORE1);

  enablePlay = true;
}

//-------------------------------------
void loop() {
  // Core0 は波形生成に専念している, loop は Core1
#ifdef _STAMPS3_H_
  int br = (int)(245 * requestedVolume) + 10;
  if (ctrlMode % 2 == 0) {
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

