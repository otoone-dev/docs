#include "afuue_common.h"
#include "i2c.h"
//#include "midi.h"
//#include "communicate.h"
#include "menu.h"
#include "wavePureSin.h"
#ifdef _M5STICKC_H_
#include "io_expander.h"
#endif

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

volatile const short* currentWaveTable = NULL;
volatile double phase = 0.0;
#define DELAY_BUFFER_SIZE (8000)
volatile double delayBuffer[DELAY_BUFFER_SIZE];
volatile int delayPos = 0;
volatile int fracTickCount = 0;
volatile int fracValue = 0;

int baseNote = 61; // 49 61 73
float fineTune = 440.0f;
volatile double delayRate = 0.15;
float portamentoRate = 0.99f;
float keySenseTimeMs = 50.0f;
float breathSenseRate = 300.0f;

#define CLOCK_DIVIDER (80)
#define TIMER_ALARM (40)
#define SAMPLING_RATE (25000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*40) = 25kHz // 80MHz / (80*50) = 20kHz
//#define SAMPLING_TIME_LENGTH (1.0f / SAMPLING_RATE)
static float currentWavelength = 0.0f;
volatile double currentWavelengthTickCount = 0.0;
volatile double requestedVolume = 0.0;
volatile float targetNote = 60.0f;
volatile float currentNote = 60.0f;
volatile float startNote = 60.0f;
static float keyTimeMs = 0.0f;
//const float noteStep = 0.12f;
static float forcePlayTime = 0.0f;
const float FORCEPLAYTIME_LENGTH = 200.0f; //ms
volatile int defaultPressureValue = 0;
volatile int pressureValue = 0;
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
volatile float growlVolume = 0.0f;
volatile float growlWavelength = 0.0f;
volatile float growlTickCount = 0.0f;
volatile float growlPhase = 0.0f;
volatile float falsettoVolume = 0.0f;
volatile float modTickCount = 0.0f;
volatile float modPhase = 0.0f;

static bool keyLowC;
static bool keyEb;
static bool keyD;
static bool keyE;
static bool keyF;
static bool keyLowCs;
static bool keyGs;
static bool keyG;
static bool keyA;
static bool keyB;
static bool octDown;
static bool octUp;

volatile float accx, accy, accz;

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
void DrawCenterString(const char* str, int x, int y, int fontId) {
  M5.Lcd.drawCentreString(str, x, y, fontId);
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
      //if (b & 0b10000000) n -= 0.5f;
      //if (b & 0b01000000) n -= 1.0f;
    }
    else if ((b & 0b00010000) == 0) {
      n = 7.0f; // G
      //if (b & 0b10000000) n -= 0.5f;
      //if (b & 0b01000000) n -= 1.0f;
      if (b & 0b00100000) n -= 1.0f;
    }
    if ((b & 0b00000010) == 0) {
      n += 12.0f;
    }
  }
  else if (g == 0b00000110) { // [ ][*][*]
    n = 9.0f; // A
    //if (b & 0b10000000) n -= 0.5f;
    //if (b & 0b01000000) n -= 1.0f;
    if (b & 0b00100000) n -= 1.0f;
    if (b & 0b00010000) n -= 1.0f;
  }
  else if (g == 0b00000010) { // [ ][ ][*]
    n = 11.0f; // B
    //if (b & 0b10000000) n -= 0.5f;
    //if (b & 0b01000000) n -= 1.0f;
    if (b & 0b00100000) n -= 1.0f;
    if (b & 0b00010000) n -= 1.0f;
  }
  else if (g == 0b00000100) { // [ ][*][ ]
    n = 12.0f; // HiC
    //if (b & 0b10000000) n -= 0.5f;
    //if (b & 0b01000000) n -= 1.0f;
    if (b & 0b00100000) n -= 1.0f;
    if (b & 0b00010000) n -= 1.0f;
  }
  else if (g == 0b00001000) { // [*][ ][ ]
    n = 20.0f; // HiA
    //if (b & 0b10000000) n -= 0.5f;
    //if (b & 0b01000000) n -= 1.0f;
    if (b & 0b00100000) n -= 1.0f;
    if (b & 0b00010000) n -= 1.0f;
  }
  else if (g == 0b00001010) { // [*][ ][*]
    n = 10.0f; // A#
    if (b & 0b10000000) n -= 0.5f;
    if (b & 0b01000000) n -= 1.0f;
    if (b & 0b00100000) n -= 1.0f;
    if (b & 0b00010000) n -= 1.0f;
  }
  else if (g == 0b00000000) { // [ ][ ][ ]
    n = 13.0f; // HiC#
    if (b & 0b10000000) n -= 0.5f;
    if (b & 0b01000000) n -= 1.0f;
    if (b & 0b00100000) n -= 1.0f;
    if (b & 0b00010000) n -= 1.0f;
  }

  if ((keyEb == LOW)&&(keyLowC != LOW)) n += 1.0f; // #
  if (keyGs == LOW) n += 1.0f; // #
  if (keyLowCs == LOW) n -= 1.0f; // b

  if (octDown == LOW) n -= 12.0f;
  if (octUp == LOW) n += 12.0f;

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
  return keyData;
}

