#include "afuue_common.h"
#include "i2c.h"
//#include "communicate.h"
#include "menu.h"
#include "wavePureSin.h"
#include "afuue_midi.h"

#ifdef _M5STICKC_H_
#include "io_expander.h"
#endif

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
volatile SemaphoreHandle_t timerSemaphore;
volatile int interruptCounter = 0;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool enablePlay = false;

volatile const double* currentWaveTableA = NULL;
volatile const double* currentWaveTableB = NULL;
volatile double currentWaveShiftLevel = 0.0;
volatile double phase = 0.0;
volatile double currentWaveLevel = 0.0;
#define DELAY_BUFFER_SIZE (8000)
volatile double delayBuffer[DELAY_BUFFER_SIZE];
volatile int delayPos = 0;
#define FLANGER_BUFFER_SIZE (1250) // 25000Hz で 20Hz(E0くらい)
volatile double flangerBuffer[FLANGER_BUFFER_SIZE];
volatile double flanger = 0.0;
volatile int flangerPos = 0;
volatile float flangerTime = 0.0f;
volatile int flangerLength = 0;
volatile int fracTickCount = 0;
volatile int fracValue = 0;

int baseNote = 61; // 49 61 73
float fineTune = 440.0f;
volatile double delayRate = 0.15;
float portamentoRate = 0.99f;
float keySenseTimeMs = 50.0f;
float breathSenseRate = 300.0f;
#define ENABLE_PREPARATION (0)
float preparation = 1.2f;
const float PREPARATIONTIME_LENGTH = 10.0f;
float distortion = 0.0f;
double distortionVol = 0.0f;

#define CLOCK_DIVIDER (80)
#define TIMER_ALARM (40)
//#define SAMPLING_RATE (20000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*50) = 20kHz
#define SAMPLING_RATE (25000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*40) = 25kHz
//#define SAMPLING_TIME_LENGTH (1.0f / SAMPLING_RATE)
static float currentWavelength = 0.0f;
volatile double currentWavelengthTickCount = 0.0;
volatile double requestedVolume = 0.0;
volatile double waveShift = 0.0f;
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
volatile double noiseVolume = 0.0;
volatile float noteOnTimeMs = 0.0f;
const float noiseLevel = 0.5f;
const float NOISETIME_LENGTH = 50.0f; //ms
volatile double growlVolume = 0.0f;
volatile double growlWavelength = 0.0f;
volatile double growlTickCount = 0.0f;
volatile double growlPhase = 0.0f;
volatile double falsettoVolume = 0.0f;
volatile double modTickCount = 0.0f;
volatile double modPhase = 0.0f;

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
      float b2 = (pressureValue2 - defaultPressureValue2) / breathSenseRate;
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
      if (requestedVolume < 0.2f) {
        maximumVolume = 0.0f;
        growlVolume = 0.0f;
      }
      else if (requestedVolume > maximumVolume) {
        maximumVolume = requestedVolume;
      }
      else {
        if (maximumVolume > 0.8f) {
          float a = 0.0f; // (maximumVolume - 0.8f) * 5.0f; // 0 - 1
          if (octDown == LOW && octUp == LOW) {
            a = 1.0f;
          }
          float wavelength = 60.0 + (rand() % 20);//240.0f - 200.0f*a;
          growlWavelength += (wavelength - growlWavelength) * 0.9f;
          growlTickCount = (growlWavelength / (float)SAMPLING_RATE);
          growlVolume = a * requestedVolume;
        }
      }
    }
    float n = (NOISETIME_LENGTH - noteOnTimeMs) / NOISETIME_LENGTH;
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;

#if ENABLE_PREPARATION
    float p = (PREPARATIONTIME_LENGTH*2 - noteOnTimeMs) / (PREPARATIONTIME_LENGTH*2);
    if (p < 0.0f) p = 0.0f;
    if (p > 1.0f) p = 1.0f;
    float rnd = -((rand()%10000)/10000.0f)/10000.0f; // -1.0f - 0.0f
    float preparationShift = -p * preparation + (1.0f - p) * (rnd * 6);
    if (p == 0.0f) {
      p = (PREPARATIONTIME_LENGTH - keyTimeMs) / PREPARATIONTIME_LENGTH;
      if (p < 0.0f) p = 0.0f;
      if (p > 1.0f) p = 1.0f;
      if (currentNote < targetNote) {
        // preparation
        float d = targetNote - currentNote;
        float s = 1.0f;
        
        if (d > 12.0f) {
          s = 2.0f;
        }
        else if (d > 4.0f) {
          s = 2.0f * (d - 4.0f) / 8.0f;
        }
        preparationShift = -s*p * preparation;
      }
      else {
        // overshoot
        float d = currentNote - targetNote;
        float s = 1.0f;
        if (d > 12.0f) {
          s = 2.0f;
        }
        else if (d > 4.0f) {
          s = 1.5f;
        }
        preparationShift = s*p * preparation;
      }
    }
