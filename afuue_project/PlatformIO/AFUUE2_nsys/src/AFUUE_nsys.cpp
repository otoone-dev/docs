#include <Arduino.h>
#include <M5Unified.h>
#define CORE0 (0)
#define CORE1 (1)

//-----------
#include <Wire.h>
#define I2CPIN_SDA (38)
#define I2CPIN_SCL (39)
#define I2C_FREQ (400000)

#define LPS33_ADDR (0x5C)
#define LPS33_WHOAMI_REG (0x0F)
#define LPS33_WHOAMI_RET (0xB1)
#define LPS33_REG_CTRL (0x10)
#define LPS33_REG_OUT_XL (0x28)

//-----------
#include <USB.h>
#include <USBMIDI.h>
USBMIDI MIDI("AFUUE2R");
 
//-----------
#include <FastLED.h>
#define LED_PIN   (35)
#define NUM_LEDS  (1)
static CRGB leds[NUM_LEDS];
void setLed(CRGB color) {
  uint8_t t = color.r;
  color.r = color.g;
  color.g = t;
  leds[0] = color;
  FastLED.show();
}

//---------------------
#define SOUND_TWOPWM
#define PWMPIN_LOW (5)
#define PWMPIN_HIGH (6)
#define MIDI_IN_PIN (9)  // not use
#define MIDI_OUT_PIN (7)
#define LEDPIN (8)

volatile uint8_t waveOutL = 0;
volatile uint8_t waveOutH = 0;
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
#define QUEUE_LENGTH 1
volatile QueueHandle_t xQueue;
TaskHandle_t taskHandle;

#define CLOCK_DIVIDER (80)
#define TIMER_ALARM (40)
//#define SAMPLING_RATE (20000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*50) = 20kHz
#define SAMPLING_RATE (25000.0f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*40) = 25kHz
//#define SAMPLING_RATE (32000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (50*50) = 32kHz
//#define SAMPLING_RATE (44077.13f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (55*33) = 44kHz
//#define SAMPLING_RATE (48019.2f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (49*34) = 48kHz

//---------------------------------
void IRAM_ATTR OnTimer() {

  portENTER_CRITICAL_ISR(&timerMux);
    uint8_t h = waveOutH;
    uint8_t l = waveOutL;

    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(taskHandle, 0, eNoAction, &higherPriorityTaskWoken);
  portEXIT_CRITICAL_ISR(&timerMux);

  #ifdef SOUND_TWOPWM
  ledcWriteChannel(0, l);
  ledcWriteChannel(1, h);
#else
  //dacWrite(DACPIN, h);
#endif
}

//---------------------
class System {
public:
  void initialize() {
    FastLED.addLeds<WS2811, LED_PIN, RGB>(leds, NUM_LEDS);
    setLed(CRGB(100, 0, 0));
    delay(200);
    setLed(CRGB(1, 0, 0));
    delay(200);
    auto cfg = M5.config();
    M5.begin(cfg);
    setLed(CRGB(200, 0, 0));
    delay(200);
    setLed(CRGB(1, 0, 0));
    delay(500);

    Serial.begin(115200);
    MIDI.begin();
    USB.begin();
    delay(1000);
    if (tud_mounted()) {
      setLed(CRGB(0, 100, 0));
      delay(200);
      setLed(CRGB(0, 0, 0));
      delay(200);
    }
    Wire.begin(I2CPIN_SDA, I2CPIN_SCL, I2C_FREQ);
    {
      M5.Lcd.setTextColor(TFT_WHITE);
      M5.Lcd.setTextSize(2);

      Wire.beginTransmission(LPS33_ADDR);
      Wire.write(LPS33_WHOAMI_REG);
      Wire.endTransmission();
      Wire.requestFrom(LPS33_ADDR, 1);
      uint8_t whoami = Wire.read();
      if (whoami != LPS33_WHOAMI_RET) {
        while (1) {
          setLed(CRGB(100, 0, 0));
          delay(200);
          setLed(CRGB(0, 0, 0));
          delay(200);
        }
      }
      Wire.beginTransmission(LPS33_ADDR);
      Wire.write(LPS33_REG_CTRL);
      Wire.write(0x50); // odr (0x50=75Hz, 0x40=50Hz, 0x30=25Hz, 0x20=10Hz, 0x10=1Hz, 0x00=OFF)
      Wire.endTransmission();
      delay(100);
      m_defaultPressure = GetPressure(0);
#if 0
      while (1) {
        delay(100);
        Wire.beginTransmission(LPS33_ADDR);
        Wire.write(LPS33_REG_OUT_XL);
        Wire.endTransmission();
        Wire.requestFrom(LPS33_ADDR, 3);

        uint8_t data[3];
        for (int i = 0; i < 3; i++) {
          data[i] = Wire.read();
        }        
        uint32_t pressure = ((((uint32_t)data[2]) << 16) | (((uint32_t)data[1]) << 8) | data[0]);
        M5.Lcd.clear(TFT_DARKGREEN);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.printf("%02X %02X %02x\n", data[2], data[1], data[0]);
        M5.Lcd.printf("%4.1f hPa", pressure / 4096.0f);
      }
#endif
    }

    pinMode(PWMPIN_LOW, OUTPUT);
    pinMode(PWMPIN_HIGH, OUTPUT);
    pinMode(LEDPIN, OUTPUT);
    ledcAttachChannel(PWMPIN_LOW, 156250, 8, 0); // PWM 156,250Hz, 8Bit(256段階)
    ledcAttachChannel(PWMPIN_HIGH, 156250, 8, 1); // PWM 156,250Hz, 8Bit(256段階)
    ledcAttachChannel(LEDPIN, 156250, 8, 5); // PWM 156,250Hz, 8Bit(256段階)
    ledcWriteChannel(0, 0);
    ledcWriteChannel(1, 0);
    ledcWriteChannel(5, 0);

    xQueue = xQueueCreate(QUEUE_LENGTH, sizeof(int8_t));
    xTaskCreatePinnedToCore(CreateWaveTask, "CreateWaveTask", 16384, this, configMAX_PRIORITIES-1, &taskHandle, CORE0); // 波形生成は Core0 が専念
    xTaskCreatePinnedToCore(UpdateTask, "UpdateTask", 2048, this, 2, NULL, CORE1);

    timer = timerBegin(80*1000*1000 / CLOCK_DIVIDER);
    timerAttachInterrupt(timer, &OnTimer);
    timerAlarm(timer, TIMER_ALARM, true, 0);    
  }
  bool m_btnPressed = false;
  void SetTickCount(float v) {
    m_tickCount = v;
  }
  void SetVolume(float v) {
    m_volume = v;
  }
  float GetVolume() const {
    return m_volume;
  }

private:
  float m_tickCount = 0.0f;
  float m_volume = 0.0f;
  float m_defaultPressure = 0.0f;

