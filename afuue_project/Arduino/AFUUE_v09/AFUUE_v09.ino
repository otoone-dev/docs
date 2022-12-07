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

int baseNote = 61; // 49 61 73
double fineTune = 440.0;
volatile double delayRate = 0.15;
double portamentoRate = 0.99;
double keySenseTimeMs = 50.0;
double breathSenseRate = 300.0;

#define CLOCK_DIVIDER (80)
#define TIMER_ALARM (40)
#define SAMPLING_RATE (25000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*40) = 25kHz // 80MHz / (80*50) = 20kHz
//#define SAMPLING_TIME_LENGTH (1.0 / SAMPLING_RATE)
static double currentWavelength = 0.0;
volatile double currentWavelengthTickCount = 0.0;
volatile double requestedVolume = 0.0;
volatile double targetNote = 60;
volatile double currentNote = 60;
volatile double startNote = 60;
static double keyTimeMs = 0;
//const double noteStep = 0.12;
static double forcePlayTime = 0;
const double FORCEPLAYTIME_LENGTH = 200; //ms
static int bendCounter = 0;
const double BENDRATE = -1.0;
const double BENDDOWNTIME_LENGTH = 100; //ms
volatile double bendNoteShift = 0.0;
volatile double bendDownTime = 0.0;
volatile double bendVolume = 0.0;
volatile double noiseVolume = 0.0;
volatile double noteOnTimeMs = 0.0;
const double noiseLevel = 0.5;
const double NOISETIME_LENGTH = 50; //ms
volatile double growlVolume = 0;
volatile double growlWavelength = 0;
volatile double growlTickCount = 0;
volatile double growlPhase = 0;
volatile double falsettoVolume = 0;

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
  float ax = 0;
  float ay = 0;
  float az = 1;
#ifdef _M5STICKC_H_
  M5.IMU.getAccelData(&ax,&ay,&az);
#endif
  accx += (ax - accx) * 0.3f;
  accy += (ay - accy) * 0.3f;
  accz += (az - accz) * 0.3f;
}

