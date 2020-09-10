#include <Wire.h>
#include <Preferences.h>
#include <nvs.h>

const char* version = "AFUUE ver0.5.1.4";

static Preferences pref;

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

#define BMP180_ADDR 0x77 // 7-bit address
#define BMP180_REG_CONTROL 0xF4
#define BMP180_REG_RESULT 0xF6
#define BMP180_COMMAND_TEMPERATURE 0x2E
#define BMP180_COMMAND_PRESSURE0 0x34
#define BMP180_COMMAND_PRESSURE1 0x74
#define BMP180_COMMAND_PRESSURE2 0xB4
#define BMP180_COMMAND_PRESSURE3 0xF4
#define TEMP_RATE (-0.55f)

static char _error;

hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
volatile int interruptCounter = 0;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

static uint8_t initCnt = 50;

static uint32_t defPressure = 0;
static uint32_t defTemperature = 0;

#define fs (20000.0)
static double ftt[3] = { 0.0, 0.0, 0.0 };
volatile double fst[3] = { 0.0, 0.0, 0.0 };
volatile double volBase[3] = {1.0, 0.0, 0.0 };
volatile double volReq = 0.0;
volatile double shift = 0.0;
volatile double noise = 0.0;
volatile double pitch = 0.0;

static double vol = 0.0;

static double currentNote[3] = { 60, 64, 67 };
static double startNote[3] = { 60, 64, 67 };
static double targetNote[3] = { 60, 64, 67 };
static double noteStep = 0;
static double noteDiv = (1.0 / 3.0);
static int noteOnTimeLength = 0;
static double keyChangeCurve = 12.0;
static int baseNote = 0;
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
#define REVERB_BUFFER_SIZE (9801)
volatile double reverbBuffer[REVERB_BUFFER_SIZE];
volatile int reverbPos = 0;

volatile bool metronome = false;
volatile uint8_t metronome_m = 1;
volatile unsigned long metronome_tm = 0;
volatile int metronome_t = 90;
volatile uint8_t metronome_v = 10;
volatile int metronome_cnt = 0;

#define ENABLE_BLE_MIDI (0)
#define ENABLE_MIDI (1)

bool midiEnabled = false;
uint8_t midiPacket[32];
bool deviceConnected = false;
bool playing = false;
int channelNo = 0;
int prevNoteNumber = -1;
unsigned long activeNotifyTime = 0;

void SerialPrintLn(char* text) {
#if !ENABLE_MIDI
  //Serial.println(text);
#endif
}

static uint8_t sendBuffer[600];
static uint8_t receiveBuffer[1024];
static int receivePos = 0;
static bool waitForCommand = true;
void SerialSend(int sendSize) {
  if (midiEnabled == false) {
    Serial.write(sendBuffer, sendSize);
  }
}

static int usePreferencesDepth = 0;
void BeginPreferences() {
  usePreferencesDepth++;
  if (usePreferencesDepth > 1) return;
  if (timer) timerAlarmDisable(timer);
  delay(10);
  pref.begin("Afuue/Settings", false);
}

void EndPreferences() {
  pref.end();  
  delay(10);
  usePreferencesDepth--;
  if (usePreferencesDepth == 0) {
    if (timer) timerAlarmEnable(timer);
  }
  if (usePreferencesDepth < 0) usePreferencesDepth = 0;
}


#if ENABLE_MIDI
#include <HardwareSerial.h>
HardwareSerial SerialHW(0);
#endif

#if ENABLE_BLE_MIDI
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"  // Apple BLE MIDI とするので固定
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

BLECharacteristic *pCharacteristic;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      SerialPrintLn("connected");

      MIDI_NoteOn(baseNote + 1, 50);
      delay(500);
      MIDI_NoteOff();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      SerialPrintLn("disconnected");
    }
};
#endif