  float GetPressure(uint8_t side) {
    Wire.beginTransmission(LPS33_ADDR + side);
    Wire.write(LPS33_REG_OUT_XL);
    Wire.endTransmission();
    Wire.requestFrom(LPS33_ADDR + side, 3);

    uint8_t data[3];
    for (int i = 0; i < 3; i++) {
      data[i] = Wire.read();
    }        
    uint32_t pressure = ((((uint32_t)data[2]) << 16) | (((uint32_t)data[1]) << 8) | data[0]);
    return pressure / 256.0f;
  }

  static float CalcFrequency(float note) {
    return 440.0f * pow(2, (note - (69.0f-12.0f))/12.0f);
  }

  static void UpdateTask(void *parameter) {
    System *pSystem = static_cast<System *>(parameter);
    while (1) {
      M5.update();

      float p = pSystem->GetPressure(0);
      float v = (p - pSystem->m_defaultPressure) / 500.0f;
      v *= v;
      if (v < 0.0f) {
        v = 0.0f;
      }
      else if (v > 1.0f) {
        v = 1.0f;
      }

      if (M5.BtnA.isPressed()) {
        pSystem->m_btnPressed = true;
        //noteOn(60, 127);
        //setLed(CRGB(0, 100, 0));
        //delay(500);
        //noteOff();
        v = 0.2f;
      }
      else {
        pSystem->m_btnPressed = false;
      }
      pSystem->SetVolume(v);
      float wavelength = CalcFrequency(60);
      pSystem->SetTickCount(wavelength / SAMPLING_RATE);

      static int count = 0;
      setLed(CRGB(0, 0, (count < 60 ? count : 120-count)));
      count = (count + 1) % 120;
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }

  static void CreateWaveTask(void *parameter) {
    System *pSystem = static_cast<System *>(parameter);
    float phase = 0.0f;
    while (1) {
      uint32_t data;
      xTaskNotifyWait(0, 0, &data, portMAX_DELAY);

      phase += pSystem->m_tickCount;
      if (phase >= 1.0f) phase -= 1.0f;

      float f = 60000.0f * (pSystem->GetVolume() * (-0.5f + phase));
      uint16_t d = static_cast<uint16_t>(32768.0f + f);

      uint8_t h = (d >> 8) & 0xFF;
      uint8_t l = d & 0xFF;
      portENTER_CRITICAL(&timerMux);
        waveOutH = h;
        waveOutL = l;
      portEXIT_CRITICAL(&timerMux);
    }
  }
};
System sys;

//---------------------
void setup() {
  sys.initialize();
}

//---------------------
static int prev_note = 0;
void noteOn(uint8_t note, uint8_t vol) {
  int channelNo = 0;
  //midiEventPacket_t packet = {0x90 + channelNo, 0x90 + channelNo, note, vol};
  //MIDI.writePacket(&packet);
  MIDI.noteOn(note, vol);
  midiEventPacket_t packet2 = {0xB0 + channelNo, 0xB0 + channelNo, 0x02, vol};
  MIDI.writePacket(&packet2);
  prev_note = note;
}

void noteOff() {
  int channelNo = 0;
  //midiEventPacket_t packet = {0x80 + channelNo, 0x80 + channelNo, prev_note, 0};
  //MIDI.writePacket(&packet);
  MIDI.noteOff(prev_note, 0);
}

//---------------------
void loop() {
  //midiEventPacket_t midi_packet_in = {0, 0, 0, 0};

  //if (MIDI.readPacket(&midi_packet_in)) {
  //  setLed(CRGB(0, 0, 100));
  //  delay(500);
  //}
  static int loopCount = 0;
  M5.Lcd.clear(TFT_BLACK);
  M5.Lcd.setCursor(0, 0);
  float v = sys.GetVolume();
  M5.Lcd.printf("%d\n%1.2f", loopCount++, v);
  delay(500);
  ledcWriteChannel(5, (int)(v * 255));
}
