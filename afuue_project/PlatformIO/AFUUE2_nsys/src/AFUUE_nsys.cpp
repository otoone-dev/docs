#include "DeviceBase.h"
#include "InputDeviceBase.h"
#include "InputDevicePressure.h"
#include "InputDeviceKey.h"
#include "OutputDeviceBase.h"
#include "OutputDeviceSpeaker.h"
#include "OutputDeviceLED.h"

#include <M5Unified.h>
#include <Arduino.h>
#include <WiFi.h>
#include <vector>

//-----------
#include <Wire.h>
#define I2CPIN_SDA (38)
#define I2CPIN_SCL (39)
#define I2C_FREQ (400000)

//-----------
#include <USB.h>
#include <USBMIDI.h>
USBMIDI MIDI("AFUUE2R");
 
//-----------
#include <FastLED.h>
#define HAS_DISPLAY
#if !defined(HAS_DISPLAY)
#define FASTLED_PIN   (35)
#endif
//---------------------
#define MIDI_IN_PIN (9)  // not use
#define MIDI_OUT_PIN (7)

#define BUTTON_PIN (41)

char debugMessage[512] = "";

//---------------------
class System {
public:
    float m_cpuLoad = 0.0f;
    bool m_btnPressed = false;

    //--------------
    void initialize() {
        InitLED();
        SetLED(CRGB(100, 0, 0));
        delay(200);
        SetLED(CRGB(1, 0, 0));
        delay(200);
#ifdef HAS_DISPLAY
        auto cfg = M5.config();
        M5.begin(cfg);
#endif
        ClearDisplay(2);
        Display(" AFUUE2R");
        delay(500);

        SetLED(CRGB(200, 0, 0));
        delay(200);
        SetLED(CRGB(1, 0, 0));
        delay(500);

        ClearDisplay(1);

        Serial.begin(115200);
        MIDI.begin();
        USB.begin();
        delay(1000);
        if (tud_mounted()) {
            Display("USB MIDI");
            SetLED(CRGB(0, 100, 0));
            delay(200);
            SetLED(CRGB(0, 0, 0));
            delay(200);
        }
        else {
            Display("PLAY MODE");
        }

        btStop();
        WiFi.mode(WIFI_OFF); 
        Wire.begin(I2CPIN_SDA, I2CPIN_SCL, I2C_FREQ);
        m_inputDevices.push_back(new InputDevicePressure(Wire));
        m_inputDevices.push_back(new InputDeviceKey(Wire));
        m_outputDevices.push_back(new OutputDeviceSpeaker());
        m_outputDevices.push_back(new OutputDeviceLED());

        std::string errorMessage = "";
        // Input Devices
        for (auto& device : m_inputDevices) {
            Display(device->GetName());
            auto result = device->Initialize();
            if (!result.success) {
                errorMessage += result.errorMessage + "\n";
            }
        }
        // Output Devices
        for (auto& device : m_outputDevices) {
            Display(device->GetName());
            auto result = device->Initialize();
            if (!result.success) {
                errorMessage += result.errorMessage + "\n";
            }
        }
        if (errorMessage != "") {
            Display(errorMessage.c_str(), TFT_RED);
            while (1) {
                delay(1000);
            }
        }

        xTaskCreatePinnedToCore(UpdateTask, "UpdateTask", 4096, this, 1, NULL, CORE1);

        Display("READY");
    }
    //--------------
#ifdef FASTLED_PIN
    void SetLED(CRGB rgb) {
        uint8_t t = color.r;
        color.r = color.g;
        color.g = t;
        leds[0] = color;
        FastLED.show();
    }
#else
    void SetLED(CRGB rgb) {}
#endif

private:
    std::vector<InputDeviceBase*> m_inputDevices;
    std::vector<OutputDeviceBase*> m_outputDevices;
    Parameters m_parameters;
    int m_counter = 0;

    //--------------
#ifdef FASTLED_PIN
    void InitLED() {
        FastLED.addLeds<WS2811, LED_PIN, RGB>(leds, NUM_LEDS);
    }
#else
    void InitLED() {}
#endif
    //--------------
    void ClearDisplay(float textSize) {
#ifdef HAS_DISPLAY
        M5.Lcd.setTextColor(TFT_WHITE);
        M5.Lcd.setTextSize(textSize);
        M5.Lcd.clearDisplay(TFT_BLACK);
        M5.Lcd.setCursor(0, 0);
#endif
    }
    //--------------
    void Display(const char* message, int color = TFT_WHITE) {
#ifdef HAS_DISPLAY
        M5.Lcd.setTextColor(color);
        M5.Lcd.printf("%s\n", message);
        delay(100);
#endif
    }

    //--------------
    void Update() {
        float v = 0.0f;
        float note = 0.1f;
        // 入力
        for (auto& device : m_inputDevices) {
            InputResult result = device->Update(m_parameters);
            if (result.hasPressure) {
                v = result.pressure;
                sprintf(debugMessage, "%s", result.debugMessage.c_str());
            }
            if (result.hasNote) {
                note = result.note;
            }
        }
        if (m_btnPressed) {
            v = 0.2f;
        }
        // 出力
        for (auto& device : m_outputDevices) {
            OutputResult ret = device->Update(m_parameters, note, v);
            if (ret.hasCpuLoad) {
                m_cpuLoad += (ret.cpuLoad - m_cpuLoad) * 0.1f;
            }
        }

        SetLED(CRGB(0, 0, (m_counter < 60 ? m_counter : 120 - m_counter)));
        m_counter = (m_counter + 1) % 120;
    }

    //--------------
    static void UpdateTask(void *parameter) {
        System *pSystem = static_cast<System *>(parameter);
        TickType_t xLastWakeTime;
        const TickType_t xFrequency = 8 / portTICK_PERIOD_MS; // 8ms
        while (1) {
            pSystem->Update();
            xTaskDelayUntil( &xLastWakeTime, xFrequency );
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
#ifdef HAS_DISPLAY
  M5.Lcd.clear(TFT_BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf("PLAYING\n%3.2f%%", sys.m_cpuLoad * 100.0f);
#endif
  //M5.update();
  //if (M5.BtnA.isPressed()) {
#ifdef BUTTON_PIN
  if (digitalRead(BUTTON_PIN) == LOW) {
    sys.m_btnPressed = true;
    //noteOn(60, 127);
    //setLed(CRGB(0, 100, 0));
    //delay(500);
    //noteOff();
  }
  else {
    sys.m_btnPressed = false;
  }
#endif
  delay(500);
}