//--------------------------
double CalcFrequency(double note) {
  return fineTune * pow(2, (note - (69-12))/12);
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
double GetNoteNumber() {
  int b = 0;
  if (keyLowC == LOW) b |= (1 << 7);
  if (keyD == LOW) b |= (1 << 6);
  if (keyE == LOW) b |= (1 << 5);
  if (keyF == LOW) b |= (1 << 4);
  if (keyG == LOW) b |= (1 << 3);
  if (keyA == LOW) b |= (1 << 2);
  if (keyB == LOW) b |= (1 << 1);

  double n = 0;

  // 0:C 2:D 4:E 5:F 7:G 9:A 11:B 12:HiC
  int g = (b & 0b00001110);  // keyG, keyA, keyB の組み合わせ
  
  if ((g == 0b00001110) || (g == 0b00001100)) {      // [*][*][*] or [*][*][ ]
    if ((b & 0b11110000) == 0b11110000) {
      n = 0.0; // C
      if (keyEb == LOW) n = 2.0; // D
    }
    else if ((b & 0b11110000) == 0b01110000) {
      n = 2.0; // D
    }
    else if ((b & 0b01110000) == 0b00110000) {
      n = 4.0; // E
      //if (b & 0b10000000) n -= 1.0;
    }
    else if ((b & 0b00110000) == 0b00010000) {
      n = 5.0; // F
      //if (b & 0b10000000) n -= 0.5;
      //if (b & 0b01000000) n -= 1.0;
    }
    else if ((b & 0b00010000) == 0) {
      n = 7.0; // G
      //if (b & 0b10000000) n -= 0.5;
      //if (b & 0b01000000) n -= 1.0;
      if (b & 0b00100000) n -= 1.0;
    }
    if ((b & 0b00000010) == 0) {
      n += 12.0;
    }
  }
  else if (g == 0b00000110) { // [ ][*][*]
    n = 9.0; // A
    //if (b & 0b10000000) n -= 0.5;
    //if (b & 0b01000000) n -= 1.0;
    if (b & 0b00100000) n -= 1.0;
    if (b & 0b00010000) n -= 1.0;
  }
  else if (g == 0b00000010) { // [ ][ ][*]
    n = 11.0; // B
    //if (b & 0b10000000) n -= 0.5;
    //if (b & 0b01000000) n -= 1.0;
    if (b & 0b00100000) n -= 1.0;
    if (b & 0b00010000) n -= 1.0;
  }
  else if (g == 0b00000100) { // [ ][*][ ]
    n = 12.0; // HiC
    //if (b & 0b10000000) n -= 0.5;
    //if (b & 0b01000000) n -= 1.0;
    if (b & 0b00100000) n -= 1.0;
    if (b & 0b00010000) n -= 1.0;
  }
  else if (g == 0b00001000) { // [*][ ][ ]
    n = 20.0; // HiA
    //if (b & 0b10000000) n -= 0.5;
    //if (b & 0b01000000) n -= 1.0;
    if (b & 0b00100000) n -= 1.0;
    if (b & 0b00010000) n -= 1.0;
  }
  else if (g == 0b00001010) { // [*][ ][*]
    n = 10.0; // A#
    if (b & 0b10000000) n -= 0.5;
    if (b & 0b01000000) n -= 1.0;
    if (b & 0b00100000) n -= 1.0;
    if (b & 0b00010000) n -= 1.0;
  }
  else if (g == 0b00000000) { // [ ][ ][ ]
    n = 13.0; // HiC#
    if (b & 0b10000000) n -= 0.5;
    if (b & 0b01000000) n -= 1.0;
    if (b & 0b00100000) n -= 1.0;
    if (b & 0b00010000) n -= 1.0;
  }

  if ((keyEb == LOW)&&(keyLowC != LOW)) n += 1.0; // #
  if (keyGs == LOW) n += 1.0; // #
  if (keyLowCs == LOW) n -= 1.0; // b

  if (octDown == LOW) n -= 12;
  if (octUp == LOW) n += 12;

  double bnote = baseNote - 13; // C
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
void BendExec(double td) {
  // LowC + Eb down -------
  if ((keyLowC == LOW) && (keyEb == LOW)) {
    bendDownTime += td;
    if (bendDownTime > BENDDOWNTIME_LENGTH) {
      bendDownTime = BENDDOWNTIME_LENGTH;
    }
  }
  else {
    if (bendDownTime > 0.0) {
      bendDownTime = -bendDownTime;
    }
    else {
      bendDownTime += td;
      if (bendDownTime > 0.0) {
        bendDownTime = 0.0;
      }
    }
  }
  double bend = 0.0;
  if (bendDownTime > 0) {
    bend = bendDownTime / BENDDOWNTIME_LENGTH;
    bendVolume = bend;
  }
  else if (bendDownTime < 0) {
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
    double td = (m0 - mloop)/1000.0;
    mloop = m0;

    // ノイズ
    if (requestedVolume < 0.01) {
      noteOnTimeMs = 0.0;
    }
    else {
      noteOnTimeMs += td;
    }
    double n = (NOISETIME_LENGTH - noteOnTimeMs) / NOISETIME_LENGTH;
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    noiseVolume = n * requestedVolume * noiseLevel;

    double portamento = portamentoRate;

    // がなり (傾きによるもの)
    if (menu.isAccControl) {
      double a = accy;
      if (accy > 0.5f) {
        falsettoVolume = 0.0;
        a = (a - 0.5)*2;
        if (a > 1.0) a = 1.0;
        float wavelength = 240.0f - 200.0f*a;
        growlWavelength += (wavelength - growlWavelength) * 0.9;
        growlTickCount = (growlWavelength / (double)SAMPLING_RATE);
        growlVolume = a;
      }
      else if (accy < -0.1f) {
        growlVolume = 0.0;
        if (a < -1.0) a = -1.0;
        falsettoVolume = -a;
      }
      else {
        growlVolume = 0.0;
        falsettoVolume = 0.0;
      }
  
      // 左右傾きによるポルタメント制御
      if ((accx > 0.5) || (accx < -0.5)) {
        portamento = 0.1;
      }
    }

    // キー押されてもしばらくは反応しない処理（ピロ音防止）
    if (keyTimeMs < 1000.0) {
      keyTimeMs += td;
      if (keyTimeMs >= keySenseTimeMs) {
        keyTimeMs = 1000.0;
        currentNote = targetNote;
      }
      else {
        if (noiseVolume < 0.1) {
          noiseVolume = 0.1; //キー切り替え中の謎のノイズ
        }
      }
    }

    // 現在の再生周波数から１サンプルあたりのフェーズの進みを計算
    double wavelength = CalcFrequency(currentNote + bendNoteShift);
    currentWavelength += (wavelength - currentWavelength) * portamento;
    currentWavelengthTickCount = (currentWavelength / (double)SAMPLING_RATE);

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
    portamentoRate = 1 - (menu.portamentoRate * 0.01);
    delayRate = menu.delayRate * 0.01;
    keySenseTimeMs = menu.keySense;
    breathSenseRate = menu.breathSense;
}

//-------------------------------------
void TickThread(void *pvParameters) {
  unsigned long loopTime = 0;
  uint16_t keyCurrent = 0;
  int keyRepeatCount = 0;
  while (1) {
    unsigned long t = micros();
    double td = (t - loopTime)/1000.0;
    loopTime = t;  

    //気圧センサー
    int averaged = 0;
    for (int i = 0; i < 3; i++) {
      averaged += analogRead(36);
      delay(1);
    }
    averaged /= 3;
    double vol = (averaged - 460) / breathSenseRate; // 0 - 1
    if (vol < 0.0) vol = 0.0;
    if (vol > 1.0) vol = 1.0;
    requestedVolume = pow(vol,2) * (1-bendVolume);
  
    //キー操作や加速度センサー
    uint16_t mcpKeys = readFromIOExpander();
    UpdateKeys(mcpKeys);
    UpdateAcc();

    uint16_t keyData = GetKeyData();
    uint16_t keyPush = (keyData ^ keyCurrent) & keyData;
    if ((menu.isEnabled) && (keyData != 0) && (keyData == keyCurrent)) {
      // メニュー用キーリピート
      keyRepeatCount++;
      if (keyRepeatCount > 100) {
        if (keyRepeatCount % 2 == 0) {
          keyPush = keyData;
        }
      }
    }
    else {
      keyRepeatCount = 0;
    }
    keyCurrent = keyData;
    if (menu.Update(keyPush, averaged)) {
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

    BendExec(td);
  
    double n = GetNoteNumber();
    if (forcePlayTime > 0) {
      requestedVolume = 0.02;
      forcePlayTime -= td;
    } else {
      if (targetNote != n) {
        keyTimeMs = 0.0;
        startNote = currentNote;
        targetNote = n;
      }
    }
    if (requestedVolume < 0.001) {
      currentNote = targetNote;
      keyTimeMs = 1000.0;
    }
    delay(5);
  }
}

//-------------------------------------
double SawWave(double p) {
  return -1.0 + 2 * p;
}

//---------------------------------
double Voice(double p) {
  double v = 255*p;
  int t = (int)v;
  double f = (v - t);
#if 1
  double w0 = currentWaveTable[t];
  t = (t + 1) % 256;
  double w1 = currentWaveTable[t];
  double w = (w0 * (1-f)) + (w1 * f);
  return (w / 32767.0);
  //return (currentWaveTable[t] / 32767.0);
#else
  double w0 = (waveA[t] * (1-shift) + waveB[t] * shift);
  t = (t + 1) % 256;
  double w1 = (waveA[t] * (1-shift) + waveB[t] * shift);
  double w = (w0 * (1-f)) + (w1 * f);
  return (w / 32767.0);
#endif
}

//---------------------------------
double VoiceSin(double p) {
  double v = 255*p;
  int t = (int)v;
  double f = (v - t);
  return (wavePureSin[t] / 32767.0);
}

//-------------------------------------
uint8_t CreateWave() {
    // 波形を生成する
    phase += currentWavelengthTickCount;
    if (phase >= 1.0) phase -= 1.0;
    double g = Voice(phase);

    // がなり (オーバーシュート＋断続)
    if (growlVolume > 0.1) {
      growlPhase += growlTickCount;
      if (growlPhase >= 1.0) growlPhase -= 1.0;
      g *= (1.0 + growlVolume);
      if (g > 2) g = 2;
      if (g < -2) g = -2;
      if (g > 1) g = 2 - g;
      if (g < -1) g = -2 - g;
      double gn = 1;
      if (growlPhase > 0.5) gn = 1 - growlVolume;
      //g *= 0.5 + 0.5 * tsin(phase[2]) * channelVolume[2] + 0.5 * (1.0 - channelVolume[2]);
      g *= gn;
    }

    //ノイズ
    double n = ((double)rand() / RAND_MAX);
    g = (g * (1-noiseVolume) + n * noiseVolume);

    // ファルセット
    if (falsettoVolume > 0.1) {
      double gs = VoiceSin(phase);
      g = (g * (1-falsettoVolume) + gs * falsettoVolume);
    }

    double e = 150 * g * requestedVolume + delayBuffer[delayPos];;

    if ( (-0.005 < e) && (e < 0.005) ) {
      e = 0.0;
    }
    delayBuffer[delayPos] = e * delayRate;
    delayPos = (delayPos + 1) % DELAY_BUFFER_SIZE;

    if (e < -127.0) e = -127.0;
    else if (e > 127.0) e = 127.0;

    uint8_t dac = (uint8_t)(e + 127);
    return dac;
}

//---------------------------------
void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  {
    if (enablePlay) {
      uint8_t dac = CreateWave();
      dacWrite(DACPIN, dac);
    }
    interruptCounter++;
  }
  portEXIT_CRITICAL_ISR(&timerMux);
  //xSemaphoreGiveFromISR(timerSemaphore, NULL);
}

//-------------------------------------
void setup() {
  pinMode(DACPIN, OUTPUT);
#ifdef ENABLE_SPEAKERHAT
  digitalWrite(0, HIGH);
#endif

  //bat_per_inclination = 100.0F/(bat_percent_max_vol-bat_percent_min_vol);
  //bat_per_intercept = -1 * bat_percent_min_vol * bat_per_inclination;

#ifdef _M5STICKC_H_
  M5.begin();
  M5.Axp.ScreenBreath(15);
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
  M5.Axp.ScreenBreath(8);

#ifdef ENABLE_IMU
  M5.IMU.Init();
  M5.IMU.SetAccelFsr(M5.IMU.AFS_2G);
#endif
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

#if ENABLE_BMP180
  DrawCenterString("Check BMP180...", 67, 120, 2);
  bool retBMP180 = CheckBMP180();
  if (retBMP180 == false) {
    DrawCenterString("[ NG ]", 67, 135, 2);
    i2cError = true;
  }
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

  BeginPreferences(); {
    menu.Initialize(pref);
  } EndPreferences();
  
  GetMenuParams();

  currentWaveTable = menu.waveData.GetWaveTable(menu.waveIndex);
  for (int i = 0; i < DELAY_BUFFER_SIZE; i++) {
    delayBuffer[i] = 0.0;
  }

  delay(100);

  pinMode(36, INPUT);
  gpio_pulldown_dis(GPIO_NUM_25);
  gpio_pullup_dis(GPIO_NUM_25);
  analogSetAttenuation(ADC_0db);

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

  enablePlay = true;
}

//-------------------------------------
void loop() {
  // Core0 は波形生成に専念している
  delay(5000);
}


/*

struct ToneSetting {
  int8_t transpose = 0;
  int8_t fineTune = 0;
  int8_t reverb = 0;
  int8_t portamento = 0;
  int8_t keySense = 0;
  int8_t breathSense = 0;
  int8_t padd0 = 0;
  int8_t padd1 = 0;
  int16_t waveA[256];
  int16_t waveB[256];
  uint8_t shiftTable[32];
  uint8_t noiseTable[32];
  uint8_t pitchTable[32];
};

static ToneSetting toneSettings[5];

static bool testMode = false;
static char _error;

hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
volatile int interruptCounter = 0;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool enablePlay = false;

static uint8_t forcePlayTime = 50;

#define MIN_PRESSURE (80)
#define MAX_PRESSURE (450)
volatile uint32_t normalPressure = 0;
volatile double currentPressure = 0.0;
volatile uint32_t rawPressure = 0;
#define TEMPERATURE_PRESSURE_RATE (-0.59f)
volatile uint32_t normalTemperature = 0;
volatile int16_t currentTemperature = 0;
volatile int16_t rawTemperature = 0;

volatile uint16_t mcpKeys = 0;

static double currentVolume = 0.0;
volatile double channelVolume[3] = {1.0, 0.0, 0.0 };
volatile double requestedVolume = 0.0;
volatile double maxVolume = 0.0;
#if USE_SPEAKERHAT
volatile double masterVolume = 0.2;
#endif

#if ENABLE_IMU
float accx, accy, accz;
#endif

bool lipSensorEnabled = false;

volatile double shift = 0.0;
volatile double noise = 0.0;
volatile double pitch = 0.0;

#define CLOCK_DIVIDER (80)
#define TIMER_ALARM (50)
#define SAMPLING_RATE (20000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*50) = 20kHz
#define SAMPLING_TIME_LENGTH (1.0 / SAMPLING_RATE)
static double currentWavelength[3] = { 0.0, 0.0, 0.0 };
volatile double currentWavelengthTickCount[3] = { 0.0, 0.0, 0.0 };

static double currentNote[2] = { 60, 64 };
static double startNote[2] = { 60, 64 };
static double targetNote[2] = { 60, 64 };
static double noteTime = 0;
static double noteStep = 0.5;
static double keyChangeCurve = 12;
static int bendCounter = 0;
const int BENDCOUNT_MAX = 7;
const double BENDRATE = -1.0;
int baseNote = 0; // extern
static int bootBaseNote = 0;
static int toneNo = -1;
static double fineTune = 440.0;
static double reverbRate = 0.152;
static double portamentoRate = 0.99;
static double volumeRate = 0.5;

volatile short waveA[256];
volatile short waveB[256];
volatile uint8_t shiftTable[32];
volatile uint8_t noiseTable[32];
volatile uint8_t pitchTable[32];

volatile double phase[3] = { 0.0, 0.0, 0.0 };
volatile double mphase = 0.0;
volatile double noteOnTimeLength = 0.0;
#define REVERB_BUFFER_SIZE (8801)
volatile double reverbBuffer[REVERB_BUFFER_SIZE];
volatile int reverbPos = 0;

volatile bool metronome = false;
volatile uint8_t metronome_m = 1;
volatile unsigned long metronome_tm = 0;
volatile int metronome_t = 90;
volatile uint8_t metronome_v = 10;
volatile int metronome_cnt = 0;

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

void SerialPrintLn(char* text) {
#if ENABLE_SERIALOUTPUT
  Serial.println(text);
#endif
}

#if ENABLE_MIDI
static uint8_t sendBuffer[600];
static uint8_t receiveBuffer[1024];
static int receivePos = 0;
static bool waitForCommand = true;
void SerialSend(int sendSize) {
  if (midiEnabled) {
    Serial.write(sendBuffer, sendSize);
  }
}
#endif

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


#define BMP180_ADDR 0x77 // 7-bit address
#define BMP180_REG_CONTROL 0xF4
#define BMP180_REG_RESULT 0xF6
#define BMP180_COMMAND_TEMPERATURE 0x2E
#define BMP180_COMMAND_PRESSURE0 0x34
#define BMP180_COMMAND_PRESSURE1 0x74
#define BMP180_COMMAND_PRESSURE2 0xB4
#define BMP180_COMMAND_PRESSURE3 0xF4

//---------------------------------
char ReadBytes(uint8_t addr, uint8_t* values, uint8_t length)
// Read an array of bytes from device
// values: external array to hold data. Put starting register in values[0].
// length: number of bytes to read
{
  char x;

  Wire.beginTransmission(addr);
  Wire.write(values[0]);
  _error = Wire.endTransmission();
  if (_error == 0)
  {
    Wire.requestFrom(addr,length);
    while(Wire.available() != length) ; // wait until bytes are ready
    for(x=0;x<length;x++)
    {
      values[x] = Wire.read();
    }
    return(1);
  }
  return(0);
}

//---------------------------------
char ReadInt(char address, int16_t &value)
{
  unsigned char data[2];

  data[0] = address;
  if (ReadBytes(BMP180_ADDR, data,2))
  {
    value = (int16_t)((data[0]<<8)|data[1]);
    //if (*value & 0x8000) *value |= 0xFFFF0000; // sign extend if negative
    return(1);
  }
  value = 0;
  return(0);
}
char readUInt(char address, uint16_t &value) {
  unsigned char data[2];

  data[0] = address;
  if (ReadBytes(BMP180_ADDR, data,2))
  {
    value = (uint16_t)((data[0]<<8)|data[1]);
    //if (*value & 0x8000) *value |= 0xFFFF0000; // sign extend if negative
    return(1);
  }
  value = 0;
  return(0);
}

//---------------------------------
char WriteBytes(unsigned char addr, unsigned char *values, char length)
{
  char x;
  
  Wire.beginTransmission(addr);
  Wire.write(values,length);
  _error = Wire.endTransmission();
  if (_error == 0)
    return(1);
  else
    return(0);
}

//---------------------------------
bool CheckBMP180() {
  Wire.beginTransmission(BMP180_ADDR);
  int error = Wire.endTransmission();
  return (error == 0);
}

//---------------------------------
uint16_t GetPressure() {  // 16bit unsigned
  unsigned char wdata[2] = { BMP180_REG_CONTROL, BMP180_COMMAND_PRESSURE0 };
  if (WriteBytes(BMP180_ADDR, wdata, 2)) {
    delay(5);
    unsigned char rdata[3] = { BMP180_REG_RESULT, 0, 0 };
    if (ReadBytes(BMP180_ADDR, rdata, 3)) {
      return (((uint16_t)rdata[0]) << 8) | (uint16_t)rdata[1];
    }
  }
  return 0;
}

//---------------------------------
int16_t GetTemperature() {  // 16bit singed
  unsigned char wdata[2] = { BMP180_REG_CONTROL, BMP180_COMMAND_TEMPERATURE };
  if (WriteBytes(BMP180_ADDR, wdata, 2)) {
    delay(5);
    unsigned char rdata[2] = { BMP180_REG_RESULT, 0 };
    if (ReadBytes(BMP180_ADDR, rdata, 2)) {
      return (int16_t)((((uint16_t)rdata[0]) << 8) | (uint16_t)rdata[1]);
    }
  }
  return 0;
}

#if ENABLE_ADXL345
//---------------------------------
void ReadXYZ(double* x, double* y, double* z) {
  unsigned char _buff[8];
    _buff[0] = ADXL345_DATAX0;
    ReadBytes(ADXL345_DEVICE, _buff, 6);
    short sx = (short)((((unsigned short)_buff[1]) << 8) | _buff[0]);
    short sy = (short)((((unsigned short)_buff[3]) << 8) | _buff[2]);
    short sz = (short)((((unsigned short)_buff[5]) << 8) | _buff[4]);
    *x = ADXL345_RESO * (double)sx;
    *y = ADXL345_RESO * (double)sy;
    *z = ADXL345_RESO * (double)sz;
}
#endif

//----------------------------------
const double bat_percent_max_vol = 4.1;     // バッテリー残量の最大電圧
const double bat_percent_min_vol = 3.3;     // バッテリー残量の最小電圧
static double bat_per_inclination = 0;        // バッテリー残量のパーセンテージ式の傾き
static double bat_per_intercept   = 0;        // バッテリー残量のパーセンテージ式の切片

void drawBattery(int x, int y) {
  double bat_vol = M5.Axp.GetVbatData() * 1.1 / 1000;   // V
  double bat_per = bat_per_inclination * bat_vol + bat_per_intercept;    // %
  if(bat_per > 100.0){
      bat_per = 100.0;
  }
  //M5.Lcd.setCursor(10, 100);
  //M5.Lcd.printf("%1.1f V / %3.1f%%_", bat_vol, bat_per);

  // battery
  M5.Lcd.fillRect(x, y+5, 3, 5, WHITE);
  M5.Lcd.fillRect(x+2, y, 24, 15, WHITE);
  
  M5.Lcd.fillRect(x+4, y+2, 20, 11, BLACK);
  if (bat_per > 75) {
    M5.Lcd.fillRect(x+6, y+4, 4, 7, WHITE);
  }
  if (bat_per > 50) {
    M5.Lcd.fillRect(x+12, y+4, 4, 7, WHITE);
  }
  if (bat_per > 25) {
    M5.Lcd.fillRect(x+18, y+4, 4, 7, WHITE);
  }
}

void updateAcc() {
  float ax, ay, az;
  M5.IMU.getAccelData(&ax,&ay,&az);
  ay = asin(ay) / (1.571 - 0.1);
  if (ay > 0.1) {
    ay = ay - 0.1;
  }
  else if (ay < -0.1) {
    ay = ay + 0.1;
  }
  else {
    ay = 0.0;
  }
  accx += (ax - accx) * 0.1;
  accy += (ay - accy) * 0.1;
  accz += (az - accz) * 0.1;
  accx = ax;
  accy = ay;
  accz = az;
#if 0
  M5.Lcd.setCursor(10, 10);
  char s[32];
  sprintf(s, "%d, %d, %d", (int)(accx*100), (int)(accy*100), (int)(accz*100));
  M5.Lcd.println(s);
#endif
}

//---------------------------------
#if 1
double GetNoteNumber(bool chordFlag) {
  int b = 0;
  if (keyLowC == LOW) b |= (1 << 7);
  if (keyD == LOW) b |= (1 << 6);
  if (keyE == LOW) b |= (1 << 5);
  if (keyF == LOW) b |= (1 << 4);
  if (keyG == LOW) b |= (1 << 3);
  if (keyA == LOW) b |= (1 << 2);
  if (keyB == LOW) b |= (1 << 1);

  double n = 0;

  // 0:C 2:D 4:E 5:F 7:G 9:A 11:B 12:HiC
  int g = (b & 0b00001110);  // keyG, keyA, keyB の組み合わせ
  
  if ((g == 0b00001110) || (g == 0b00001100)) {      // [*][*][*] or [*][*][ ]
    if ((b & 0b11110000) == 0b11110000) {
      n = 0.0; // C
    }
    else if ((b & 0b11110000) == 0b01110000) {
      n = 2.0; // D
    }
    else if ((b & 0b01110000) == 0b00110000) {
      n = 4.0; // E
      if (b & 0b10000000) n -= 1.0;
    }
    else if ((b & 0b00110000) == 0b00010000) {
      n = 5.0; // F
      if (b & 0b10000000) n -= 0.5;
      if (b & 0b01000000) n -= 1.0;
    }
    else if ((b & 0b00010000) == 0) {
      n = 7.0; // G
      if (b & 0b10000000) n -= 0.5;
      if (b & 0b01000000) n -= 1.0;
      if (b & 0b00100000) n -= 1.0;
    }
    if ((b & 0b00000010) == 0) {
      n += 12.0;
    }
  }
  else if (g == 0b00000110) { // [ ][*][*]
    n = 9.0; // A
    if (b & 0b10000000) n -= 0.5;
    if (b & 0b01000000) n -= 1.0;
    if (b & 0b00100000) n -= 1.0;
    if (b & 0b00010000) n -= 1.0;
  }
  else if (g == 0b00000010) { // [ ][ ][*]
    n = 11.0; // B
    if (b & 0b10000000) n -= 0.5;
    if (b & 0b01000000) n -= 1.0;
    if (b & 0b00100000) n -= 1.0;
    if (b & 0b00010000) n -= 1.0;
  }
  else if (g == 0b00000100) { // [ ][*][ ]
    n = 12.0; // HiC
    if (b & 0b10000000) n -= 0.5;
    if (b & 0b01000000) n -= 1.0;
    if (b & 0b00100000) n -= 1.0;
    if (b & 0b00010000) n -= 1.0;
  }
  else if (g == 0b00001000) { // [*][ ][ ]
    n = 20.0; // HiA
    if (b & 0b10000000) n -= 0.5;
    if (b & 0b01000000) n -= 1.0;
    if (b & 0b00100000) n -= 1.0;
    if (b & 0b00010000) n -= 1.0;
  }
  else if (g == 0b00001010) { // [*][ ][*]
    n = 10.0; // A#
    if (b & 0b10000000) n -= 0.5;
    if (b & 0b01000000) n -= 1.0;
    if (b & 0b00100000) n -= 1.0;
    if (b & 0b00010000) n -= 1.0;
  }
  else if (g == 0b00000000) { // [ ][ ][ ]
    n = 13.0; // HiC#
    if (b & 0b10000000) n -= 0.5;
    if (b & 0b01000000) n -= 1.0;
    if (b & 0b00100000) n -= 1.0;
    if (b & 0b00010000) n -= 1.0;
  }

  if (keyEb == LOW) n += 1.0; // #
  if (keyGs == LOW) n += 1.0; // #
  if (keyLowCs == LOW) n -= 1.0; // b

  if (chordFlag == false) {
    if (octDown == LOW) n -= 12;
    if (octUp == LOW) n += 12;
  }

  double bnote = baseNote - 13; // C
  return bnote + n;
}

#else
//---------------------------------
int GetNoteNumber(bool chordFlag) {
  (void)chordFlag;
  
  int bnote = 0;
  if (keyA == LOW) bnote = 1;
  if (keyB == LOW) bnote |= 2;   // 0x00:C#, 0x01:C, 0x02:B, (0x03:A)
  if (bnote == 3) bnote = 4;

  int note = baseNote - bnote;
  
  if ((bnote == 1) && (keyG == LOW)) note += 9;  // +12-3
  if (keyG == LOW) note -= 2;
  if (keyGs == LOW) note++;
  if (keyF == LOW) {
    note -= 2;
  }
  if (keyE == LOW) note--;
  if (keyD == LOW) note -= 2;
  if (keyEb == LOW) note++;
  if (keyLowC == LOW) note -= 2;
  if (keyLowCs == LOW) note--;

  if (octDown == LOW) note -= 12;
  if (octUp == LOW) note += 12;
  
  return note;
}
#endif


//---------------------------------
double BendExec() {
  // LowC + Eb down -------
  if ((keyLowC == LOW) && (keyEb == LOW)) {
    if (bendCounter < BENDCOUNT_MAX) {
      bendCounter++;
    }
  }
  else {
    if (bendCounter > 0) {
      bendCounter--;    
    }
  }
  return bendCounter / (double)BENDCOUNT_MAX;
}

//---------------------------------
void GetChord(int& n0, int& n1, int& n2) {
  double note = GetNoteNumber(true);

  n0 = ((int)(note - (baseNote - 1 -4))+24) % 12;
  n1 = (n0 + 4) % 12;
  n2 = (n0 + 7) % 12;

  //if (keyGs == LOW) n0 -= 2;     // 7th
  //if (keyLowCs == LOW) n1--;  // Minor
  if ((octDown == LOW) && (octUp == HIGH)) n1 += 11; // Minor
  if ((octDown == HIGH) && (octUp == LOW)) {
    n1 += 11;
    n2 += 11; // dim
  }
  if ((octDown == LOW) && (octUp == LOW)) n1++; // sus4

  n0 = (n0 % 12) - 12;
  n1 = (n1 % 12) - 12;
  n2 = (n2 % 12) - 12;

  n0 += (int)(baseNote-1 -4);
  n1 += (int)(baseNote-1 -4);
  n2 += (int)(baseNote-1 -4);
}

//---------------------------------
bool IsAnyKeyDown() {
  return (GetKeyData() != 0);
}

//--------------------------
double CalcFrequency(double note) {
  return fineTune * pow(2, (note - (69-12))/12);
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
void ConfigExec() {
  if (M5.BtnA.pressedFor(100)) {
    toneNo = (toneNo + 1) % 5;
    enablePlay = false;
    delay(100);
    normalPressure = rawPressure;
    delay(100);
    normalTemperature = rawTemperature;
    BeginPreferences(); {
      pref.putInt("ToneNo", toneNo);
      LoadToneSetting(toneNo);
    } EndPreferences();
    enablePlay = true;
    forcePlayTime = 10;

    M5.Axp.ScreenBreath(15);
    delay(1000);
    M5.Axp.ScreenBreath(8);
    while (1) {
      delay(100);
      M5.update();
      if (M5.BtnA.isReleased()) break;
    }
  }
#if USE_SPEAKERHAT
  if (M5.BtnB.pressedFor(100)) {
    masterVolume += 0.2;
    if (masterVolume >= 1.0) {
      masterVolume = 0.2;
    }
    forcePlayTime = 10;
    while (1) {
      delay(100);
      M5.update();
      if (M5.BtnB.isReleased()) break;
    }
  }
#endif
  
  static uint16_t keyCurrent = 0;
  static int32_t testModeCnt = 0;
  uint16_t keyData = GetKeyData();

  uint16_t keyPush = (keyData ^ keyCurrent) & keyData;
  keyCurrent = keyData;

  // # + b key down -------
  if ((keyData & (1<<5))&&(keyData & (1<<6))) {
    if ((octUp == LOW) && (octDown == LOW)) {
      if (testModeCnt++ > 100) {
        testMode = true;
      }
    } else {
      testModeCnt = 0;
    }

    ToneSetting* ts = &toneSettings[toneNo];
    if (keyPush & (1<<0)) {
      ts->reverb = ((ts->reverb / 3)+1) * 3;  // 0, 3, 6, 9
      if (ts->reverb > 10) {
        ts->reverb = 0;
      }
      reverbRate = ts->reverb * 0.02; // (default 0.152)
      forcePlayTime = 10;
    }
    else if (keyPush & (1<<4)) {
      if (toneNo >= 4) {
        toneNo = 0;
      } else {
        toneNo++;
      }
      enablePlay = false;
      delay(100);
      normalPressure = rawPressure;
      delay(100);
      normalTemperature = rawTemperature;
      BeginPreferences(); {
        pref.putInt("ToneNo", toneNo);
        LoadToneSetting(toneNo);
      } EndPreferences();
      enablePlay = true;
      forcePlayTime = 10;
    }
    else if (keyPush & (1<<9)) {
      if (metronome == false) {
        metronome = true;
        metronome_m = 1;
        metronome_v = 10;
        metronome_cnt = 0;
      }
      else {
        switch (metronome_v) {
          case 10:
            metronome_v = 20;
            break;
          case 20:
            metronome_v = 30;
            break;
          case 30:
            metronome_v = 40;
            break;
          case 40:
            metronome_m++;
            //if (metronome_m >= 5) {
            if (metronome_m >= 2) {
              metronome_m = 1;
              metronome = false;
            }
            break;
        }
        metronome_cnt = 0;
      }
    }
    else if ((keyData & (1<<8)) && (keyData & (1<<7))) {
      metronome_t = 90;
      metronome_cnt = 0;
    }
    else if (keyPush & (1<<8)) {
      metronome_t += 5;
      metronome_cnt = 0;
      if (metronome_t > 360) metronome_t = 360;
    }
    else if (keyPush & (1<<7)) {
      metronome_t -= 5;
      metronome_cnt = 0;
      if (metronome_t < 40) metronome_t = 40;
    }
    else if (keyPush & (1<<3)) {
      if (keyData & (1<<2)) {
        baseNote = 61;
        forcePlayTime = 10;
      }
      else {
        baseNote ++; // 49 61 73
        if (baseNote > 73) baseNote = 73;
        forcePlayTime = 2;
        if ((baseNote == 49) || (baseNote == 61) || (baseNote == 73)) {
          forcePlayTime = 10;
        }
      }
    }
    if (keyPush & (1<<2)) {
      if (keyData & (1<<3)) {
        baseNote = 61;
        forcePlayTime = 10;
      } else {
        baseNote --; // 49 61 73
        if (baseNote < 49) baseNote = 49;
        forcePlayTime = 2;
        if ((baseNote == 49) || (baseNote == 61) || (baseNote == 73)) {
          forcePlayTime = 10;
        }
      }
    }
  }
}

//---------------------------------
void UpdateKeys() {
  M5.update();
#if ENABLE_MCP23017
  //uint16_t data = readMCP23017();
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
void Tick() {
  UpdateKeys();

  double pp = currentPressure;  // calculated by another thread.
#if ENABLE_ADXL345
  {
    double x, y, z;
    ReadXYZ(&x, &y, &z);
    //pp = 1 + y; // play by accellero meter
  }
#endif
  ConfigExec();

  if (pp < 0.0) pp = 0.0;
  if (pp > 1.0) pp = 1.0;
  if (forcePlayTime > 0) {
    forcePlayTime--;
    pp = 0.2;
    currentNote[0] = baseNote - 1;
    currentNote[1] = currentNote[0] + 4;
    for (int i = 0; i < 2; i++) {
      targetNote[i] = currentNote[i];
    }
    noteTime = 1;
    
  } else {
    
    int n0;
    int n1;
    n0 = GetNoteNumber(false);
    n1 = n0;
    if (testMode) {
      // play by pushing keys
      if (IsAnyKeyDown()) {
        pp = 0.8;
        n0 = 60;
        if (keyEb == LOW) n0 = 63;
        if (keyD == LOW) n0 = 62;
        if (keyE == LOW) n0 = 64;
        if (keyF == LOW) n0 = 65;
        if (keyLowCs == LOW) n0 = 61;
        if (keyGs == LOW) n0 = 68;
        if (keyG == LOW) n0 = 67;
        if (keyA == LOW) n0 = 69;
        if (keyB == LOW) n0 = 71;
        if (octDown == LOW) n0 -= 12;
        if (octUp == LOW) n0 += 12;
        n1 = n0;
        for (int i = 0; i < 2; i++) {
          startNote[i] = n0;
          currentNote[i] = n0;
          targetNote[i] = n0;
        }
      }
    }

    if ((targetNote[0] != n0) || (targetNote[1] != n1)) {
      //double d = abs(note - currentNote);
      noteTime = 0.0;
      for (int i = 0; i < 2; i++) {
        startNote[i] = currentNote[i];
      }
      targetNote[0] = n0;
      targetNote[1] = n1;
    }
    else if (pp < 0.01) {
      for (int i = 0; i < 2; i++) {
        currentNote[i] = targetNote[i];
      }
      noteTime = 1.0;
    }
    else {
      if (noteTime >= 1.0) {
        
      } else {
        noteTime += noteStep;
        if (noteTime > 1.0) {
          noteTime = 1.0;
        }
        double t = pow(noteTime, keyChangeCurve);
        for (int i = 0; i < 2; i++) {
          currentNote[i] = startNote[i] + (targetNote[i] - startNote[i]) * t;
        }
      }
    } 
  }

  double bend = BendExec();
  
  double ppReq = pp*pp * (1-bend);

  if (ppReq > currentVolume) {
    currentVolume += (ppReq - currentVolume) * volumeRate;
  } else {
    currentVolume += (ppReq - currentVolume) * 0.8;
  }
  requestedVolume = currentVolume;

  if (midiEnabled) {
    if (deviceConnected) {
      int note = (int)targetNote[0];
      int v = (int)(currentVolume * 127);
      if (playing == false) {
        if (v > 0) {
          MIDI_NoteOn(note, v);
        }
      } else {
        if (v == 0) {
            MIDI_NoteOff();
        } else {
          if ((prevNoteNumber != note) && (noteTime > 0.8)) {
            MIDI_ChangeNote(note, v);
  
          } else {
            MIDI_BreathControl(v);
          }
        }
      }
      unsigned long t = millis();
      if ((t < activeNotifyTime) || (activeNotifyTime + 200 < t)) {
        activeNotifyTime = t;
        MIDI_ActiveNotify();
      } else {
//        delay(1);
      }
    } else {
      delay(1);
    }
    return;
  }

#if 0
  {
    // 時間によるシフト
    double k = noteOnTimeLength / 10.0;
    if (k > 1) k = 1;
    shift = 1 - k;
  }
#endif

#if 1
  {
    // 音量によるシフト
    double u = currentVolume * 30.99;
    int t = (int)(u);
    double f = u - t;
    if (t < 0) t = 0;
    if (t > 30) t = 30;

    double k0 = shiftTable[t] / 127.0;
    double k1 = shiftTable[t + 1] / 127.0;
    shift = (k0 * (1.0 - f) + k1 * f);

    double n0 = noiseTable[t] / 127.0;
    double n1 = noiseTable[t + 1] / 127.0;
    noise = (n0 * (1.0 - f) + n1 * f);

    double p0 = pitchTable[t] / 127.0;
    double p1 = pitchTable[t + 1] / 127.0;
    pitch = ((p0 * (1.0 - f) + p1 * f)) - 0.5;
  }
#endif

  // ベンド (キー操作によるもの)
  double bendNoteShift = bend * BENDRATE;
#if ENABLE_ANALOGREAD_GPIO32
  if (lipSensorEnabled) {
    // ベンド (感圧センサーによるもの)
    int d = (4095 - analogRead(32));
    if (d > 1000) d = 1000;
    double a = (d / 1000.0);
    static double ana = 0.0;
    ana += (a - ana)*0.5;
    a = BENDRATE * (1-ana);
    bendNoteShift += a;
  }
#endif
#if 0
  // ベンド (傾きにによるもの)
  if (accx > 1) {
    bendNoteShift -= (accx - 1.0);
  }
  if (accx < -1) {
    bendNoteShift += (accx + 1.0);
  }
#endif
  // ベンド (音量によるもの)
  double bv = -(0.5 - currentVolume) * 0.02;
  bendNoteShift += bv;

#if 0
  // ファルセット
  if (accy < -0.5) {
    float a = -accy;
    if (a > 1) a = 1;
    a = (a - 0.5)*2; // 0 - 1
    if (a < 0) a = 0;
    if (a > 1) a = 1;
    channelVolume[0] = (1 - a);
    channelVolume[1] = a;
  }
  else {
    channelVolume[0] = 1;
    channelVolume[1] = 0;
  }
#endif

#if 1
  // ブレスノイズ
  if (accy < 0) {
    float a = -accy;
    if (a > 1) a = 1;
    if (noise < a) noise = a;
    channelVolume[0] = 1 - noise;
  }
  else {
    channelVolume[0] = 1;
  }
#endif

#if 1
  // がなり (傾きによるもの)
  if (accy > 0) {
    float a = accy;
    if (a > 1) a = 1;
    double wavelength = 240 - 200*a;
    currentWavelength[2] += (wavelength - currentWavelength[2]) * portamentoRate;
    currentWavelengthTickCount[2] = (currentWavelength[2] / (double)SAMPLING_RATE);
    channelVolume[2] = a;
  }
  else {
    channelVolume[2] = 0;
  }
#endif
  
  for (int i = 0; i < 2; i++) {
    double wavelength = CalcFrequency(currentNote[i] + bendNoteShift);
    currentWavelength[i] += (wavelength - currentWavelength[i]) * portamentoRate;
    currentWavelengthTickCount[i] = (currentWavelength[i] / (double)SAMPLING_RATE);
  }

  if (interruptCounter > 0) {
    portENTER_CRITICAL(&timerMux);
    {
        interruptCounter--;
    }
    portEXIT_CRITICAL(&timerMux);
  }
#if ENABLE_COMMUNICATETOOL
  // 音色設定ツール(PC)との通信用
  SerialProc();
#endif
}

//---------------------------------
int GetPreferenceInt(int tno, const char* name, int defaultValue) {
  char key[32];
  sprintf(key, "Tone%d/%s", tno, name);
  int ret = -1;
  BeginPreferences(); {
    ret = pref.getInt(key, defaultValue);
  } EndPreferences();
  return ret;
}

//---------------------------------
size_t GetPreferenceBytes(int tno, const char* name, uint8_t* data, size_t length) {
  char key[32];
  sprintf(key, "Tone%d/%s", tno, name);
  size_t ret = 0;
  BeginPreferences(); {
    ret = pref.getBytes(key, data, length);
  } EndPreferences();
  return ret;
}

//---------------------------------
void PutPreferenceInt(int idx, const char* name, int value) {
  char key[32];
  sprintf(key, "Tone%d/%s", idx, name);
  BeginPreferences(); {
    pref.putInt(key, value);
  } EndPreferences();
}

//---------------------------------
void PutPreferenceBytes(int idx, const char* name, const uint8_t* data, size_t length) {
  char key[32];
  sprintf(key, "Tone%d/%s", idx, name);
  BeginPreferences(); {
    pref.putBytes(key, data, length);
  } EndPreferences();
}

//---------------------------------
void LoadToneSetting(int idx) {
  BeginPreferences(); {
    ToneSetting* ts = &toneSettings[idx];

    ts->transpose = 0;
    ts->fineTune = 0;
    ts->reverb = 6;
    ts->portamento = 2;
    ts->keySense = 3;
    ts->breathSense = 6;

    if (idx == 0) {
      for (int i = 0; i < 256; i++) ts->waveA[i] = waveAfuueBrass[i];
      for (int i = 0; i < 256; i++) ts->waveB[i] = waveAfuueBrass[i];
    }
    else if (idx == 1) {
      for (int i = 0; i < 256; i++) ts->waveA[i] = waveAfuueCla[i];
      for (int i = 0; i < 256; i++) ts->waveB[i] = waveAfuueCla[i];
    }
    else if (idx == 2) {
      ts->breathSense = 15;
      for (int i = 0; i < 256; i++) ts->waveA[i] = waveAfuueFlute[i];
      for (int i = 0; i < 256; i++) ts->waveB[i] = waveAfuueFlute[i];
    }
    else if (idx == 3) {
      ts->transpose = -12;
      for (int i = 0; i < 256; i++) ts->waveA[i] = waveAfuueViolin[i];
      for (int i = 0; i < 256; i++) ts->waveB[i] = waveAfuueViolin[i];
    }
    else {
      ts->reverb = 15;
      ts->portamento = 20;
      for (int i = 0; i < 256; i++) {
        if (i < 64) {
          ts->waveA[i] = (-i*400);
        } else if (i < 192) {
          ts->waveA[i] = (i*400)-(400*128);
        } else {
          ts->waveA[i] = ((256-i)*400);
        }
      }
      for (int i = 0; i < 256; i++) ts->waveB[i] = (i*235)-30080;
    }
    for (int i = 0; i < 32; i++) {
      ts->shiftTable[i] = (127 * i) / 31;
    }
  
    // load from pref
    ts->transpose = GetPreferenceInt(idx, "Transpose", ts->transpose);
    ts->fineTune = GetPreferenceInt(idx, "FineTune", ts->fineTune);
    ts->reverb = GetPreferenceInt(idx, "Reverb", ts->reverb);
    ts->portamento = GetPreferenceInt(idx, "Portamento", ts->portamento);
    ts->keySense = GetPreferenceInt(idx, "KeySense", ts->keySense);
    ts->breathSense = GetPreferenceInt(idx, "BreathSense", ts->breathSense);
    GetPreferenceBytes(idx, "WaveA", (uint8_t*)ts->waveA, 256*2);
    GetPreferenceBytes(idx, "WaveB", (uint8_t*)ts->waveB, 256*2);
    GetPreferenceBytes(idx, "ShiftTable", (uint8_t*)ts->shiftTable, 32);
    GetPreferenceBytes(idx, "NoiseTable", (uint8_t*)ts->noiseTable, 32);
    GetPreferenceBytes(idx, "PitchTable", (uint8_t*)ts->pitchTable, 32);
  
    // apply
    if (bootBaseNote == 0) {
      baseNote = 60 + 1 + ChangeSigned(ts->transpose);
    } else {
      baseNote = bootBaseNote;
    }
    fineTune = 440 + ChangeSigned(ts->fineTune);
    reverbRate = ts->reverb * 0.02; // (default 0.152)
    portamentoRate = 1 - (ts->portamento * 0.04); // (default 0.99)
    noteStep = 1.0 / (1 + (ts->keySense * 0.3)); // (default 1.0 / 3.0)
    volumeRate = 0.9 - (ts->breathSense * 0.04); // (default 0.5)

    for (int i = 0; i < 256; i++) {
      waveA[i] = ts->waveA[i];
      waveB[i] = ts->waveB[i];
    }
    for  (int i = 0; i < 32; i++) {
      shiftTable[i] = ts->shiftTable[i];
    }
  } EndPreferences();
}

//---------------------------------
void SaveToneSetting(int idx) {
  BeginPreferences(); {
    ToneSetting* ts = &toneSettings[idx];
  
    PutPreferenceInt(idx, "Transpose", ts->transpose);
    PutPreferenceInt(idx, "FineTune", ts->fineTune);
    PutPreferenceInt(idx, "Reverb", ts->reverb);
    PutPreferenceInt(idx, "Portamento", ts->portamento);
    PutPreferenceInt(idx, "KeySense", ts->keySense);
    PutPreferenceInt(idx, "BreathSense", ts->breathSense);
    PutPreferenceBytes(idx, "WaveA", (uint8_t*)ts->waveA, 512);
    PutPreferenceBytes(idx, "WaveB", (uint8_t*)ts->waveB, 512);
    PutPreferenceBytes(idx, "ShiftTable", (uint8_t*)ts->shiftTable, 32);
    PutPreferenceBytes(idx, "NoiseTable", (uint8_t*)ts->noiseTable, 32);
    PutPreferenceBytes(idx, "PitchTable", (uint8_t*)ts->pitchTable, 32);

  } EndPreferences();
}

//---------------------------------
#if 1
double SawWave(double p) {
  return -1.0 + 2 * p;
}
double SawWave2(double p) {
  if (p < 0.25) {
    return (0.125 - p) * 8;
  }
  return (p - 0.625) * (8/3);
}
#endif

//---------------------------------
double Voice(double p) {
  double v = 255*p;
  int t = (int)v;
  double f = (v - t);
  
  double w0 = (waveA[t] * (1-shift) + waveB[t] * shift);
  t = (t + 1) % 256;
  double w1 = (waveA[t] * (1-shift) + waveB[t] * shift);
  double w = (w0 * (1-f)) + (w1 * f);
  return (w / 32767.0);
}

//---------------------------------
double VoiceA(double p) {
  double v = 255*p;
  int t = (int)v;
  double f = (v - t);
  return (waveA[t] / 32767.0);
}

#define SIN_TABLE_COUNT (1000)
double sin_table[SIN_TABLE_COUNT];
//---------------------------------
double tsin(double p) {
  double v = (SIN_TABLE_COUNT-1)*p;
  int t = (int)v;
  double f = (v - t);
  
  double w0 = sin_table[t];
  t = (t + 1) % SIN_TABLE_COUNT;
  double w1 = sin_table[t];
  return (w0 * (1-f)) + (w1 * f);
}

#if ENABLE_FMSG
//---------------------------------
double FmVoice(double phase, double cp, double mv, double mp) {
//  return 0.5 + 0.5 * sin(6.2831853 * phase * cp + mv * sin(6.2831853 * phase * mp));
  return 0.5 + 0.5 * tsin(phase * cp + (mv / 6.2831853) * tsin(phase * mp));
}
#endif

//---------------------------------
uint8_t CreateWave() {
    if (requestedVolume < 0.001) {
      noteOnTimeLength = 0;
      maxVolume = 0.0;
    }
    else {
      if (maxVolume < requestedVolume) {
        maxVolume = requestedVolume;
      }
      noteOnTimeLength += SAMPLING_TIME_LENGTH;
    }
    // 波形を生成する
#if ENABLE_FMSG
    // double 演算で波形のフェーズを 0-255 で算出する
    for (int i = 0; i < 3; i++) {
      phase[i] += currentWavelengthTickCount[i];
      if (phase[i] >= 100000.0) phase[i] -= 100000.0;
    }
    double g = FmVoice(phase[0], 2.0, 0.5, 3.0);// * 0.5*requestedVolume;
//    g += FmVoice(phase[0] + 0.25, 1.0, 1.0, 1.0) * 0.3;
//    g += FmVoice(phase[0] + 0.5, 0.5, 2.0, 1.0) * 0.3;
    g *= channelVolume[0];
#else
    // double 演算で波形のフェーズを 0-255 で算出する
    for (int i = 0; i < 3; i++) {
      phase[i] += currentWavelengthTickCount[i];
      if (phase[i] >= 1.0) phase[i] -= 1.0;
    }
    //double g = Voice(phase[0]) * channelVolume[0];// + Voice(phase[1]) * channelVolume[1] + Voice(phase[2]) * channelVolume[2];

#if 1
    // waveB でモジュレーション
    mphase += (currentWavelengthTickCount[0] * 1.5);
    if (mphase >= 1.0) mphase -= (int)mphase;
    double s = (1.0 - noteOnTimeLength*30);//0.001 * shiftTable[(int)(32*requestedVolume)];
    if (s < 0.0) s = 0.0;
    double g = VoiceA(phase[0] + s*(waveB[(int)(255*mphase)] / 32767.0)) * channelVolume[0];
#endif

#if 0
    // ファルセット
    g += tsin(phase[1]) * channelVolume[1];
#endif

#if 1
    // がなり (オーバーシュート＋断続)
    g *= (1.0 + channelVolume[2]);
    if (g > 2) g = 2;
    if (g < -2) g = -2;
    if (g > 1) g = 2 - g;
    if (g < -1) g = -2 - g;
    double gn = 1;
    if (phase[2] > 0.5) gn = 1 - channelVolume[2];
    //g *= 0.5 + 0.5 * tsin(phase[2]) * channelVolume[2] + 0.5 * (1.0 - channelVolume[2]);
    g *= gn;
#endif

#endif
    //ノイズ
    double n = ((double)rand() / RAND_MAX);
#if USE_SPEAKERHAT
    double e = 150 * (g * (1-noise) + n * noise) * requestedVolume * masterVolume + reverbBuffer[reverbPos];
#else
    double e = 150 * (g * (1-noise) + n * noise) * requestedVolume + reverbBuffer[reverbPos];
#endif
    //double e = 150 * n * requestedVolume + reverbBuffer[reverbPos];;
    if ( (-0.005 < e) && (e < 0.005) ) {
      e = 0.0;
    }

    reverbBuffer[reverbPos] = e * reverbRate;
    reverbPos = (reverbPos + 1) % REVERB_BUFFER_SIZE;

#if 1
    //メトロノーム
    if (metronome) {
      unsigned long t = millis();
      unsigned long p = t - metronome_tm;
      if (p > 60000/metronome_t) {
        metronome_tm = t;
        metronome_cnt++;
      }
      if (p < 10) {
        //ホワイトノイズを乗せる
        if (metronome_m <= 1) {
          e += metronome_v * n;
        }
        else {
          if ((metronome_cnt % metronome_m) == 1) {
            e += metronome_v * n;
          } else {
            e += metronome_v/2 * n;
          }
        }
      }
    }
#endif
#if 1
    if (e < -127.0) e = -256 - e;
    else if (e > 127.0) e = 256 - e;
#endif
    if (e < -127.0) e = -127.0;
    else if (e > 127.0) e = 127.0;

    uint8_t dac = (uint8_t)(e + 127);
    return dac;
}

//---------------------------------
void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  {
    if (enablePlay) {
      uint8_t dac = CreateWave();
      dacWrite(DACPIN, dac);
    }
    interruptCounter++;
  }
  portEXIT_CRITICAL_ISR(&timerMux);
  //xSemaphoreGiveFromISR(timerSemaphore, NULL);
}

//---------------------------------
void SensorThread(void *pvParameters)
{
    for(;;) {
      if (enablePlay) {
        rawTemperature = GetTemperature();
        int32_t t = rawTemperature - normalTemperature;
        uint32_t prsZero = MIN_PRESSURE + normalPressure + (uint32_t)(t * TEMPERATURE_PRESSURE_RATE);
        rawPressure = GetPressure();
        int32_t p = 0;
        if (rawPressure > prsZero) p = (int32_t)(rawPressure - prsZero);
        // else p = (int32_t)(prsZero - rawPressure); // to enable sukking play
      
        if (p > MAX_PRESSURE) p = MAX_PRESSURE;
        currentPressure = p / (double)MAX_PRESSURE;
      }
      delay(1);
      mcpKeys = readMCP23017();
      delay(1);
      updateAcc();
      delay(1);
    }
}

//---------------------------------
void TickThread(void *pvParameters) {
  for (;;) {
    Tick();
    delay(15);
    if (testMode) {
      static int cnt = 0;
      cnt++;
      M5.Lcd.setCursor(10, 10);
      char s[32];
      sprintf(s, "%04x: %d, %1.1f::", mcpKeys, cnt, (float)currentPressure);
      M5.Lcd.println(s);
      delay(200);
    }
  }
}

//---------------------------------
void loop() {
  drawBattery(100, 5);
  delay(5000);
}

//---------------------------------
// 135x 240
void setup() {
  pinMode(DACPIN, OUTPUT);
#if USE_SPEAKERHAT
  digitalWrite(0, HIGH);
#endif

  bat_per_inclination = 100.0F/(bat_percent_max_vol-bat_percent_min_vol);
  bat_per_intercept = -1 * bat_percent_min_vol * bat_per_inclination;

  M5.begin();
  M5.Axp.ScreenBreath(15);
  M5.Lcd.setRotation(0);
  M5.Lcd.fillScreen(WHITE);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(300);
  digitalWrite(LED_BUILTIN, LOW);
  delay(300);
  digitalWrite(LED_BUILTIN, HIGH);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(1);
  
  M5.Axp.ScreenBreath(8);

#if ENABLE_IMU
  M5.IMU.Init();
  M5.IMU.SetAccelFsr(M5.IMU.AFS_2G);
#endif

#if ENABLE_SERIALOUTPUT
  Serial.begin(115200);
  SerialPrintLn("------");
#endif

  for (int i = 0; i < SIN_TABLE_COUNT; i++) {
    sin_table[i] = 0.6 * sin(i * (3.141592653*2) / SIN_TABLE_COUNT) + 0.2 * sin(i * (2*3.141592653*2) / SIN_TABLE_COUNT) +  + 0.1 * sin(i * (3*3.141592653*2) / SIN_TABLE_COUNT);
  }

  Wire.begin();
  SerialPrintLn("wire begin");

  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  delay(500);
  digitalWrite(LEDPIN, LOW);
  delay(500);
  digitalWrite(LEDPIN, HIGH);

  WiFi.mode(WIFI_OFF); 

  bool i2cError = false;
#if ENABLE_MCP23017
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.drawCentreString("Check MCP23017...", 67, 80, 2);
  bool retMCP23017 = setupMCP23017();
  if (retMCP23017 == false) {
    M5.Lcd.drawCentreString("[ NG ]", 67, 95, 2);
    i2cError = true;
  }
#else
  const int keyPortList[13] = { 13, 12, 14, 27, 26, 16, 17, 5,18, 19, 23, 3, 4 };
  for (int i = 0; i < 13; i++) {
    pinMode(keyPortList[i], INPUT_PULLUP); // INPUT FOR SWITCHES
  }
#endif
  M5.Lcd.drawCentreString("Check BMP180...", 67, 120, 2);
  bool retBMP180 = CheckBMP180();
  if (retBMP180 == false) {
    M5.Lcd.drawCentreString("[ NG ]", 67, 135, 2);
    i2cError = true;
  }
  if (i2cError) {
    for(;;);
  }
  delay(500);
  UpdateKeys();

#if KEYDATACHECK
  int c = 0;
  while (1) {
    uint16_t keyData = GetKeyData();
    char s[32];
    sprintf(s, "%d:key=%04x", (c++)%2, keyData);
    if (keyData & (1<<0)) sprintf(s, "%s LowC", s);
    if (keyData & (1<<1)) sprintf(s, "%s Eb", s);
    if (keyData & (1<<2)) sprintf(s, "%s D", s);
    if (keyData & (1<<3)) sprintf(s, "%s E", s);
    if (keyData & (1<<4)) sprintf(s, "%s F", s);
    if (keyData & (1<<5)) sprintf(s, "%s LowCs", s);
    if (keyData & (1<<6)) sprintf(s, "%s Gs", s);
    if (keyData & (1<<7)) sprintf(s, "%s G", s);
    if (keyData & (1<<8)) sprintf(s, "%s A", s);
    if (keyData & (1<<9)) sprintf(s, "%s B", s);
    if (keyData & (1<<10)) sprintf(s, "%s Down", s);
    if (keyData & (1<<11)) sprintf(s, "%s Up", s);
    SerialPrintLn(s);
    delay(100);
    UpdateKeys();
  }
#endif
  testMode= false;

  BeginPreferences(); {
     // pref.clear(); // CLEAR ALL FLASH MEMORY
    bootBaseNote = 0;
    int ver = pref.getInt("AfuueVer", -1);
    if (ver != AFUUE_VER) {
      pref.clear(); // CLEAR ALL FLASH MEMORY
      pref.putInt("AfuueVer", AFUUE_VER);
    }

    if (toneNo < 0) {
      toneNo = pref.getInt("ToneNo", 0);
    }
    else {
      SerialPrintLn("save toneNo");
      pref.putInt("ToneNo", toneNo);
    }
    LoadToneSetting(toneNo);
    metronome_m = (uint8_t)pref.getInt("MetroMode", 1);
    metronome_t = pref.getInt("MetroTempo", 120);
    metronome_v = (uint8_t)pref.getInt("MetroVolume", 20);

    channelVolume[0] = 1.0;
    channelVolume[1] = 0.0;
    channelVolume[2] = 0.0;

    if ((keyLowC == LOW) && (keyEb == LOW)) {
      lipSensorEnabled = true;
    }

#if ENABLE_MIDI
    midiMode = pref.getInt("MidiMode", MIDIMODE_EXPRESSION);
    midiPgNo = pref.getInt("MidiPgNo", 64);
    if ((keyLowCs == LOW)&&(keyGs == LOW)) {
      midiEnabled = true;
    }
#endif
  } EndPreferences();
  
  //SerialPrintLn("---baseNote");
  //SerialPrintLn(baseNote);
  //SerialPrintLn("---toneNo");
  //SerialPrintLn(toneNo);

  currentPressure = normalPressure = GetPressure();
  delay(100);
  currentTemperature = normalTemperature = GetTemperature();

#if ENABLE_ADXL345
  {
    unsigned char _buff[8];
    _buff[0] = ADXL345_POWER_CTL;
    _buff[1] = 8; // Measure ON
    WriteBytes(ADXL345_DEVICE, _buff, 2);
  }
#endif

  delay(500);
  xTaskCreatePinnedToCore(SensorThread, "SensorThread", 4096, NULL, 1, NULL, CORE1);
  xTaskCreatePinnedToCore(TickThread, "TickThread", 4096, NULL, 1, NULL, CORE1);

  for (int i = 0; i < REVERB_BUFFER_SIZE; i++) {
    reverbBuffer[i] = 0.0;
  }

#if ENABLE_MIDI
  if (midiEnabled) {
    MIDI_Initialize();
  }
  else
#endif
  {
#if !ENABLE_SERIALOUTPUT
  Serial.begin(115200);
#endif

    timerSemaphore = xSemaphoreCreateBinary();
    timer = timerBegin(0, CLOCK_DIVIDER, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, TIMER_ALARM, true);
    timerAlarmEnable(timer);
    SerialPrintLn("timer begin");
  }
  SerialPrintLn("setup done");

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.drawBitmap(0, 240-115, 120, 115, bitmap_logo);

  enablePlay = true;
}

*/
