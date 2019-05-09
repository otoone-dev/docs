#include <avr/io.h>
#include <wiring_private.h>
#include <Wire.h>
#include <EEPROM.h>

#define ENABLE_BMP180 (1)

#define TEMP_RATE (-0.55f)

#define BMP180_ADDR 0x77 // 7-bit address
#define BMP180_REG_CONTROL 0xF4
#define BMP180_REG_RESULT 0xF6
#define BMP180_COMMAND_TEMPERATURE 0x2E
#define BMP180_COMMAND_PRESSURE0 0x34
#define BMP180_COMMAND_PRESSURE1 0x74
#define BMP180_COMMAND_PRESSURE2 0xB4
#define BMP180_COMMAND_PRESSURE3 0xF4
static int16_t AC1,AC2,AC3,VB1,VB2,MB,MC,MD;
static uint16_t AC4,AC5,AC6; 
static double c5,c6,mc,md,x0,x1,x2,y0,y1,y2,p0,p1,p2;
static char _error;
static uint8_t initCnt = 100;

static uint32_t defPressure = 0;
static uint32_t defTemperature = 0;

#define fs (78125.0f / 2.0f)
static float fstd = 0.0f;
volatile float fst = 0.0f;

volatile float phase = 0.0f;
volatile uint16_t volReq = 0x00;
static float vol = 0.0f;
static float pitch = 0.0f;

static float currentNote = 60.0f;
static int baseNote = 0;
static byte toneNo = 0;

#define KeyLowC (1<<2) // PC2
#define KeyEb   (1<<1) // PC1
#define KeyD    (1<<0) // PC0

#define StatusLED (1<<5) // PB5
#define KeyE    (1<<2) // PB2
#define KeyF    (1<<1) // PB1

#define KeyLowCs (1<<0) // PD0
#define KeyGs   (1<<1) // PD1
#define KeyG    (1<<2) // PD2
#define SpOut   (1<<3) // PD3
#define KeyA    (1<<4) // PD4
#define KeyB    (1<<5) // PD5
#define KeyDown (1<<6) // PD6
#define KeyUp   (1<<7) // PD7

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
uint32_t getPressure() {
  unsigned char wdata[2] = { BMP180_REG_CONTROL, BMP180_COMMAND_PRESSURE0 };
  if (writeBytes(wdata, 2)) {
    delay(5);
    unsigned char rdata[3] = { BMP180_REG_RESULT, 0, 0 };
    if (readBytes(rdata, 3)) {
      return (((uint32_t)rdata[0]) << 8) | (uint32_t)rdata[1];
    }
  }
  return 0;
}

//---------------------------------
int32_t getTemperature() {
  unsigned char wdata[2] = { BMP180_REG_CONTROL, BMP180_COMMAND_TEMPERATURE };
  if (writeBytes(wdata, 2)) {
    delay(5);
    unsigned char rdata[2] = { BMP180_REG_RESULT, 0 };
    if (readBytes(rdata, 2)) {
      return (int32_t)((((uint32_t)rdata[0]) << 8) | (uint32_t)rdata[1]);
    }
  }
  return 0;
}

//---------------------------------
void SetupTimer() {
  cli();

#if 0
  // Non prescaler 8bit-PWM (78kHz)
  TCCR1B |= _BV(CS10);
  TCCR1B &= ~_BV(CS11);
  TCCR1B &= ~_BV(CS12);   // non prescaler
  analogWrite(9, 0x10);
  
  TIMSK1 |= _BV(TOIE1); // overflow interrupt
#endif
  //TCCR2A |= _BV(WGM20); // fastPWM
  //TCCR2A |= _BV(WGM21);
  //TCCR2B &= ~_BV(WGM22); // top=0xff

  //TCCR2A &= ~_BV(COM2A0);
  //TCCR2A &= ~_BV(COM2A1); // OC2A disable
  //TCCR2A |= _BV(COM2B0);
  //TCCR2A |= _BV(COM2B1);  // OC2B compare match HIGH
  analogWrite(3, 0x10);

  // non prescaler 8bit-PWM (78kHz)
  TCCR2B |= _BV(CS20);
  TCCR2B &= ~_BV(CS21);
  TCCR2B &= ~_BV(CS22);

  TIMSK2 |= _BV(TOIE2); // overflow interrupt

  sei();    // 割り込み許可
}

