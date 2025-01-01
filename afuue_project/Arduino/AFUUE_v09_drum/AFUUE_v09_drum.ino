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
#define QUEUE_LENGTH 1
QueueHandle_t xQueue;
TaskHandle_t taskHandle;
volatile bool enablePlay = false;

volatile const float* currentWaveTableA = NULL;
volatile const float* currentWaveTableB = NULL;
volatile float currentWaveShiftLevel = 0.0f;
volatile float phase = 0.0f;
volatile float currentWaveLevel = 0.0f;
#define DELAY_BUFFER_SIZE (8000)
volatile float delayBuffer[DELAY_BUFFER_SIZE];
volatile int delayPos = 0;

int baseNote = 61; // 49 61 73
float fineTune = 440.0f;
volatile float delayRate = 0.15f;
float portamentoRate = 0.99f;
float keySenseTimeMs = 50.0f;
float breathSenseRate = 300.0f;
const float PREPARATIONTIME_LENGTH = 10.0f;

#define CLOCK_DIVIDER (191)
#define TIMER_ALARM (19)
//#define SAMPLING_RATE (20000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*50) = 20kHz
//#define SAMPLING_RATE (25000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*40) = 25kHz
#define SAMPLING_RATE (22044) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (191*19) = 22.05kHz
//#define SAMPLING_RATE (32000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (50*50) = 32kHz
//#define SAMPLING_RATE (48019.2f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (49*34) = 48kHz
//#define SAMPLING_TIME_LENGTH (1.0f / SAMPLING_RATE)
static float currentWavelength = 0.0f;
volatile float currentWavelengthTickCount = 0.0f;
volatile float requestedVolume = 0.0f;
volatile float waveShift = 0.0f;
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

volatile float accx, accy, accz;
volatile float noteOnAccX, noteOnAccY, noteOnAccZ;

#define DRUM_MAX (5)
volatile int32_t drumWavePos[DRUM_MAX];
volatile int32_t drumWaveLength[DRUM_MAX];
volatile const float* drumWaveData[DRUM_MAX];

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

//-------------------------------------
void SynthesizerThread(void *pvParameters) {
  uint16_t keyCurrent = 0;
  while (1) {
    uint16_t mcpKeys = readFromIOExpander();
    UpdateKeys(mcpKeys);
    uint16_t keyData = GetKeyData();
    uint16_t keyPush = (keyData ^ keyCurrent) & keyData;
    keyCurrent = keyData;

      bool keyLowC_Push = ((keyPush & (1 << 0)) != 0);
      bool keyEb_Push = ((keyPush & (1 << 1)) != 0);
      bool keyD_Push = ((keyPush & (1 << 2)) != 0);
      bool keyE_Push = ((keyPush & (1 << 3)) != 0);
      bool keyF_Push = ((keyPush & (1 << 4)) != 0);
      bool keyLowCs_Push = ((keyPush & (1 << 5)) != 0);
      bool keyGs_Push = ((keyPush & (1 << 6)) != 0);
      bool keyG_Push = ((keyPush & (1 << 7)) != 0);
      bool keyA_Push = ((keyPush & (1 << 8)) != 0);
      bool keyB_Push = ((keyPush & (1 << 9)) != 0);

    if (keyLowC_Push || keyLowCs_Push) {
      drumWavePos[1] = 1;
    }
    if (keyEb_Push || keyGs_Push) {
      drumWavePos[2] = 1;
    }
    if (keyD_Push || keyG_Push) {
      drumWavePos[0] = 1;
    }
    if (keyE_Push || keyA_Push) {
      drumWavePos[3] = 1;
    }
    if (keyF_Push || keyB_Push) {
      drumWavePos[4] = 1;
    }
    delay(10);
  }
}

//-------------------------------------
//float sigmoid(float value) {
//  return 1.0f / (1.0f + exp(-value));
//}