#endif

#if 1
    double noteOnShift = 0.0;
    if (currentWaveTableA == currentWaveTableB) {
      noiseVolume = n * requestedVolume * noiseLevel;
    }
    else {
      float p = 0.5f * (PREPARATIONTIME_LENGTH*2 - noteOnTimeMs) / (PREPARATIONTIME_LENGTH*2);
      if (p < 0.0f) p = 0.0f;
      if (p > 1.0f) p = 1.0f;
      noteOnShift = (1.0 - requestedVolume) * p;
    }
#else
    noiseVolume = 0.0;
    // ディストーション利用のアタックじゃりじゃり
    if (distortion > 0.0f) {
      distortionVol = 1.0f + n * distortion;
    }
    else {
      distortionVol = 0.0;
    }
#endif

    double ab = requestedVolume + currentWaveShiftLevel;
    if (ab < 0.0) ab = 0.0;
    if (ab > 1.0) ab = 1.0;
    waveShift = ab;

    if (menu.isAccControl) {
      //atan2f(accy, accz);
    }

    // 現在の再生周波数から１サンプルあたりのフェーズの進みを計算
    float wavelength = CalcFrequency(currentNote + bendNoteShift + noteOnShift
#if ENABLE_PREPARATION
     + preparationShift
#endif
     );
    currentWavelength += (wavelength - currentWavelength) * portamentoRate;
    currentWavelengthTickCount = (currentWavelength / (float)SAMPLING_RATE);

    modTickCount = (currentWavelength) / (float)SAMPLING_RATE;

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
#ifdef _M5STICKC_H_
    fineTune = menu.fineTune;
    baseNote = 61 + menu.transpose;
    portamentoRate = 1 - (menu.portamentoRate * 0.01f);
    delayRate = menu.delayRate * 0.01f;

    preparation = menu.preparation / 10.0f;
    //overshoot = menu.overshoot / 10.0f;
    distortion = menu.distortion;
    flanger = menu.flanger/100.0;
    flangerTime = menu.flangerTime / 100.0f;
#endif
    keySenseTimeMs = menu.keySense;
    breathSenseRate = menu.breathSense;
}

//-------------------------------------
#define PRESSURE_AVERAGE_COUNT (20)
int GetPressureValue(int index) {
#ifdef ENABLE_MCP3425
    Wire.requestFrom(MCP3425_ADDR, 2);
    return (Wire.read() << 8 ) | Wire.read();
#else
    int averaged = 0;
#ifdef ENABLE_ADC2
    if (index == 1) {
      for (int i = 0; i < PRESSURE_AVERAGE_COUNT; i++) {
        averaged += analogRead(ADCPIN2);
      }
    }
    else
#endif
    for (int i = 0; i < PRESSURE_AVERAGE_COUNT; i++) {
      averaged += analogRead(ADCPIN);
    }
    return averaged / PRESSURE_AVERAGE_COUNT;
#endif  
}