//---------------------------------
void SetupBMP180() {
  if (readInt(0xAA,AC1) &&
    readInt(0xAC,AC2) &&
    readInt(0xAE,AC3) &&
    readUInt(0xB0,AC4) &&
    readUInt(0xB2,AC5) &&
    readUInt(0xB4,AC6) &&
    readInt(0xB6,VB1) &&
    readInt(0xB8,VB2) &&
    readInt(0xBA,MB) &&
    readInt(0xBC,MC) &&
    readInt(0xBE,MD))
  {
    double c3,c4,b1;
    c3 = 160.0 * pow(2,-15) * AC3;
    c4 = pow(10,-3) * pow(2,-15) * AC4;
    b1 = pow(160,2) * pow(2,-30) * VB1;
    c5 = (pow(2,-15) / 160) * AC5;
    c6 = AC6;
    mc = (pow(2,11) / pow(160,2)) * MC;
    md = MD / 160.0;
    x0 = AC1;
    x1 = 160.0 * pow(2,-13) * AC2;
    x2 = pow(160,2) * pow(2,-25) * VB2;
    y0 = c4 * pow(2,15);
    y1 = c4 * c3;
    y2 = c4 * b1;
    p0 = (3791.0 - 8.0) / 1600.0;
    p1 = 1.0 - 7357.0 * pow(2,-20);
    p2 = 3038.0 * 100.0 * pow(2,-36);
    
    Wire.beginTransmission(BMP180_ADDR);
  }
}

//---------------------------------
int getNoteNumber() {
  bool keyLowC = (PINC & KeyLowC);
  bool keyEb = (PINC & KeyEb);
  bool keyD = (PINC & KeyD);
  bool keyE = (PINB & KeyE);
  bool keyF = (PINB & KeyF);
  bool keyLowCs = (PIND & KeyLowCs);
  bool keyGs = (PIND & KeyGs);
  bool keyG = (PIND & KeyG);
  bool keyA = (PIND & KeyA);
  bool keyB = (PIND & KeyB);
  bool octDown = (PIND & KeyDown);
  bool octUp = (PIND & KeyUp);

  int bnote = 0;
  if (keyA == LOW) bnote = 1;
  if (keyB == LOW) bnote |= 2;   // 0x00:C#, 0x01:C, 0x02:B, (0x03:A)
  if (bnote == 3) bnote = 4;

  int note = baseNote - bnote;
  
  if ((bnote == 1) && (keyG == LOW)) note += 9;  // +12-3
  if (keyG == LOW) note -= 2;
  if (keyGs == LOW) note++;
  if (keyF == LOW) note -= 2;
  if (keyE == LOW) note--;
  if (keyD == LOW) note -= 2;
  if (keyEb == LOW) note++;
  if (keyLowC == LOW) note -=2;
  if (keyLowCs == LOW) note--;

  if (octDown == LOW) note -= 12;
  if (octUp == LOW) note += 12;

  static bool octHiLoPushing = false;
  if ((octDown == LOW)&&(octUp == LOW)) {
    if (octHiLoPushing == false) {
#if false
      if (keyB == LOW) channel = 1;
      if (keyA == LOW) channel = 2;
      if (keyG == LOW) channel = 3;
      if (keyOpt == LOW) {
        play = 0;
        vol = volReq = 0;
        bend = 0;
        bendReq = 0;
        defPressure = getPressure();
        defTemperature = getTemperature();
        Serial.println(" ");
        Serial.println(defPressure);
        Serial.println(defTemperature);
      }
#endif
    }
    octHiLoPushing = true;
  } else {
    octHiLoPushing = false;
  }
  
  return note;
}

//---------------------------------
void setupPorts() {
  DDRB = DDRB | B00100000; // PB5 LED
  DDRC = DDRC | B00000000;
  DDRD = DDRD | B00001000; // PD3 SpOut
  PORTB = PORTB | B00000110; // PB1,PB2 PullUp  
  PORTC = PORTC | B00000111; // PC0,PC1,PC2 PullUp
  PORTD = PORTD | B11110111; // PD0,1,2,4,5,6,7 PullUp  
}

//---------------------------------
void setup() {
  // put your setup code here, to run once:
  setupPorts();

  baseNote = 49 + 12 + 12; // C (C#)
  bool octDown = (PIND & KeyDown);
  bool octUp = (PIND & KeyUp);
  
#if true
  if ((octDown == LOW)&&(octUp == LOW)) {
    baseNote = 49 + 12 + 12 + 12; // Hi-C (Hi-C#)
  }
  else if (octDown == LOW) {
    baseNote = 47 + 12 + 12; // Bb (B)
  }
  else if (octUp == LOW) {
    baseNote = 52 + 12 + 12; // Eb (E)
  }
#else
  if ((octDown == LOW)&&(octUp == LOW)) {
    while (1) {
        bool keyLowC = (PINC & KeyLowC);
        if (keyLowC == false) {
          digitalWrite(LED_BUILTIN, HIGH);
        } else {
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
          digitalWrite(LED_BUILTIN, LOW);
          delay(100);
        }
    }
  }
#endif

  Wire.begin();
  setupPorts();
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);

  {
    bool keyLowC = (PINC & KeyLowC);
    bool keyEb = (PINC & KeyEb);
    //bool keyD = (PINC & KeyD);
    if (keyLowC == false) toneNo = 1;
    if (keyEb == false) toneNo = 2;
  }