//---------------------------------
char readBytes(unsigned char *values, char length)
// Read an array of bytes from device
// values: external array to hold data. Put starting register in values[0].
// length: number of bytes to read
{
  char x;

  Wire.beginTransmission(BMP180_ADDR);
  Wire.write(values[0]);
  _error = Wire.endTransmission();
  if (_error == 0)
  {
    Wire.requestFrom(BMP180_ADDR,length);
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
char readInt(char address, int16_t &value)
{
  unsigned char data[2];

  data[0] = address;
  if (readBytes(data,2))
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
  if (readBytes(data,2))
  {
    value = (uint16_t)((data[0]<<8)|data[1]);
    //if (*value & 0x8000) *value |= 0xFFFF0000; // sign extend if negative
    return(1);
  }
  value = 0;
  return(0);
}

//---------------------------------
char writeBytes(unsigned char *values, char length)
{
  char x;
  
  Wire.beginTransmission(BMP180_ADDR);
  Wire.write(values,length);
  _error = Wire.endTransmission();
  if (_error == 0)
    return(1);
  else
    return(0);
}

//---------------------------------
uint16_t getPressure() {  // 16bit unsigned
  unsigned char wdata[2] = { BMP180_REG_CONTROL, BMP180_COMMAND_PRESSURE0 };
  if (writeBytes(wdata, 2)) {
    delay(5);
    unsigned char rdata[3] = { BMP180_REG_RESULT, 0, 0 };
    if (readBytes(rdata, 3)) {
      return (((uint16_t)rdata[0]) << 8) | (uint16_t)rdata[1];
    }
  }
  return 0;
}

//---------------------------------
int16_t getTemperature() {  // 16bit singed
  unsigned char wdata[2] = { BMP180_REG_CONTROL, BMP180_COMMAND_TEMPERATURE };
  if (writeBytes(wdata, 2)) {
    delay(5);
    unsigned char rdata[2] = { BMP180_REG_RESULT, 0 };
    if (readBytes(rdata, 2)) {
      return (int16_t)((((uint16_t)rdata[0]) << 8) | (uint16_t)rdata[1]);
    }
  }
  return 0;
}


//---------------------------------
double getNoteNumber2(bool chordFlag) {
  bool keyLowC = digitalRead(16);
  bool keyEb = digitalRead(17);
  bool keyD = digitalRead(5);
  bool keyE = digitalRead(18);
  bool keyF = digitalRead(19);
  bool keyLowCs = digitalRead(13);
  bool keyGs =digitalRead(12);
  bool keyG = digitalRead(14);
  bool keyA = digitalRead(27);
  bool keyB = digitalRead(26);
  bool octDown = digitalRead(23);
  bool octUp = digitalRead(3);

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
  if (chordFlag == false) {
    if (keyGs == LOW) n += 1.0; // #
    if (keyLowCs == LOW) n -= 1.0; // b
  }
  
  if (octDown == LOW) n -= 12;
  if (octUp == LOW) n += 12;

  double bnote = baseNote - 13; // C
  return bnote + n;
}

//---------------------------------
int getNoteNumber() {
  bool keyLowC = digitalRead(16);
  bool keyEb = digitalRead(17);
  bool keyD = digitalRead(5);
  bool keyE = digitalRead(18);
  bool keyF = digitalRead(19);
  bool keyLowCs = digitalRead(13);
  bool keyGs =digitalRead(12);
  bool keyG = digitalRead(14);
  bool keyA = digitalRead(27);
  bool keyB = digitalRead(26);
  bool octDown = digitalRead(23);
  bool octUp = digitalRead(3);

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

//---------------------------------
void getChord(int& n0, int& n1, int& n2) {
  bool keyLowCs = digitalRead(13);
  bool keyGs =digitalRead(12);

  double note = getNoteNumber2(true);

  n0 = note;
  n1 = note + 4;
  n2 = note + 7;

  if (keyGs == LOW) n0 -= 2;     // 7th
  if (keyLowCs == LOW) n1--;  // Minor
}

//--------------------------
double CalcInvFrequency(double note) {
  return fineTune * pow(2, (note - (69-12))/12);
}

//---------------------------------
void loop() {
  int32_t tmp = getTemperature() - defTemperature;
  uint32_t prsZero = 10 + defPressure + (uint32_t)(tmp * TEMP_RATE);
  uint32_t prs = getPressure();
  int32_t p = 0;
  if (prs > prsZero) p = (int32_t)(prs - prsZero);

  bool updateFlag = true;
  const int maxPressure = 400;
  if (p > maxPressure) p = maxPressure;
  double pp = p / (double)maxPressure;
  if (pp > 1.0) pp = 1.0;
  if (initCnt > 0) {
    initCnt--;
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
    if ((volBase[1] > 0.0) || (volBase[2] > 0.0)) {
      getChord(n0, n1, n2);
    } else {
      n0 = getNoteNumber2(false);
      n1 = n2 = n0;
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
      noteOnTimeLength = 0;
    }
    else {
      noteOnTimeLength++;
      if (noteStep >= 1.0) {
        
      } else {
        noteStep += noteDiv;
        if (noteStep > 1.0) {
          noteStep = 1.0;
        }
        double t = pow(noteStep, keyChangeCurve);
        for (int i = 0; i < 3; i++) {
          currentNote[i] = startNote[i] + (targetNote[i] - startNote[i]) * t;
        }
      }
    } 
  }

  double ppReq = pp*pp;

#if 0
  // デバッグ用ボリューム最大
  {
    bool octDown = digitalRead(3);
    bool octUp = digitalRead(23);
    if ((octDown == false) && (octUp == false)) {
      ppReq = 1.0;
    }
  }
#endif

  if (ppReq > vol) {
    vol += (ppReq - vol) * volumeRate;
  } else {
    vol += (ppReq - vol) * 0.9;
  }
  volReq = vol;

  if (midiEnabled) {
    if (deviceConnected) {
      int note = (int)targetNote;
      int v = (int)(vol * 127);
      if (playing == false) {
        if (v > 0) {
          MIDI_NoteOn(note, v);
        }
      } else {
        if (v == 0) {
            MIDI_NoteOff();
        } else {
          if (prevNoteNumber != note) {
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
        delay(1);
      }
    }
    return;
  }

#if 0
  {
    // 時間によるシフト
    double k = noteOnTimeLength / 50.0;
    if (k > 1) k = 1;
    shift = 1 - k;
  }
#endif

#if 1
  {
    // 音量によるシフト
    double u = vol * 30.99;
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

  for (int i = 0; i < 3; i++) {
    double ft = CalcInvFrequency(currentNote[i]);
    ftt[i] += (ft - ftt[i]) * portamentoRate;
    fst[i] = (ftt[i] / fs);
  }
  ledcWrite(0, (uint8_t)(volReq * 255));

  if (interruptCounter > 0) {
    portENTER_CRITICAL(&timerMux);
    {
        interruptCounter--;
    }
    portEXIT_CRITICAL(&timerMux);
  }
  SerialProc();
  delay(1);
}

//---------------------------------
void MIDI_NoteOn(int note, int vol) {
    if (playing != false) return;
#if ENABLE_BLE_MIDI
    midiPacket[0] = 0x80; // header
    midiPacket[1] = 0x80; // timestamp, not implemented
    midiPacket[2] = 0x90 + channelNo; // note down, channel 0
    midiPacket[3] = note;
    midiPacket[4] = 127;  // velocity
    midiPacket[5] = 0xB0 + channelNo; // control change, channel 0
    //midiPacket[6] = 0x0B; // expression
    midiPacket[6] = 0x02; // breath control
    midiPacket[7] = vol;
    pCharacteristic->setValue(midiPacket, 8); // packet, length in bytes
    pCharacteristic->notify();
#endif
#if ENABLE_MIDI
    midiPacket[0] = 0x90 + channelNo;
    midiPacket[1] = note;
    midiPacket[2] = 127;
    midiPacket[3] = 0xB0 + channelNo;
    //midiPacket[3] = 0x0B; // expression
    midiPacket[3] = 0x02; // breath control
    midiPacket[5] = vol;
      
    SerialHW.write(midiPacket, 6);
#endif
    playing = true;
    prevNoteNumber = note;
    delay(16);
}

//---------------------------------
void MIDI_ChangeNote(int note, int vol) {
    if ((playing == false) || (prevNoteNumber < 0)) return;
    MIDI_NoteOff();
    
    MIDI_NoteOn(note, vol);
}

//---------------------------------
void MIDI_NoteOff() {
    if ((playing == false) || (prevNoteNumber < 0)) return;
#if ENABLE_BLE_MIDI
    midiPacket[0] = 0x80; // header
    midiPacket[1] = 0x80; // timestamp, not implemented
    midiPacket[2] = 0x80 + channelNo; // note up, channel 0
    midiPacket[3] = prevNoteNumber;
    midiPacket[4] = 0;    // velocity
    pCharacteristic->setValue(midiPacket, 5); // packet, length in bytes)
    pCharacteristic->notify();
#endif
#if ENABLE_MIDI
    midiPacket[0] = 0x80 + channelNo;
    midiPacket[1] = prevNoteNumber;
    midiPacket[2] = 0;
      
    SerialHW.write(midiPacket, 3);
#endif
    playing = false;
    prevNoteNumber = -1;
    delay(16);
}

//---------------------------------
void MIDI_BreathControl(int vol) {
#if ENABLE_BLE_MIDI
    midiPacket[0] = 0x80; // header
    midiPacket[1] = 0x80; // timestamp, not implemented
    midiPacket[2] = 0xB0 + channelNo; // control change, channel 0
    //midiPacket[3] = 0x0B; // expression
    midiPacket[3] = 0x02; // breath control
    midiPacket[4] = vol;
    pCharacteristic->setValue(midiPacket, 5); // packet, length in bytes
    pCharacteristic->notify();
#endif
#if ENABLE_MIDI
    midiPacket[0] = 0xB0 + channelNo;
    //midiPacket[1] = 0x0B; // expression
    midiPacket[1] = 0x02; // breath control
    midiPacket[2] = vol;
      
    SerialHW.write(midiPacket, 3);
#endif
    delay(16);
}

//---------------------------------
void MIDI_ActiveNotify() {
#if ENABLE_BLE_MIDI
    midiPacket[0] = 0x80; // header
    midiPacket[1] = 0x80; // timestamp, not implemented
    midiPacket[2] = 0xFE; // active sense
    pCharacteristic->setValue(midiPacket, 3); // packet, length in bytes
    pCharacteristic->notify();
#endif
#if ENABLE_MIDI
    midiPacket[0] = 0xFE;
      
    SerialHW.write(midiPacket, 1);
#endif
    delay(4);
}

//---------------------------------
int ChangeSigned(int data) {
    if (data < 64)
    {
        return data;
    }
    else
    {
        return data - 128;
    }
}

int ChangeUnsigned(int data)
{
    if (data >= 0)
    {
        return data;
    }
    else
    {
        return 128 + data;
    }
}

//---------------------------------
void OnReceiveCommand() {
    /*
     * 0xA1 SET TONE
     * 0xA2 SET TRANSPOSE
     * 0xA3 SET FINE TUNE
     * 0xA4 SET REVERB LEVEL
     * 0xA5 SET PORTAMENTO LEVEL
     * 0xA6 SET KEY SENSITIVITY
     * 0xA7 SET BREATH SENSITIVITY
     * 0xA8 SET METRONOME
     * 0xAA SET WAVE A DATA
     * 0xAB SET WAVE B DATA
     * 0xAC SET SHIFT TABLE
     * 
     * 0xB1 GET TONE
     * 0xB2 GET TRANSPOSE
     * 0xB3 GET FINE TUNE
     * 0xB4 GET REVERB LEVEL
     * 0xB5 GET PORTAMENTO LEVEL
     * 0xB6 GET KEY SENSITIVITY
     * 0xB7 GET BREATH SENSITIVITY
     * 0xB8 GET METRONOME
     * 0xBA GET WAVE A DATA
     * 0xBB GET WAVE B DATA
     * 0xBC GET SHIFT TABLE
     * 
     * 0xC1 RESPONSE TONE
     * 0xC2 RESPONSE TRANSPOSE
     * 0xC3 RESPONSE FINE TUNE
     * 0xC4 RESPONSE REVERB LEVEL
     * 0xC5 RESPONSE PORTAMENTO LEVEL
     * 0xC6 RESPONSE KEY SENSITIVITY
     * 0xC7 RESPONSE BREATH SENSITIVITY
     * 0xC8 RESPONSE METRONOME
     * 0xCA RESPONSE WAVE A DATA
     * 0xCB RESPONSE WAVE B DATA
     * 0xCC RESPONSE SHIFT TABLE
     * 
     * 0xE1 STORE CONFIGS
     * 
     * 0xEE SOFTWARE RESET
     * 
     * 0xF1 GET VERSION
     * 
     * 0xFE COMMAND ACK
     * 0xFF END MESSAGE
     */
    ToneSetting* ts = &toneSettings[toneNo];
    int command = receiveBuffer[0];
    switch (command)
    {
        case 0xA1: // SET TONE
        case 0xC1: // RESPONSE TONE
            {
                toneNo = receiveBuffer[1];
                volBase[0] = receiveBuffer[2] / 127.0;
                volBase[1] = receiveBuffer[3] / 127.0;
                volBase[2] = receiveBuffer[4] / 127.0;
                BeginPreferences(); {
                  pref.putInt("ToneNo", toneNo);
                  LoadToneSetting(toneNo);
                } EndPreferences();
                SendCommandAck();
            }
            break;
        case 0xB1: // GET TONE NUMBER
            {
              sendBuffer[0] = 0xC1;
              sendBuffer[1] = toneNo;
              sendBuffer[2] = (uint8_t)(volBase[0] * 127);
              sendBuffer[3] = (uint8_t)(volBase[1] * 127);
              sendBuffer[4] = (uint8_t)(volBase[2] * 127);
              sendBuffer[5] = 0xFF;
              Serial.write(sendBuffer, 4);
              Serial.flush();
            }
            break;
        case 0xA2: // SET TRANSPOSE
        case 0xC2: // RESPONSE TRANSPOSE
            {
                int tno = receiveBuffer[1];
                int data = ChangeSigned( receiveBuffer[2] );
                toneSettings[tno].transpose = data;
                baseNote = 60 + 1 + data; // 61  C (C#)
                SendCommandAck();
            }
            break;
        case 0xB2: // GET TRANSPOSE
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xC2;
              sendBuffer[2] = ChangeUnsigned(ts->transpose);
              sendBuffer[3] = 0xFF;
              Serial.write(sendBuffer, 4);
              Serial.flush();
            }
            break;
        case 0xA3: // SET FINE TUNE
        case 0xC3: // RESPONSE FINE TUNE
            {
                int tno = receiveBuffer[1];
                int data = ChangeSigned( receiveBuffer[2] );
                toneSettings[tno].fineTune = data;
                fineTune = 440.0 + data;
                SendCommandAck();
            }
            break;
        case 0xB3: // GET FINE TUNE
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xC3;
              sendBuffer[2] = ChangeUnsigned(ts->fineTune);
              sendBuffer[3] = 0xFF;
              Serial.write(sendBuffer, 4);
              Serial.flush();
            }
            break;
        case 0xA4: // SET REVERB LEVEL
        case 0xC4: // RESPONSE REVERB LEVEL
            {
                int tno = receiveBuffer[1];
                int data = ChangeSigned( receiveBuffer[2] ); // 0 - 20
                toneSettings[tno].reverb = data;
                reverbRate = data * 0.02; // (default 0.152)
                SendCommandAck();
            }
            break;
        case 0xB4: // GET REVERB LEVEL
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xC4;
              sendBuffer[2] = ChangeUnsigned(ts->reverb);
              sendBuffer[3] = 0xFF;
              Serial.write(sendBuffer, 4);
              Serial.flush();
            }
            break;
        case 0xA5: // SET PORTAMENTO LEVEL
        case 0xC5: // RESPONSE PORTAMENTO LEVEL
            {
                int tno = receiveBuffer[1];
                int data = ChangeSigned( receiveBuffer[2] ); // 0 - 20
                toneSettings[tno].portamento = data;
                portamentoRate = 1 - (data * 0.04); // (default 0.99)
                SendCommandAck();
            }
            break;
        case 0xB5: // GET PORTAMENTO LEVEL
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xC5;
              sendBuffer[2] = ChangeUnsigned(ts->portamento);
              sendBuffer[3] = 0xFF;
              Serial.write(sendBuffer, 4);
              Serial.flush();
            }
            break;
        case 0xA6: // SET KEY SENSITIVITY
        case 0xC6: // RESPONSE KEY SENSITIVITY
            {
                int tno = receiveBuffer[1];
                int data = ChangeSigned( receiveBuffer[2] ); // 0 - 20
                toneSettings[tno].keySense = data;
                noteDiv = 1.0 / (1 + (data * 0.3)); // (default 1.0 / 3.0)
                SendCommandAck();
            }
            break;
        case 0xB6: // GET KEY SENSITIVITY
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xC6;
              sendBuffer[2] = ChangeUnsigned(ts->keySense);
              sendBuffer[3] = 0xFF;
              Serial.write(sendBuffer, 4);
              Serial.flush();
            }
            break;
        case 0xA7: // SET BREATH SENSITIVITY
        case 0xC7: // RESPONSE BREATH SENSITIVITY
            {
                int tno = receiveBuffer[1];
                int data = ChangeSigned( receiveBuffer[2] ); // 0 - 20
                toneSettings[tno].breathSense = data;
                volumeRate = 0.9 - (data * 0.04); // (default 0.5)
                SendCommandAck();
            }
            break;
        case 0xB7: // GET BREATH SENSITIVITY
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xC7;
              sendBuffer[2] = ChangeUnsigned(ts->breathSense);
              sendBuffer[3] = 0xFF;
              Serial.write(sendBuffer, 4);
              Serial.flush();
            }
            break;
        case 0xA8: // SET METRONOME
            {
              bool enabled = (receiveBuffer[1] == 1);
              uint8_t mode = receiveBuffer[2];
              int volume = receiveBuffer[3];
              int tempo = receiveBuffer[4] & 0x7F | ((receiveBuffer[5] & 0x7F) << 7);
              metronome = enabled;
              metronome_m = mode;
              metronome_t = tempo;
              metronome_v = volume;
              metronome_cnt = 0;
              BeginPreferences(); {
                pref.putInt("MetroMode", metronome_m);
                pref.putInt("MetroTempo", metronome_t);
                pref.putInt("MetroVolume", metronome_v);
              } EndPreferences();
              SendCommandAck();
              break;
            }
        case 0xB8: // GET METRONOME
            {
              sendBuffer[0] = 0xC8;
              sendBuffer[1] = 0;
              if (metronome) {
                sendBuffer[1] = 1;
              }
              sendBuffer[2] = metronome_m;
              sendBuffer[3] = metronome_v;
              sendBuffer[4] = (uint8_t)(metronome_t & 0x7F);
              sendBuffer[5] = (uint8_t)((metronome_t >> 7) & 0x7F);
              sendBuffer[6] = 0xFF;
              Serial.write(sendBuffer, 7);
              Serial.flush();
              break;
            }
        case 0xAA: // SET WAVE A DATA
        case 0xCA: // SET WAVE A DATA
        case 0xAB: // SET WAVE B DATA
        case 0xCB: // SET WAVE B DATA
            if (receivePos >= 512) {
              int tno = receiveBuffer[1];
              for (int i = 0; i < 256; i++) {
                uint32_t d0 = receiveBuffer[2 + i*2];
                uint32_t d1 = receiveBuffer[2 + i*2 + 1];
                uint32_t rdata = ((d0 << 2) | (d1 << 9));
                int32_t data = *reinterpret_cast<int32_t*>(&rdata);
                if ((command & 0x0F) == 0x0A) {
                  toneSettings[tno].waveA[i] = data;
                  waveA[i] = data;
                } else {
                  toneSettings[tno].waveB[i] = data;
                  waveB[i] = data;
                }
              }
              SendCommandAck();
            }
            break;
        case 0xBA: // GET WAVE A DATA
        case 0xBB: // GET WAVE B DATA
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              if ((command & 0x0F) == 0x0A) {
                sendBuffer[0] = 0xCA;
                for (int i = 0; i < 256; i++) {
                  int d0 = (toneSettings[tno].waveA[i] >> 2) & 0x7F;
                  int d1 = (toneSettings[tno].waveA[i] >> 9) & 0x7F;
                  sendBuffer[2 + i*2] = (byte)d0; 
                  sendBuffer[2 + i*2 + 1] = (byte)d1; 
                }
              } else {
                sendBuffer[0] = 0xCB;
                for (int i = 0; i < 256; i++) {
                  int d0 = (toneSettings[tno].waveB[i] >> 2) & 0x7F;
                  int d1 = (toneSettings[tno].waveB[i] >> 9) & 0x7F;
                  sendBuffer[2 + i*2] = (byte)d0; 
                  sendBuffer[2 + i*2 + 1] = (byte)d1; 
                }
              }
              sendBuffer[514] = 0xFF;
              Serial.write(sendBuffer, 515);
              Serial.flush();
            }
            break;
        case 0xAC: // SET SHIFT TABLE
        case 0xCC:
            {
              int tno = receiveBuffer[1];
              for (int i = 0; i < 32; i++) {
                shiftTable[i] = receiveBuffer[2 + i];
                toneSettings[tno].shiftTable[i] = shiftTable[i];
              }
              SendCommandAck();
            }
            break;
        case 0xBC: // GET SHIFT TABLE
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xCC;
              for (int i = 0; i < 32; i++) {
                sendBuffer[2 + i] = toneSettings[tno].shiftTable[i];
              }
              sendBuffer[34] = 0xFF;
              Serial.write(sendBuffer, 35);
              Serial.flush();
            }
            break;
        case 0xAD: // SET NOISE TABLE
        case 0xCD:
            {
              int tno = receiveBuffer[1];
              for (int i = 0; i < 32; i++) {
                noiseTable[i] = receiveBuffer[2 + i];
                toneSettings[tno].noiseTable[i] = noiseTable[i];
              }
              SendCommandAck();
            }
            break;
        case 0xBD: // GET NOISE TABLE
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xCD;
              for (int i = 0; i < 32; i++) {
                sendBuffer[2 + i] = toneSettings[tno].noiseTable[i];
              }
              sendBuffer[34] = 0xFF;
              Serial.write(sendBuffer, 35);
              Serial.flush();
            }
            break;
        case 0xAE: // SET PITCH TABLE
        case 0xCE:
            {
              int tno = receiveBuffer[1];
              for (int i = 0; i < 32; i++) {
                pitchTable[i] = receiveBuffer[2 + i];
                toneSettings[tno].pitchTable[i] = pitchTable[i];
              }
              SendCommandAck();
            }
            break;
        case 0xBE: // GET PITCH TABLE
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xCE;
              for (int i = 0; i < 32; i++) {
                sendBuffer[2 + i] = toneSettings[tno].pitchTable[i];
              }
              sendBuffer[34] = 0xFF;
              Serial.write(sendBuffer, 35);
              Serial.flush();
            }
            break;
        case 0xE1: // WRITE TO FLASH
            if (receiveBuffer[1] == 0x7F) {
              SaveToneSetting(toneNo);
              SendCommandAck();
            }
            break;
        case 0xEE: // SOFTWARE RESET
            SendCommandAck();
            delay(1000);

            if (receiveBuffer[1] == 0x7F) {
              ESP.restart();
            } else if ((receiveBuffer[1] == 0x7A) && (receiveBuffer[2] == 0x3C)) {
              BeginPreferences(); {
                pref.clear(); // CLEAR ALL FLASH MEMORY
              } EndPreferences();
              ESP.restart();
            }
            break;
        case 0xF1: // GET AFUUE VERSION
          sendBuffer[0] = 0xF1;
          sendBuffer[1] = 0x15; // ver1.5
          sendBuffer[2] = 0x01; // protocol version
          sendBuffer[3] = 0xFF;
          Serial.write(sendBuffer, 3);
          Serial.flush();
    }

    waitForCommand = true;
    receivePos = 0;
}