//-------------------------------------
float LowPass(float value, float freq, float q) {
	float omega = 2.0f * 3.14159265f * freq / SAMPLING_RATE;
	float alpha = sin(omega) / (2.0f * q);
 
  float cosv = cos(omega);
  float one_minus_cosv = 1.0f - cosv;
	float a0 =  1.0f + alpha;
	float a1 = -2.0f * cosv;
	float a2 =  1.0f - alpha;
	float b0 = one_minus_cosv / 2.0f;
	float b1 = one_minus_cosv;
	float b2 = b0;//one_minus_cosv / 2.0f;
 
	// フィルタ計算用のバッファ変数。
	static float in1  = 0.0f;
	static float in2  = 0.0f;
	static float out1 = 0.0f;
	static float out2 = 0.0f;

  float lp = b0/a0 * value + b1/a0 * in1  + b2/a0 * in2 - a1/a0 * out1 - a2/a0 * out2;

  in2  = in1;
  in1  = value;
  out2 = out1;
  out1 = lp;
  return lp;
}

//-------------------------------------
uint16_t CreateWave() {
  // 波形を生成する
  float e = 0.0f;
#if 1
  for (int i = 0; i < DRUM_MAX; i++) {
    int32_t p = drumWavePos[i];
    if (p > 0) {
    float r = 1.0f;
    if (i == 0) r = 0.3f;
    if (i == 2) r = 0.7f;
    if (i == 3) r = 0.5f;
    if (i == 4) r = 0.8f;
      e += (drumWaveData[i][p] * r);
      p += 2;
      if (p >= drumWaveLength[i]) {
        drumWavePos[i] = 0;
      }
      else {
        drumWavePos[i] = p;
      }
    }
  }
#endif
    //float e = g * requestedVolume + delayBuffer[delayPos];;

    if ( (-0.00002f < e) && (e < 0.00002f) ) {
      e = 0.0f;
    }
    delayBuffer[delayPos] = e * delayRate;
    delayPos = (delayPos + 1) % DELAY_BUFFER_SIZE;

    if (e < -32700.0f) e = -32700.0f;
    else if (e > 32700.0f) e = 32700.0f;

    return static_cast<uint16_t>(e + 32767.0f);
}

//---------------------------------
void IRAM_ATTR onTimer(){
  int8_t data;
  xQueueSendFromISR(xQueue, &data, 0); // キューを送信
}

//---------------------------------
void task(void *pvParameters) {
  while (1) {
    int8_t data;
    xQueueReceive(xQueue, &data, portMAX_DELAY); // キューを受信するまで待つ

  {
    if (enablePlay) {
      uint16_t dac = CreateWave();
#ifdef SOUND_TWOPWM
      ledcWrite(0, dac & 0xFF);
      ledcWrite(1, (dac >> 8) & 0xFF); // HHHH HHHH LLLL LLLL
#else
      dacWrite(DACPIN, dac >> 8);
#endif
    }
  }
  }
}

//-------------------------------------
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

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
#endif // _M5STICKC_H_

  pinMode(DACPIN, OUTPUT);

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

  BeginPreferences(); {
    menu.Initialize(pref);
  } EndPreferences();
  
  for (int i = 0; i < DELAY_BUFFER_SIZE; i++) {
    delayBuffer[i] = 0.0f;
  }
  for (int i = 0; i < DRUM_MAX; i++) {
    drumWavePos[i] = 0;
    drumWaveLength[i] = menu.waveData.GetDrumLength(i)/2;
    drumWaveData[i] = menu.waveData.GetDrumData(i);
  }
  SerialPrintLn("setup done");

#ifdef _M5STICKC_H_
  menu.Display();
#endif

  if (!isUSBMidiMounted) {
    xQueue = xQueueCreate(QUEUE_LENGTH, sizeof(int8_t));
    xTaskCreateUniversal(task, "createWaveTask", 16384, NULL, 5, &taskHandle, CORE0);

    timer = timerBegin(0, CLOCK_DIVIDER, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, TIMER_ALARM, true);
    timerAlarmEnable(timer);
    SerialPrintLn("timer begin");
  }

  xTaskCreatePinnedToCore(SynthesizerThread, "SynthesizerThread", 2048, NULL, 1, NULL, CORE1);

  enablePlay = true;
  requestedVolume = 0.7f;
}

//-------------------------------------
void loop() {
  // Core0 は波形生成に専念している, loop は Core1
  for (int i = 0; i < DRUM_MAX; i++) {
    int32_t p = drumWavePos[i];
    char s[32];
    sprintf(s, "_%d_", p);
    M5.Lcd.drawString(s, 10, 100 + i * 30, 2);
  }
  delay(100);
}