//---------------------------------
void BendExec(float td) {
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
    if (requestedVolume < 0.01f) {
      noteOnTimeMs = 0.0f;
    }
    else {
      noteOnTimeMs += td;
    }
    float n = (NOISETIME_LENGTH - noteOnTimeMs) / NOISETIME_LENGTH;
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    noiseVolume = n * requestedVolume * noiseLevel;

    float portamento = portamentoRate;

    // がなり (傾きによるもの)
    if (menu.isAccControl) {
      float a = accy;
      if (accy > 0.5f) {
        falsettoVolume = 0.0f;
        a = (a - 0.5f)*2.0f;
        if (a > 1.0f) a = 1.0f;
        float wavelength = 240.0f - 200.0f*a;
        growlWavelength += (wavelength - growlWavelength) * 0.9f;
        growlTickCount = (growlWavelength / (float)SAMPLING_RATE);
        growlVolume = a;
      }
      else if (accy < -0.1f) {
        growlVolume = 0.0f;
        if (a < -1.0f) a = -1.0f;
        falsettoVolume = -a;
      }
      else {
        growlVolume = 0.0f;
        falsettoVolume = 0.0f;
      }
  
      // 左右傾きによるポルタメント制御
      if ((accx > 0.5f) || (accx < -0.5f)) {
        portamento = 0.1f;
      }
    }

    // キー押されてもしばらくは反応しない処理（ピロ音防止）
    if (keyTimeMs < 1000.0f) {
      keyTimeMs += td;
      if (keyTimeMs >= keySenseTimeMs) {
        keyTimeMs = 1000.0f;
        currentNote = targetNote;
      }
      else {
        if (noiseVolume < 0.1f) {
          noiseVolume = 0.1f; //キー切り替え中の謎のノイズ
        }
      }
    }

    // 現在の再生周波数から１サンプルあたりのフェーズの進みを計算
    float wavelength = CalcFrequency(currentNote + bendNoteShift);
    currentWavelength += (wavelength - currentWavelength) * portamento;
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
    fineTune = menu.fineTune;
    baseNote = 61 + menu.transpose;
    portamentoRate = 1 - (menu.portamentoRate * 0.01f);
    delayRate = menu.delayRate * 0.01f;
    keySenseTimeMs = menu.keySense;
    breathSenseRate = menu.breathSense;
}