//---------------------------------
void SendCommandAck() {
  Serial.write(0xFE);
  Serial.write(0xFF);
  Serial.flush();
}


//---------------------------------
void SerialProc() {
  int dataSize = Serial.available();
  if (dataSize == 0) return;

  uint8_t buff[256];
  if (dataSize > 256) dataSize = 256;
  int readSize = Serial.readBytes(buff, dataSize);
  for (int i = 0; i < readSize; i++) {
    if (receivePos >= 1000) break;
    int d = buff[i];
    if (waitForCommand)
    {
        if ((d >= 0x80) && (d != 0xFF))
        {
            receivePos = 0;
            receiveBuffer[receivePos++] = d;
            waitForCommand = false;
        }
    }
    else
    {
        if (d < 0x80)
        {
            receiveBuffer[receivePos++] = d;
        }
        else
        {
            OnReceiveCommand();
            if (d < 0xFF)
            {
                receivePos = 0;
                receiveBuffer[receivePos++] = d;
                waitForCommand = false;
            }
        }
    }
  }
}



const short waveWSynth[256] = {
    -11947,-14483,-17018,-19554,-22090,-24625,-26859,-27731,
    -28604,-29476,-30348,-31221,-31703,-31501,-31300,-31099,
    -30897,-30696,-30352,-29891,-29429,-28967,-28506,-28044,
    -27548,-27038,-26529,-26019,-25510,-25001,-24497,-23994,
    -23492,-22989,-22486,-21991,-21569,-21148,-20727,-20306,
    -19884,-19462,-19036,-18610,-18184,-17758,-17332,-16876,
    -16385,-15893,-15402,-14910,-14419,-13958,-13515,-13072,
    -12629,-12185,-11742,-11424,-11133,-10842,-10551,-10261,
    -9970,-9767,-9564,-9362,-9159,-8956,-8751,-8533,
    -8315,-8097,-7879,-7661,-7435,-7196,-6956,-6716,
    -6476,-6236,-5994,-5751,-5507,-5263,-5019,-4776,
    -4554,-4342,-4129,-3917,-3704,-3491,-3320,-3153,
    -2986,-2819,-2653,-2484,-2304,-2124,-1944,-1764,
    -1584,-1382,-1125,-868,-611,-354,-97,163,
    430,697,964,1231,1498,1715,1904,2093,
    2282,2470,2659,2829,2996,3162,3328,3495,
    3661,3848,4036,4223,4411,4598,4782,4947,
    5113,5278,5444,5610,5767,5909,6052,6194,
    6336,6479,6621,6763,6904,7046,7188,7330,
    7458,7582,7706,7830,7954,8078,8199,8319,
    8440,8560,8681,8806,8971,9137,9302,9468,
    9634,9809,10013,10216,10420,10623,10826,11038,
    11261,11484,11706,11929,12151,12343,12516,12690,
    12863,13037,13210,13281,13329,13376,13424,13472,
    13519,13562,13606,13649,13692,13735,13803,13986,
    14169,14352,14534,14717,14880,15006,15133,15259,
    15386,15512,15544,15497,15450,15403,15356,15309,
    15343,15408,15473,15538,15603,15668,15897,16143,
    16389,16634,16880,17115,17242,17368,17495,17621,
    17748,17872,17991,18109,18228,18346,18465,18599,
    18751,18904,19056,19208,19361,18903,18097,17291,
    16485,15678,14872,12654,10121,7589,5057,2524,
};