#if ENABLE_BMP180
  SetupBMP180();
#endif

#if ENABLE_BMP180
  defPressure = getPressure();
  delay(100);
  defTemperature = getTemperature();
#endif

  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);

  SetupTimer();

#if false
  while (true) {
    while (1) {
        bool keyLowC = (PINC & KeyLowC);
        if (keyLowC == false) {
          digitalWrite(LED_BUILTIN, HIGH);
        } else {
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
          digitalWrite(LED_BUILTIN, LOW);
          delay(100);
        }
    }
  }
#endif
}

//--------------------------
float CalcInvFrequency(float note) {
  return 440.0f * pow(2, (note - (69-12))/12);
}

//--------------------------
void loop() {
#if ENABLE_BMP180
  int32_t tmp = getTemperature() - defTemperature;
  uint32_t prsZero = 10 + defPressure + (uint32_t)(tmp * TEMP_RATE);
  uint32_t prs = getPressure();
#else
  uint32_t prsZero = 0;
  uint32_t prs = 0;
#endif
  int32_t p = 0;
  if (prs > prsZero) p = (int32_t)(prs - prsZero);

  const int maxPressure = 400;
  if (p > maxPressure) p = maxPressure;
  float pp = p / (float)maxPressure;
  if (pp > 1.0f) pp = 1.0f;
  if (initCnt > 0) {
    initCnt--;
    pp = 0.2f;
    currentNote = 69;
    fst = fstd;
    delay(1);
  } else {
    currentNote = getNoteNumber();
  }
  
  float ppReq = pp*pp;

#if 0
  {
    bool octUp = (PIND & KeyUp);
    bool octDown = (PIND & KeyDown);
    if ((octDown == false) && (octUp == false)) {
      ppReq = 1.0f;
    }
  }
#endif
  vol += (ppReq - vol) * 0.4f;
  volReq = (uint16_t)(vol * 255);
#if 0
  if (volReq > 0) {
    if (pitch < 0.0f) pitch += ppReq;
    else pitch = 0.0f;
  } else {
    if (pitch > -0.4f) pitch -= 0.01f;
  }
#else
  //pitch = (vol - 1.0f) * 0.5f;
#endif

  float ft = CalcInvFrequency(currentNote + pitch);
  fstd = (ft / fs) * 256;
  fst += (fstd - fst) * 0.9;
  //fst = fstd;

  if (volReq > 0) {
        digitalWrite(LED_BUILTIN, HIGH);
  } else {
        digitalWrite(LED_BUILTIN, LOW);
  }
}

volatile uint8_t count = 0;
volatile uint8_t phase8 = 0;
//--------------------------
//ISR(TIMER1_OVF_vect) {
ISR(TIMER2_OVF_vect) {
  // 78125Hz を 2回 に分けて 39062.5Hz で波形を作る
  count = 1 - count;

  if (count == 0) {
    // 1 回目は処理負荷の高い float 演算で波形のフェーズを 0-255 で算出する
    phase += fst;
    if (phase >= 256.0f) phase -= 256.0f;
      phase8 = (uint8_t)phase;
  }
  else {
    // 2 回目は波形を生成する
    int32_t e = 0;

#if 1
    switch (toneNo) {
      default:  //のこぎり波
        {
          e = phase8 - 0x80;
        }
        break;
      case 1: //三角波
        {
          if (phase8 < 0x80) {
            e = phase8 * 2;          
          } else {
            e = 0xFF - (phase8 - 0x80)*2;
          }
        }
        break;
      case 2:  //方形波
        {
          if (phase8 > 0x80) e = 0xFF;
          else e = 0x00;
        }
        break;
    }
#else
          if (phase8 > 0x80) e = 0xFF;
          else e = 0x00;
#endif
    int32_t f = e * 256;
    if (f > 32767) f = 32767;
    if (f < -32767) f = -32767;

    uint16_t g = 32767;
    g += f;
    
    OCR2B = (uint8_t)((volReq * (g>>8)) >> 8);

#if 0
    if (phase < volReq) {
          digitalWrite(LED_BUILTIN, HIGH);
    } else {
          digitalWrite(LED_BUILTIN, LOW);
    }
#endif
  }

}