//-------------------------------------
void FlangerExec() {
    if ((flanger > 0.0) && (currentWavelengthTickCount > 0.0f)) {
      flangerLength = (int)((flangerTime * currentWavelengthTickCount) * SAMPLING_RATE);
      if (flangerLength > FLANGER_BUFFER_SIZE-2) {
        flangerLength = FLANGER_BUFFER_SIZE-2;
      }
    }
    else {
      flangerLength = 0;
    }
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
#ifdef ENABLE_MCP3425
    float vol = (pressureValue - defaultPressureValue) / ((2047.0f-breathSenseRate)-defaultPressureValue); // 0 - 1
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    requestedVolume = pow(vol,2.0f) * (1.0f-bendVolume);
#else
    float vol = (pressureValue - defaultPressureValue) / breathSenseRate; // 0 - 1
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    requestedVolume = pow(vol,2.0f) * (1.0f-bendVolume);
#endif
    //キー操作や加速度センサー
#ifdef _M5STICKC_H_
    uint16_t mcpKeys = readFromIOExpander();
    UpdateKeys(mcpKeys);
    UpdateAcc();
#else
    UpdateKeys(0);
#endif
    BendExec(td, vol);

    // ディストーション
    //if (distortion > 0.0f) {
      //distortionVol = 1.0f + distortion * vol * vol;
    //}
    //FlangerExec();

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
      if (menu.Update(keyPush, pressureValue)) {
        BeginPreferences(); {
          currentWaveTableA = menu.waveData.GetWaveTable(menu.waveIndex, 0);
          currentWaveTableB = menu.waveData.GetWaveTable(menu.waveIndex, 1);
          currentWaveShiftLevel = menu.waveData.GetWaveShiftLevel(menu.waveIndex);
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
      bool keyA_Push = ((keyPush & (1 << 8)) != 0);
      bool keyB_Push = ((keyPush & (1 << 9)) != 0);
      bool keyDown_Push = ((keyPush & (1 << 10)) != 0);
      bool keyUp_Push = ((keyPush & (1 << 11)) != 0);
      bool func_Push = ((keyPush & (1 << 12)) != 0);

      if ((keyLowCs == LOW) && (keyGs == LOW)) {
        // Config
        if (keyF_Push) {
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
              currentWaveTableA = menu.waveData.GetWaveTable(menu.waveIndex, 0);
              currentWaveTableB = menu.waveData.GetWaveTable(menu.waveIndex, 1);
              currentWaveShiftLevel = menu.waveData.GetWaveShiftLevel(menu.waveIndex);
              menu.SavePreferences(pref);
            } EndPreferences();
            currentNote = baseNote-1;
            forcePlayTime = FORCEPLAYTIME_LENGTH;
          }
        }
        else if (keyD_Push) {
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
        else if (keyA_Push) {
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
          AFUUEMIDI_ChangeBreathControlMode();
        }
        else if (keyDown_Push) {
          switch ((int)fineTune) {
            default:
              fineTune = 440;
              break;
            case 440:
              fineTune = 442;
              break;
            case 442:
              fineTune = 438;
              break;
          }
          forcePlayTime = fineTune == 440 ? FORCEPLAYTIME_LENGTH*2 : FORCEPLAYTIME_LENGTH;
        }
        else if (keyUp_Push) {
          switch ((int)(delayRate*100)) {
            default:
              delayRate = 15 * 0.01f;
              break;
            case 15:
              delayRate = 30 * 0.01f;
              break;
            case 30:
              delayRate = 0;
              break;
          }
          forcePlayTime = delayRate == 15 * 0.01f ? FORCEPLAYTIME_LENGTH*2 : FORCEPLAYTIME_LENGTH;
        }
        else if (keyLowC_Push) {
          pgNumLow  = (pgNumLow + 1) % 10;
        }
        else if (keyEb_Push) {
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
double SawWave(double p) {
  return -1.0 + 2.0 * p;
}

//---------------------------------
double Voice(double p, double shift) {
  double v = 255.99*p;
  int t = (int)v;
  //double f = (v - t);
  v -= t;
#if 0
  double w0 = currentWaveTableA[t];
  t = (t + 1) % 256;
  double w1 = currentWaveTableA[t];
  return (w0 * (1.0-v)) + (w1 * v);
#else
  double w0 = (currentWaveTableA[t] * (1.0-shift) + currentWaveTableB[t] * shift);
  t = (t + 1) % 256;
  double w1 = (currentWaveTableA[t] * (1.0-shift) + currentWaveTableB[t] * shift);
  double w = (w0 * (1.0-v)) + (w1 * v);
  return w;
#endif
}

//---------------------------------
double VoiceSin(double p) {
  double v = 255.0*p;
  int t = (int)v;
  double f = (v - t);
  return (wavePureSin[t] / 32767.0);
}

#define FRAC_DIV (4)
//-------------------------------------
uint16_t CreateWave() {
    // 波形を生成する
    phase += currentWavelengthTickCount;
    if (phase >= 1.0) phase -= 1.0;

#if 1
    double g = Voice(phase, waveShift);
    //double g1 = (Voice(phase) - currentWaveLevel) * (0.1 + 0.9 * filterLevel);
    //currentWaveLevel += g1;
    //double g = currentWaveLevel;
#else
    // FM
    modPhase += modTickCount;
    if (modPhase >= 1.0) modPhase -= 1.0;

    double p = phase + 0.1*SawWave(modPhase);
    while (p >= 1.0) p -= 1.0;
    while (p < 0.0) p += 1.0;
    double g = Voice(p);
#endif

    // がなり (オーバーシュート＋断続)
#if 1
    if (growlVolume > 0.1) {
      growlPhase += growlTickCount;
      if (growlPhase >= 1.0) growlPhase -= 1.0;
      if (growlPhase > 0.5) {
        g *= 1.0 - growlVolume;
      }
    }
#endif

    // ディストーション
#if 0
  if (distortionVol > 1.0) {
    g *= distortionVol;
    if (g > 32700.0) g = 32700.0;
    else if (g < -32700.0) g = -32700.0;
  }
#endif

#if 1
    //ノイズ
    double n = ((double)rand() / RAND_MAX);
    g = (g * (1.0-noiseVolume) + 32767.0 * n * noiseVolume);
#endif

#if 0
    // ファルセット
    if (falsettoVolume > 0.1) {
      double gs = VoiceSin(phase);
      g = g * (1.0-falsettoVolume) + gs * falsettoVolume;
    }
#endif

#if 0
    // フランジャー
    if (flanger > 0) {
      flangerBuffer[flangerPos] = g;
      flangerPos = (flangerPos + 1) % FLANGER_BUFFER_SIZE;

      g = g + flangerBuffer[(flangerPos + FLANGER_BUFFER_SIZE - flangerLength) % FLANGER_BUFFER_SIZE] * flanger;

    }
#endif

    double e = g * requestedVolume + delayBuffer[delayPos];;

    if ( (-0.00002 < e) && (e < 0.00002) ) {
      e = 0.0f;
    }
    delayBuffer[delayPos] = e * delayRate;
    delayPos = (delayPos + 1) % DELAY_BUFFER_SIZE;

    if (e < -32700.0) e = -32700.0;
    else if (e > 32700.0) e = 32700.0;

    return (uint16_t)(e + 32767.0);
}

//---------------------------------
void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  {
    if (enablePlay && !midiEnabled) {
      uint16_t dac = CreateWave();
#ifdef SOUND_TWOPWM
      ledcWrite(0, dac & 0xFF);
      ledcWrite(1, (dac >> 8) & 0xFF); // HHHH HHHH LLLL LLLL
      //ledcWrite(0, (dac >> 2) & 0xFF);
      //ledcWrite(1, (dac >> 10) & 0x3F); // HHHH HHLL LLLL 0000
#else
      dacWrite(DACPIN, dac >> 8);
#endif
    }
    interruptCounter++;
  }
  portEXIT_CRITICAL_ISR(&timerMux);
  //xSemaphoreGiveFromISR(timerSemaphore, NULL);
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
  pinMode(ADCPIN, INPUT);
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
      delay(500);
    }
  }

  int pressure = 0;
  int lowestPressure = 70;
#ifdef ENABLE_MCP3425
  lowestPressure = 50;
#endif
  for (int i = 0; i < 10; i++) {
    pressure += GetPressureValue(0);
    delay(10);
  }
  defaultPressureValue = (pressure / 10) + lowestPressure;
#ifdef ENABLE_ADC2
  pressure = 0;
  for (int i = 0; i < 10; i++) {
    pressure += GetPressureValue(1);
    delay(10);
  }
  defaultPressureValue2 = (pressure / 10) + lowestPressure;
#endif

  BeginPreferences(); {
    menu.Initialize(pref);
  } EndPreferences();
  
  GetMenuParams();

  currentWaveTableA = menu.waveData.GetWaveTable(menu.waveIndex, 0);
  currentWaveTableB = menu.waveData.GetWaveTable(menu.waveIndex, 1);
  currentWaveShiftLevel = menu.waveData.GetWaveShiftLevel(menu.waveIndex);
  for (int i = 0; i < DELAY_BUFFER_SIZE; i++) {
    delayBuffer[i] = 0.0f;
  }
  SerialPrintLn("setup done");

#ifdef _M5STICKC_H_
  menu.Display();
#endif

  if (!isUSBMidiMounted) {
    timerSemaphore = xSemaphoreCreateBinary();
    timer = timerBegin(0, CLOCK_DIVIDER, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, TIMER_ALARM, true);
    timerAlarmEnable(timer);
    SerialPrintLn("timer begin");
  }

  xTaskCreatePinnedToCore(SynthesizerThread, "SynthesizerThread", 2048, NULL, 1, NULL, CORE1);

  xTaskCreatePinnedToCore(TickThread, "TickThread", 2048, NULL, 1, NULL, CORE1);
  xTaskCreatePinnedToCore(MenuThread, "MenuThread", 2048, NULL, 3, NULL, CORE1);

  enablePlay = true;
}

//-------------------------------------
void loop() {
  // Core0 は波形生成に専念している, loop は Core1
  int br = (int)(245 * requestedVolume) + 10;
  if (ctrlMode % 2 == 0) {
    SetLedColor(0, br, 0);
  }
  else {
      double r = -bendNoteShift;
      SetLedColor((int)(br * (1 - r)), 0, (int)(br * r));
  }
  delay(50);
}