const short waveFlute[256] = {
  767,509,509,1024,768,806,876,1052,1285,1536,1785,2014,2213,2399,2598,2827,
  3074,3320,3555,3822,4273,4908,5336,5586,5762,5929,6109,6281,6477,6753,7094,7410,
  7703,8024,8394,8771,9166,9582,10088,10665,11211,11769,12458,13214,14072,15032,16159,17486,
  19100,20781,22171,23374,24576,25792,27008,28243,29385,30435,31397,32126,32311,31781,30802,29667,
  28600,27495,26224,24776,23210,21585,20005,18515,17203,16159,15310,14651,14112,13636,13179,12692,
  12258,11868,11540,11224,10831,10325,9858,9507,9226,8966,8709,8452,8193,7918,7604,7252,
  6935,6644,6338,6024,5786,5589,5367,5122,4872,4638,4458,4355,4252,4072,3840,3599,
  3385,3225,3107,3006,2895,2815,2727,2589,2450,2337,2236,2126,2047,1970,1867,1790,
  1713,1610,1533,1456,1354,1277,1200,1100,1040,1010,964,868,733,595,504,403,
  223,-11,-263,-514,-749,-929,-1031,-1134,-1312,-1535,-1749,-1928,-2108,-2326,-2575,-2851,
  -3184,-3593,-4006,-4357,-4694,-5057,-5411,-5820,-6329,-6801,-7177,-7558,-8056,-8667,-9332,-10099,
  -10966,-11869,-12837,-13880,-15026,-16296,-17619,-19027,-20488,-22232,-24299,-26303,-28110,-29699,-31034,-31612,
  -31539,-31100,-30475,-29732,-28793,-27715,-26642,-25605,-24595,-23622,-22658,-21576,-20381,-19246,-18194,-17208,
  -16340,-15646,-15013,-14336,-13651,-13002,-12354,-11719,-11079,-10469,-9920,-9504,-9148,-8786,-8453,-8120,
  -7764,-7443,-7132,-6756,-6309,-5933,-5622,-5301,-4951,-4648,-4412,-4229,-4049,-3831,-3588,-3344,
  -3127,-2947,-2768,-2554,-2331,-2153,-2052,-1957,-1800,-1604,-1394,-1232,-1084,-936,-719,-406,
};