//-------------------------------------
#define PRESSURE_AVERAGE_COUNT (3)
int GetPressureValue() {
#if ENABLE_MCP3425
    Wire.requestFrom(MCP3425_ADDR, 2);
    return (Wire.read() << 8 ) | Wire.read();
#else
    int averaged = 0;
    for (int i = 0; i < PRESSURE_AVERAGE_COUNT; i++) {
      averaged += analogRead(ADCPIN);
    }
    return averaged / PRESSURE_AVERAGE_COUNT;
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
    pressureValue = GetPressureValue();
#if ENABLE_MCP3425
    float vol = (pressureValue - defaultPressureValue) / (2047.0f-defaultPressureValue); // 0 - 1
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
    uint16_t mcpKeys = readFromIOExpander();

    UpdateKeys(mcpKeys);
    UpdateAcc();

    BendExec(td);
  
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
    if (menu.Update(keyPush, pressureValue)) {
      BeginPreferences(); {
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
    delay(50);
  }
}

//-------------------------------------
double SawWave(double p) {
  return -1.0 + 2.0 * p;
}

//---------------------------------
double Voice(double p) {
  double v = 255.99*p;
  int t = (int)v;
  //double f = (v - t);
  v -= t;
#if 1
  double w0 = (double)currentWaveTable[t];
  t = (t + 1) % 256;
  double w1 = (double)currentWaveTable[t];
  return (w0 * (1.0-v)) + (w1 * v);
#else
  double w0 = (waveA[t] * (1.0-shift) + waveB[t] * shift);
  t = (t + 1) % 256;
  double w1 = (waveA[t] * (1.0-shift) + waveB[t] * shift);
  double w = (w0 * (1.0-f)) + (w1 * f);
  return (w / 32767.0);
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
    double g = Voice(phase);

#if 0
    // FM
    modPhase += modTickCount;
    if (modPhase >= 1.0) modPhase -= 1.0;

    double p = phase + 0.1*SawWave(modPhase);
    while (p >= 1.0) p -= 1.0;
    while (p < 0.0) p += 1.0;
    double g = Voice(p);
#endif

    // がなり (オーバーシュート＋断続)
#if 0
    if (growlVolume > 0.1) {
      growlPhase += growlTickCount;
      if (growlPhase >= 1.0) growlPhase -= 1.0;
      g *= (1.0 + growlVolume);
      if (g > 2.0) g = 2.0;
      if (g < -2.0) g = -2.0;
      if (g > 1.0) g = 2.0 - g;
      if (g < -1.0) g = -2.0 - g;
      double gn = 1.0;
      if (growlPhase > 0.5) gn = 1.0 - growlVolume;
      //g *= 0.5 + 0.5 * tsin(phase[2]) * channelVolume[2] + 0.5 * (1.0 - channelVolume[2]);
      g *= gn;
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
      g = (g * (1.0-falsettoVolume) + gs * falsettoVolume);
    }
#endif

    double e = g * requestedVolume + delayBuffer[delayPos];;

    if ( (-0.0002 < e) && (e < 0.0002) ) {
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
    if (enablePlay) {
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
  //bat_per_inclination = 100.0F/(bat_percent_max_vol-bat_percent_min_vol);
  //bat_per_intercept = -1 * bat_percent_min_vol * bat_per_inclination;

#ifdef _M5STICKC_H_
  auto cfg = M5.config();
  M5.begin(cfg);
  //M5.begin();
  //M5.Axp.ScreenBreath(15);
  M5.Lcd.setBrightness(255);
  //M5.Lcd.setRotation(0);
  M5.Lcd.setRotation(0);
  //M5.Lcd.fillScreen(WHITE);
  M5.Lcd.fillScreen(WHITE);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  delay(300);
  digitalWrite(LEDPIN, LOW);
  delay(300);
  digitalWrite(LEDPIN, HIGH);
  //M5.Lcd.fillScreen(BLACK);
  M5.Lcd.fillScreen(BLACK);
  //M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
  //M5.Lcd.setTextSize(1);
  M5.Lcd.setTextSize(1);
  //M5.Axp.ScreenBreath(8);
  M5.Lcd.setBrightness(127);

#ifdef ENABLE_IMU
  //M5.IMU.Init();
  //M5.IMU.SetAccelFsr(M5.IMU.AFS_2G);
#endif
#endif

#ifdef SOUND_TWOPWM
  pinMode(PWMPIN_LOW, OUTPUT);
  pinMode(PWMPIN_HIGH, OUTPUT);
  ledcSetup(0, 156250, 8); // 156250Hz, 8Bit(256段階)
  //ledcSetup(0, 312500, 7); // 312500Hz, 7Bit(128段階)
  //ledcSetup(0, 625000, 6); // 625000Hz, 6Bit(64段階)
  ledcAttachPin(PWMPIN_LOW, 0);
  ledcWrite(0, 0);
  ledcSetup(1, 156250, 8); // 156250Hz, 8Bit(256段階)
  //ledcSetup(1, 312500, 7); // 312500Hz, 7Bit(128段階)
  //ledcSetup(1, 625000, 6); // 625000Hz, 6Bit(64段階)
  ledcAttachPin(PWMPIN_HIGH, 1);
  ledcWrite(1, 0);
#else
  pinMode(DACPIN, OUTPUT);
#endif
#ifdef ENABLE_SPEAKERHAT
  digitalWrite(0, HIGH);
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
  const int keyPortList[13] = { 13, 12, 14, 27, 26, 16, 17, 5,18, 19, 23, 3, 4 };
  for (int i = 0; i < 13; i++) {
    pinMode(keyPortList[i], INPUT_PULLUP); // INPUT FOR SWITCHES
  }
#endif

#if ENABLE_MCP3425
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
#elif ENABLE_BMP180
  DrawCenterString("Check BMP180...", 67, 120, 2);
  bool retBMP180 = CheckBMP180();
  if (retBMP180 == false) {
    DrawCenterString("[ NG ]", 67, 135, 2);
    i2cError = true;
  }
#else
  pinMode(ADCPIN, INPUT);
  gpio_pulldown_dis(GPIO_NUM_25);
  gpio_pullup_dis(GPIO_NUM_25);
  analogSetAttenuation(ADC_0db);
#endif

  if (i2cError) {
    for(;;) {
      digitalWrite(LEDPIN, HIGH);
      delay(200);
      digitalWrite(LEDPIN, LOW);
      delay(500);
    }
  }
  delay(500);

  int pressure = 0;
  for (int i = 0; i < 10; i++) {
    pressure += GetPressureValue();
    delay(10);
  }
  defaultPressureValue = (pressure / 10) + 30;

  BeginPreferences(); {
    menu.Initialize(pref);
  } EndPreferences();
  
  GetMenuParams();

  currentWaveTable = menu.waveData.GetWaveTable(menu.waveIndex);
  for (int i = 0; i < DELAY_BUFFER_SIZE; i++) {
    delayBuffer[i] = 0.0f;
  }

  delay(100);

  SerialPrintLn("setup done");

#ifdef _M5STICKC_H_
  menu.Display();
#endif

  timerSemaphore = xSemaphoreCreateBinary();
  timer = timerBegin(0, CLOCK_DIVIDER, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, TIMER_ALARM, true);
  timerAlarmEnable(timer);
  SerialPrintLn("timer begin");

  xTaskCreatePinnedToCore(SynthesizerThread, "SynthesizerThread", 4096, NULL, 1, NULL, CORE1);
  xTaskCreatePinnedToCore(TickThread, "TickThread", 4096, NULL, 1, NULL, CORE1);
  xTaskCreatePinnedToCore(MenuThread, "MenuThread", 4096, NULL, 3, NULL, CORE1);

  enablePlay = true;
}

//-------------------------------------
void loop() {
  // Core0 は波形生成に専念している
    char s[64];
#if 0
  sprintf(s, "%1.2f", (float)requestedVolume);
  DrawCenterString(s, 67, 120, 2);
#endif
#if 0
    float volt = pressur * 2.048f / 8192.0f / 8.0f;
    sprintf(s, "%1.3fv / %d", volt, pressur);
    Serial.println(s);
  delay(200);
#endif
#if 0
  sprintf(s, "%1.1fms", mcp3425time);
  DrawCenterString(s, 67, 120, 2);
  delay(200);
#endif

  delay(5000);
}

