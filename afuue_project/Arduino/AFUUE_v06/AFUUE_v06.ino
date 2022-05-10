#include <WiFi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "afuue_common.h"
#include "midi.h"
#include "communicate.h"
#include "wavedata.h"

static Preferences pref;

const char* version = "AFUUE ver1.0.0.1";
const uint8_t commVer1 = 0x16;  // 1.6
const uint8_t commVer2 = 0x02;  // protocolVer = 2

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

static double currentVolume = 0.0;
volatile double channelVolume[3] = {1.0, 0.0, 0.0 };
volatile double requestedVolume = 0.0;
volatile double maxVolume = 0.0;

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

static double currentNote[3] = { 60, 64, 67 };
static double startNote[3] = { 60, 64, 67 };
static double targetNote[3] = { 60, 64, 67 };
static double noteStep = 0;
static double noteDiv = 0.5;
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
double CalcInvFrequency(double note) {
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
    else if (keyPush & (1<<1)) {
      ts->fineTune++;
      if (ts->fineTune > 10) {
        ts->fineTune = -10;
      }
      fineTune = 440 + ChangeSigned(ts->fineTune);
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
}

//---------------------------------
double StepCalc(double step) {
  int b = int(step * 18.9999);
  double f = (step - (b / 19.0)) / (1 / 19.0);
  double v0 = stepTable[b];
  double v1 = stepTable[b+1];
  return (v0 * (1-f)) + (v1 * f);
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
    currentNote[2] = currentNote[0] + 7;
    for (int i = 0; i < 3; i++) {
      targetNote[i] = currentNote[i];
    }
    noteStep = 1;
    
  } else {
    
    int n0;
    int n1;
    int n2;
    if ((channelVolume[1] > 0.0) || (channelVolume[2] > 0.0)) {
      GetChord(n0, n1, n2);
    } else {
      n0 = GetNoteNumber(false);
      n1 = n2 = n0;
    }
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
        n1 = n2 = n0;
        for (int i = 0; i < 3; i++) {
          startNote[i] = n0;
          currentNote[i] = n0;
          targetNote[i] = n0;
        }
      }
    }

    if ((targetNote[0] != n0) || (targetNote[1] != n1) || (targetNote[2] != n2)) {
      //double d = abs(note - currentNote);
      noteStep = 0.0;
      for (int i = 0; i < 3; i++) {
        startNote[i] = currentNote[i];
      }
      targetNote[0] = n0;
      targetNote[1] = n1;
      targetNote[2] = n2;
    }
    else if (pp < 0.01) {
      for (int i = 0; i < 3; i++) {
        currentNote[i] = targetNote[i];
      }
      noteStep = 1.0;
    }
    else {
      if (noteStep >= 1.0) {
        
      } else {
        noteStep += noteDiv;
        if (noteStep > 1.0) {
          noteStep = 1.0;
        }
        double t = pow(noteStep, keyChangeCurve);
        //double t = StepCalc(noteStep);
        for (int i = 0; i < 3; i++) {
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
          if ((prevNoteNumber != note) && (noteStep > 0.8)) {
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
#if 0
    int d = (4095 - analogRead(32));
    if (d > 1000) d = 1000;
    double a = (d / 1000.0);
#else
    // なんかどこかで値が変わってしまった
    int d = (500 - analogRead(32));
    if (d < 0) d = 0;
    if (d > 70) d = 70;
    double a = (d / 70.0);
#endif
    static double ana = 0.0;
    ana += (a - ana)*0.5;
    a = BENDRATE * (1-ana);
    bendNoteShift += a;
  }
#endif
  // ベンド (音量によるもの)
  double bv = -(0.5 - currentVolume) * 0.02;
  bendNoteShift += bv;
  
  for (int i = 0; i < 3; i++) {
    double wavelength = CalcInvFrequency(currentNote[i] + bendNoteShift);
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
    //ts->keySense = GetPreferenceInt(idx, "KeySense", ts->keySense);
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
    noteDiv = 1.0 / (1 + (ts->keySense * 0.3)); // (default 1.0 / 3.0)
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
#if 0
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

double VoiceA(double p) {
  double v = 255*p;
  int t = (int)v;
  double f = (v - t);
  return (waveA[t] / 32767.0);
}

#if ENABLE_FMSG
#define SIN_TABLE_COUNT (1000)
double sin_table[SIN_TABLE_COUNT];
//---------------------------------
double tsin(double p) {
  int i = (int)(SIN_TABLE_COUNT * p) % SIN_TABLE_COUNT;
  return sin_table[i];
}

//---------------------------------
double FmVoice(double phase, double cp, double mv, double mp) {
//  return 0.5 + 0.5 * sin(6.2831853 * phase * cp + mv * sin(6.2831853 * phase * mp));
  return 0.5 + 0.5 * tsin(phase * cp + (mv / 6.2831853) * tsin(phase * mp));
}
#endif

//---------------------------------
uint8_t CreateWave() {
    if (requestedVolume < 0.01) {
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
    //double g = Voice(phase[0]) * channelVolume[0] + Voice(phase[1]) * channelVolume[1] + Voice(phase[2]) * channelVolume[2];

#if 1
    // waveB でモジュレーション
    mphase += (currentWavelengthTickCount[0] * 1.5);
    if (mphase >= 1.0) mphase -= (int)mphase;
    double s = (1.0 - noteOnTimeLength*30);//0.001 * shiftTable[(int)(32*requestedVolume)];
    if (s < 0.0) s = 0.0;
    s *= 0.2 * maxVolume;
    double g = VoiceA(phase[0] + s*(waveB[(int)(255*mphase)] / 32767.0)) * channelVolume[0];
#endif
#endif
    //ノイズ
    double n = ((double)rand() / RAND_MAX);
    double e = 150 * (g * (1-noise) + n * noise) * requestedVolume + reverbBuffer[reverbPos];;
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
      dacWrite(DAC1, dac);
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
  
        ledcWrite(0, 1+(uint8_t)(254 * requestedVolume));
      }
      delay(1);
    }
}

//---------------------------------
void TickThread(void *pvParameters) {
  for (;;) {
    Tick();
    delay(15);
  }
}

//---------------------------------
void loop() {
  delay(10000);  //ほぼ何もしない Core0 は波形生成に専念してもらっている
}

//---------------------------------
void setup() {
#if ENABLE_SERIALOUTPUT
  Serial.begin(115200);
  SerialPrintLn("------");
#endif

#if ENABLE_FMSG
  for (int i = 0; i < SIN_TABLE_COUNT; i++) {
    sin_table[i] = sin(i * (3.141592653*2) / SIN_TABLE_COUNT);
  }
#endif

#if ENABLE_ANALOGREAD_GPIO32
  pinMode(32, ANALOG);
  analogSetAttenuation(ADC_11db);
#if 0
  for (;;) {
    char s[32];
    sprintf(s, "Analog:%d", analogRead(32));
    SerialPrintLn(s);
    delay(500);
  }
#endif
#endif

  Wire.begin();
  SerialPrintLn("wire begin");

  ledcSetup(0, 12800, 8);
  ledcAttachPin(33, 0);

  ledcWrite(0, 255);
  //ledcWrite(1, 40);
  delay(500);
  ledcWrite(0, 40);
  //ledcWrite(1, 255);
  delay(500);
  ledcWrite(0, 20);
  //ledcWrite(1, 20);

  WiFi.mode(WIFI_OFF); 

  const int keyPortList[13] = { 13, 12, 14, 27, 26, 16, 17, 5,18, 19, 23, 3, 4 };
  for (int i = 0; i < 13; i++) {
    pinMode(keyPortList[i], INPUT_PULLUP); // INPUT FOR SWITCHES
  }
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
#if ENABLE_CHORD_PLAY
      channelVolume[0] = 0.5;
      channelVolume[1] = 0.5;
      channelVolume[2] = 0.5;
#else
      lipSensorEnabled = true;
#endif
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

  ledcWrite(0, 255);
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

  enablePlay = true;
}