const short waveOboeA[256] = {
  3063,-237,816,-292,-2212,2426,1942,1621,1313,1022,285,1054,863,1413,-587,2412,
  2773,1100,2306,2970,3407,4306,3422,4868,1608,8028,7271,5389,6741,9101,9127,6753,
  5748,2137,6676,5280,5300,10727,10388,10142,10051,8827,12088,12141,10222,15345,15869,14768,
  15723,16572,17596,18507,21697,23293,24854,24828,24230,26496,30141,24056,26045,25479,27576,25810,
  25522,24640,22653,22814,21475,21868,20043,16773,18447,17952,12193,15030,11679,17289,11770,11054,
  12195,8571,7748,8936,11108,9371,7734,10634,8282,9015,8870,5079,4675,3726,5410,5591,
  5361,3384,4189,4419,4720,3438,5846,1114,2261,6111,1400,2955,3814,4145,6984,2132,
  2295,3455,2155,4506,2549,4518,1230,5700,6141,682,3550,611,1137,-2169,3206,-267,
  417,3104,-891,1228,-994,1236,3243,606,-333,2991,1375,687,1515,-2380,241,138,
  -1427,546,371,1026,-735,-1226,-1103,-1751,1192,-1808,-3259,-3651,-2749,-2073,-1793,-66,
  -4340,-2719,-4300,-1883,-5868,-2651,-3073,-5037,-5032,-3203,-6669,-5836,-6358,-7339,-8661,-9523,
  -8847,-9121,-8403,-10875,-14877,-14688,-16396,-15376,-18273,-18216,-23618,-21809,-23337,-22809,-27481,-27869,
  -24459,-26249,-25856,-26470,-25283,-21960,-21108,-21318,-21320,-18684,-18551,-17843,-20115,-11824,-18159,-17459,
  -16485,-13168,-14241,-10644,-13904,-8651,-12491,-8760,-10695,-9993,-6506,-8329,-11316,-7269,-6165,-8925,
  -7671,-9394,-6972,-6524,-2262,-5072,-2329,-5362,-1081,-3386,-5005,-2893,-5680,-2306,-145,-4513,
  -4466,-153,-2895,-2674,-358,-186,-1853,36,-1816,-4813,237,-1929,-2115,1289,-1675,-4002,
};

const short waveCla[256] = {
    -2725, -4620, -6515, -8205, -9429, -10653, -11167, -11228,
    -11289, -10400, -9424, -8404, -7236, -6067, -5213, -4643,
    -4072, -3849, -3695, -3522, -3224, -2926, -2523, -1988,
    -1453, -1125, -868, -640, -929, -1218, -1632, -2270,
    -2908, -3621, -4373, -5124, -5539, -5942, -6163, -5900,
    -5638, -5276, -4842, -4408, -4151, -3917, -3694, -3516,
    -3337, -3208, -3130, -3052, -2849, -2616, -2351, -1813,
    -1275, -780, -345, 89, 378, 609, 830, 612,
    393, -163, -1420, -2677, -4501, -6654, -8807, -10446,
    -12050, -13185, -12849, -12514, -11494, -9907, -8321, -7035,
    -5800, -4678, -4153, -3628, -3319, -3263, -3206, -3520,
    -3945, -4414, -5441, -6467, -7738, -9411, -11083, -12853,
    -14667, -16482, -18033, -19581, -20991, -22070, -23149, -23635,
    -23727, -23818, -23236, -22582, -21959, -21444, -20930, -20619,
    -20496, -20374, -20523, -20731, -20941, -21163, -21386, -21378,
    -21066, -20754, -19956, -18984, -17975, -16123, -14271, -12090,
    -9293, -6496, -3639, -750, 2139, 4240, 6307, 8028,
    8781, 9533, 9689, 9393, 9097, 8661, 8206, 7753,
    7317, 6881, 6334, 5668, 5002, 4735, 4573, 4565,
    6025, 7486, 9250, 11467, 13685, 14758, 15353, 15921,
    13932, 11944, 9593, 6458, 3322, 906, -1071, -3050,
    -3454, -3733, -3830, -3324, -2819, -2345, -1897, -1450,
    -1309, -1225, -1106, -780, -454, 56, 791, 1527,
    2217, 2893, 3541, 3776, 4011, 4010, 3604, 3198,
    2798, 2399, 2001, 2597, 3214, 4136, 5827, 7517,
    9113, 10644, 12175, 12856, 13437, 13838, 13517, 13197,
    12463, 11326, 10189, 8887, 7546, 6241, 5207, 4173,
    3476, 3243, 3010, 3653, 4627, 5629, 7526, 9422,
    11412, 13586, 15759, 17718, 19556, 21394, 23017, 24628,
    26156, 27437, 28718, 29698, 30439, 31179, 31538, 31836,
    31923, 30970, 30017, 28319, 25791, 23264, 20193, 16967,
    13773, 10931, 8088, 5531, 3420, 1309, -829, -2979,
};


const short waveHamo[256] = {
  253,753,634,515,-248,-623,-1263,-1643,-2022,-2782,-3288,-4048,-4300,-4552,-5313,-6326,
  -7340,-9370,-10891,-12413,-14695,-17991,-21542,-20779,-18238,-14428,-12903,-11124,-10361,-9343,-8070,-7306,
  -6796,-6032,-5775,-5264,-5007,-4496,-4238,-3726,-3467,-2955,-2696,-2436,-1921,-1660,-1397,-1133,
  -866,-599,-334,-198,-61,202,466,729,992,1254,1516,1775,2285,2534,3289,3788,
  4287,4532,5284,6034,6784,8041,9044,10556,11815,13077,14848,16620,21694,30070,27781,25238,
  20918,18629,16341,14562,13035,10999,9726,8200,7182,6418,5908,5398,4888,4378,3868,3611,
  3355,3099,2842,2586,2330,2075,1946,1817,1815,1559,1558,1557,1303,1174,1045,1043,
  915,787,786,658,530,276,275,147,19,-236,-237,-365,-492,-747,-1002,-1256,
  -1511,-1638,-1766,-2020,-2275,-2784,-3038,-3293,-3547,-3801,-4310,-4564,-4819,-5328,-5583,-5838,
  -6347,-7111,-7620,-8384,-9402,-9911,-10927,-12452,-13469,-14486,-15504,-16521,-17538,-19063,-20081,-21353,
  -22879,-24914,-26948,-28475,-30509,-31019,-30514,-28994,-26459,-23415,-21387,-19866,-17839,-16066,-14802,-13538,
  -12529,-11520,-10512,-10013,-8498,-8000,-7249,-6752,-6510,-6015,-5778,-5540,-4792,-4551,-4307,-4058,
  -3930,-3800,-3541,-3408,-3274,-3011,-2749,-2614,-2479,-2217,-1956,-1694,-1430,-913,-651,-136,
  125,1146,1659,2426,3701,4722,6251,8541,10831,14899,17950,14145,11862,10595,8313,7807,
  6541,5527,4514,3755,3251,2492,2241,1735,1484,1233,981,728,603,477,225,227,
  102,-24,-22,-148,-273,-144,-17,-16,493,368,242,244,118,-8,-6,-4,
};

const short waveHamoA[256] = {
  -1669,-1304,-786,491,-606,1389,-1574,-2226,-2499,-2598,-1319,-3199,-6414,-4957,-3371,-5106,
  -6688,-7360,-8777,-12097,-12891,-14802,-15649,-16772,-17714,-16303,-14288,-9624,-9244,-10992,-6314,-6154,
  -6024,-4605,-4790,-3315,-4721,-5731,-3388,-2659,-3156,-1824,-2036,-1803,-310,-1315,-419,631,
  -34,-3787,-2586,-2176,40,-1340,1413,3485,2840,2899,1744,1482,2902,1837,4063,3716,
  5818,2196,2613,3151,5869,8958,8535,9545,10467,13178,12379,15469,19122,24203,22846,19777,
  18280,14012,14470,11005,11733,12025,11053,8393,7770,6518,4025,5937,6897,3258,4070,5521,
  6966,4367,6305,4148,1348,2327,1765,3280,2601,1025,60,1659,479,3565,-509,368,
  289,-1853,-1243,-1170,-526,3571,569,165,-367,-2262,247,-441,705,1547,1065,-3076,
  -1910,-1719,-2593,-2085,-1547,-1750,-2571,-4110,-1822,-3326,-4466,-2637,-7391,-4559,-5282,-3846,
  -1888,-6695,-6695,-5610,-7480,-6855,-10309,-9208,-10318,-11552,-13288,-15043,-16137,-15719,-16982,-17731,
  -22735,-24748,-21763,-25447,-27338,-25954,-24908,-23569,-22248,-19193,-16778,-17627,-15321,-12107,-10929,-10145,
  -8766,-10034,-11560,-7522,-8714,-4017,-3747,-5168,-6730,-5168,-5019,-5293,-5469,-6632,-4922,-4618,
  -3498,-5303,-3381,124,-856,-3549,-3757,-3722,-1204,63,-615,-3074,-2351,-3184,1713,546,
  1797,-1081,-1205,3511,4252,5219,4667,8142,9515,13053,12640,13582,11670,9882,9271,3870,
  3950,4897,5193,3772,2453,1736,3467,-529,-889,996,-1577,-901,1783,240,-1485,-2164,
  -91,-653,-591,147,1073,-2424,291,-1091,135,749,1848,985,1593,-918,410,-910,
};

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
    ts->reverb = 7;
    ts->portamento = 2;
    ts->keySense = 5;
    ts->breathSense = 4;

    if (idx == 0) {
      for (int i = 0; i < 256; i++) ts->waveA[i] = waveWSynth[i];
      for (int i = 0; i < 256; i++) ts->waveB[i] = waveWSynth[i];
    }
    else if (idx == 1) {
      ts->transpose = 3;
      for (int i = 0; i < 256; i++) ts->waveA[i] = waveCla[i];
      for (int i = 0; i < 256; i++) ts->waveB[i] = waveCla[i];
    }
    else if (idx == 2) {
      ts->breathSense = 6;
      for (int i = 0; i < 256; i++) ts->waveA[i] = waveFlute[i];
      for (int i = 0; i < 256; i++) ts->waveB[i] = waveFlute[i];
    }
    else if (idx == 3) {
      ts->reverb = 15;
      for (int i = 0; i < 256; i++) ts->waveA[i] = waveHamo[i];
      for (int i = 0; i < 256; i++) ts->waveB[i] = waveHamo[i];
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
double SawWave(double p) {
  return -1.0 + 2 * p;
}

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
uint8_t CreateWave() {
    // double 演算で波形のフェーズを 0-255 で算出する
    for (int i = 0; i < 3; i++) {
      phase[i] += fst[i];
      if (phase[i] >= 1.0) phase[i] -= 1.0;
    }
    // 波形を生成する
    //double g = Voice(phase);
    double g = Voice(phase[0]) * volBase[0] + Voice(phase[1]) * volBase[1] + Voice(phase[2]) * volBase[2];
    
    //ノイズ
    double n = ((double)rand() / RAND_MAX);
    double e = 150 * (g * (1-noise) + n * noise) * volReq + reverbBuffer[reverbPos];;
    //double e = 150 * n * volReq + reverbBuffer[reverbPos];;
    
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
    uint8_t dac = CreateWave();
    dacWrite(DAC1, dac);
    interruptCounter++;
  }
  portEXIT_CRITICAL_ISR(&timerMux);
  //xSemaphoreGiveFromISR(timerSemaphore, NULL);
}


//---------------------------------
void setup() {
#if !ENABLE_MIDI
  Serial.begin(115200);
  SerialPrintLn("------");
#endif

  Wire.begin();
  SerialPrintLn("wire begin");

  ledcSetup(0, 12800, 8);
  ledcAttachPin(A12, 0);

  ledcWrite(0, 255);
  delay(500);
  ledcWrite(0, 40);
  delay(500);

  const int portList[12] = { 13, 12, 14, 27, 26, 16, 17, 5,18, 19, 23, 3 };
  for (int i = 0; i < 12; i++) {
    pinMode(portList[i], INPUT_PULLUP);
  }

  BeginPreferences(); {
    bool keyLowC = digitalRead(16);
    bool keyEb = digitalRead(17);
    bool keyD = digitalRead(5);
    bool keyE = digitalRead(18);
    bool keyF = digitalRead(19);
    bool keyLowCs = digitalRead(13);
    bool keyGs =digitalRead(12);
    bool keyG = digitalRead(14);
    bool keyA = digitalRead(27);
    bool keyB = digitalRead(26);
    bool octDown = digitalRead(3);
    bool octUp = digitalRead(23);
    // play key setting
    bootBaseNote = 0;
  
    // baseNote はキー全開放時の音 なので通常 C#
    if ((keyE == LOW)&&(keyF == LOW)) {
      bootBaseNote = 60 + 1; // 61  C (C#)
    }
    else if (keyE == LOW) {
      bootBaseNote = 58 + 1; // 59  Bb (B)
    }
    else if (keyF == LOW) {
      bootBaseNote = 63 + 1; // 64  Eb (E)
    }
  
    // play voice setting
    if ((keyLowCs == HIGH) && (keyGs == HIGH) && (keyG == HIGH) && (keyA == HIGH) && (keyB == LOW)) {
      toneNo = 0;
    }
    else if ((keyLowCs == HIGH) && (keyGs == HIGH) && (keyG == HIGH) && (keyA == LOW) && (keyB == HIGH)) {
      toneNo = 1;
    }
    else if ((keyLowCs == HIGH) && (keyGs == HIGH) && (keyG == LOW) && (keyA == HIGH) && (keyB == HIGH)) {
      toneNo = 2;
    }
    else if ((keyLowCs == HIGH) && (keyGs == LOW) && (keyG == HIGH) && (keyA == HIGH) && (keyB == HIGH)) {
      toneNo = 3;
    }
    else if ((keyLowCs == LOW) && (keyGs == HIGH) && (keyG == HIGH) && (keyA == HIGH) && (keyB == HIGH)) {
      toneNo = 4;
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
    metronome_v = (uint8_t)pref.getInt("MetroVolume", 10);
    if (keyLowC == LOW) {
      metronome = true;
    }
    volBase[0] = 1.0;
    volBase[1] = 0.0;
    volBase[2] = 0.0;
    if (keyEb == LOW) {
      volBase[0] = 0.5;
      volBase[1] = 0.5;
      volBase[2] = 0.5;
    }
  
    if (keyD == LOW) {
      midiEnabled = true;
    }
  } EndPreferences();
  
  //SerialPrintLn("---baseNote");
  //SerialPrintLn(baseNote);
  //SerialPrintLn("---toneNo");
  //SerialPrintLn(toneNo);

  defPressure = getPressure();
  delay(100);
  defTemperature = getTemperature();

  ledcWrite(0, 255);
  delay(500);
  ledcWrite(0, 0);
  delay(500);

  for (int i = 0; i < REVERB_BUFFER_SIZE; i++) {
    reverbBuffer[i] = 0.0;
  }

  if (midiEnabled) {
#if ENABLE_MIDI
    SerialPrintLn("hardware serial begin");
    pinMode(1, OUTPUT);
    pinMode(4, INPUT);
    SerialHW.begin(31250, SERIAL_8N1, 4, 1);
    deviceConnected = true;

    delay(500);
    MIDI_NoteOn(baseNote + 1, 32);
    delay(500);
    MIDI_NoteOff();
#endif
#if ENABLE_BLE_MIDI
    BLEDevice::init(version);
      
    // Create the BLE Server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
  
    // Create the BLE Service
    BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));
  
    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
      BLEUUID(CHARACTERISTIC_UUID),
      BLECharacteristic::PROPERTY_READ   |
      BLECharacteristic::PROPERTY_WRITE  |
      BLECharacteristic::PROPERTY_NOTIFY |
      BLECharacteristic::PROPERTY_WRITE_NR
    );
  
    // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
    // Create a BLE Descriptor
    pCharacteristic->addDescriptor(new BLE2902());
  
    // Start the service
    pService->start();
  
    // Start advertising
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->start();
  
    SerialPrintLn("BLE MIDI begin");
#endif
  }
  else
  {
#if ENABLE_MIDI
  Serial.begin(115200);
#endif
    timerSemaphore = xSemaphoreCreateBinary();
    // Use 1st timer of 4 (counted from zero).
    // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
    // info).
    timer = timerBegin(0, 80, true);  // 1us increment
    timerAttachInterrupt(timer, &onTimer, true);
    // Set alarm to call onTimer function every second (value in microseconds).
    // Repeat the alarm (third parameter)
    //timerAlarmWrite(timer, 300*1000, true);
    timerAlarmWrite(timer, 50, true); //20kHz
    timerAlarmEnable(timer);
    SerialPrintLn("timer begin");
  }
  SerialPrintLn("setup done");
}

