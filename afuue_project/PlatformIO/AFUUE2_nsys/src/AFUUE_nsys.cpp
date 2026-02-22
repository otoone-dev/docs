#include "DeviceBase.h"
#include "WaveTable.h"
#include "InputDevices/InputDeviceBase.h"
#include "InputDevices/PressureLPS33.h"
//#include "InputDevices/PressureADC.h"
#include "InputDevices/KeyMCP23017.h"
//#include "InputDevices/KeyDigitalAFUUE2R.h"
#include "InputDevices/Key.h"

#include "SoundProcessor/WaveGenerator.h"
#include "SoundProcessor/LFO.h"
#include "SoundProcessor/LowPassFilter.h"
#include "SoundProcessor/Delay.h"

#include "OutputDevices/OutputDeviceBase.h"
#include "OutputDevices/Speaker.h"
#include "OutputDevices/LED.h"
#include "OutputDevices/USB_MIDI.h"
#include "OutputDevices/SerialMIDI.h"

#include "Menu/MenuForKey.h"

#ifdef HW_TONEGENERATOR
#include "MidiDebug.h"
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <M5Unified.h>

#ifdef HAS_DISPLAY
#include "logo.h"
M5Canvas canvas(&M5.Display);
#endif
#include <vector>
#include <string>

//-----------
#include <Wire.h>
#define I2C_FREQ (400000)

//-----------
 
//---------------------
class System {
public:
    float m_cpuLoad = 0.0f;
    bool m_btnPressed = false;
#ifdef DEBUG
    std::string m_debugMessage;
#endif
    //--------------
    System()
        : m_keys(0)
        , m_parameters()
    {
    }

    //--------------
    void initialize() {
        SetLED(0, 0, 100);
        delay(100);
        SetLED(0, 0, 1);
        delay(100);
#ifdef HAS_DISPLAY
        auto cfg = M5.config();
        M5.begin(cfg);
        canvas.createSprite(M5.Display.width(), M5.Display.height());
#endif
        ClearDisplay(2);
        Display("AFUUE2R", TFT_WHITE, true);
        delay(500);

        SetLED(0, 0, 100);
        delay(100);
        SetLED(0, 0, 1);
        delay(500);

        ClearDisplay(1);
#ifdef HW_TONEGENERATOR
        //DebugMIDIMessage(); // MIDI受信テスト
#endif
        btStop();
        WiFi.mode(WIFI_OFF); 

#if defined(I2CPIN_SDA) && defined(I2CPIN_SCL)
        Wire.begin(I2CPIN_SDA, I2CPIN_SCL, I2C_FREQ);
#endif
        // 登録順が重要
#ifdef HAS_LPS33
        m_inputDevices.push_back(new PressureLPS33(Wire, PressureLPS33::ReadType::BREATH_AND_BEND));
#endif
#ifdef HAS_MCP23017
        m_inputDevices.push_back(new KeyMCP23017(Wire));
#endif
        //m_inputDevices.push_back(new PressureADC(ADCPIN_BREATH, ADCPIN_BEND, PressureADC::ReadType::BREATH_AND_BEND));
        //m_inputDevices.push_back(new KeyDigitalAFUUE2R());

        m_soundProcessors.push_back(new LFO());
        m_soundProcessors.push_back(new WaveGenerator());
        m_soundProcessors.push_back(new LowPassFilter());
        m_soundProcessors.push_back(new Delay(8000));

#ifdef LED_PIN
        m_outputDevices.push_back(new LED(LED_PIN));
#endif
#ifdef USE_USBMIDI
        m_outputDevices.push_back(new USB_MIDI());
#endif
#if defined(MIDI_OUT_PIN) && defined(MIDI_IN_PIN)
        m_outputDevices.push_back(new SerialMIDI(MIDI_OUT_PIN, MIDI_IN_PIN));
#endif
#if defined(PWMPIN_LOW) && defined(PWMPIN_HIGH)
        m_outputDevices.push_back(new Speaker(PWMPIN_LOW, PWMPIN_HIGH, m_soundProcessors));
#endif

        m_menus.push_back(new MenuForKey());

        m_parameters.SetWaveTableIndex(0);

        std::string errorMessage = "";
        // Input Devices
        for (auto& device : m_inputDevices) {
            Display(device->GetName());
            auto result = device->Initialize(m_parameters);
            if (!result.success) {
                errorMessage += result.errorMessage + "\n";
            }
            if (result.skipAfter) {
                break;
            }
        }
        //  Sound Processors
        for (auto& processor : m_soundProcessors) {
            processor->Initialize(m_parameters);
        }
        // Output Devices
        for (auto& device : m_outputDevices) {
            Display(device->GetName());
            auto result = device->Initialize(m_parameters);
            if (!result.success) {
                errorMessage += result.errorMessage + "\n";
            }
            if (result.skipAfter) {
                break;
            }
        }
        // Menus
        for (auto& menu : m_menus) {
            Display(menu->GetName());
            menu->Initialize(m_parameters);
        }

        if (errorMessage != "") {
            Display(errorMessage.c_str(), TFT_RED);
            while (1) {
                delay(1000);
            }
        }
        xTaskCreatePinnedToCore(UpdateTask, "UpdateTask", 4096, this, 1, NULL, CORE1);

#ifdef HAS_DISPLAY
        M5.Lcd.setBrightness(10);
#endif
    }
    //--------------
#if defined(NEOPIXEL_PIN)
    void SetLED(uint8_t r, uint8_t g, uint8_t b) {
        rgbLedWrite((uint8_t)NEOPIXEL_PIN, r, g, b);
    }
#else
    void SetLED(uint8_t r, uint8_t g, uint8_t b) {}
#endif

    //--------------
    const std::string& GetDispMessage() const {
        return m_parameters.dispMessage;
    }
    //--------------
    const std::string& GetWaveName() {
        if (m_parameters.IsUSBMIDIEnabled()) {
            char s[32];
            sprintf(s, "No.%03d", m_parameters.GetWaveTableIndex()+1);
            m_waveNoStr = s;
            return m_waveNoStr;
        }
        return m_parameters.info.name;
    }

private:
    std::vector<InputDeviceBase*> m_inputDevices;
    std::vector<OutputDeviceBase*> m_outputDevices;
    std::vector<SoundProcessorBase*> m_soundProcessors;
    std::vector<MenuBase*> m_menus;
    Parameters m_parameters;
    std::string m_waveNoStr;
    Keys m_keys;
    bool m_btnWasPressed = false;

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
    void Display(const char* message, int color = TFT_WHITE, bool center = false) {
#ifdef HAS_DISPLAY
        M5.Lcd.setTextColor(color);
        if (center) {
            M5.Lcd.drawCenterString(message,
                M5.Lcd.width() / 2, 
                M5.Lcd.height() / 2 - (M5.Lcd.fontHeight(M5.Lcd.getFont()) * M5.Lcd.getTextSizeY()) / 2);
        }
        else {
            M5.Lcd.printf("%s\n", message);
        }
        delay(100);
#endif
    }
public:
    //--------------
    void Update() {
        uint64_t currentTime = millis();
        Message message;
#ifdef DEBUG
        std::string debugMessage = "";
#endif
        // 入力
        for (auto& device : m_inputDevices) {
            bool success = device->Update(m_parameters, message);
            if (!success) {
                continue;
            }
#ifdef DEBUG
            if (!m_parameters.debugMessage.empty()) {
                debugMessage += m_parameters.debugMessage + "\n";
                m_parameters.debugMessage = "";
            }
#endif
        }

        if (m_btnPressed) {
            if (!m_btnWasPressed) {
                m_parameters.SetBeep(53.0f, 200);
                m_parameters.NextPlayMode();
            }
        }
        m_btnWasPressed = m_btnPressed;
        {
            // メニュー
            m_keys.Update(message.keyData);
            for (auto& menu : m_menus) {
                menu->Update(m_parameters, m_keys);
#ifdef DEBUG
                if (!m_parameters.debugMessage.empty()) {
                    debugMessage += m_parameters.debugMessage + "\n";
                    m_parameters.debugMessage = "";
                }
#endif
            }
            if (m_parameters.beepTime > 0) {
                if (currentTime < m_parameters.beepTime) {
                    message.volume = 0.2f;
                    message.note = m_parameters.beepNote;
                }
                else {
                    m_parameters.beepTime = 0;
                }
            }
            if (m_parameters.dispTime > 0) {
                if (currentTime < m_parameters.dispTime) {
                    // 表示中
                }
                else {
                    m_parameters.dispTime = 0;
                }
            }
            else {
                m_parameters.dispMessage = "";
            }
        }
        // 出力
        for (auto& device : m_outputDevices) {
            OutputResult result = device->Update(m_parameters, message);
            if (result.hasCpuLoad) {
                m_cpuLoad += (result.cpuLoad - m_cpuLoad) * 0.1f;
            }
#ifdef DEBUG
            if (!m_parameters.debugMessage.empty()) {
                debugMessage += m_parameters.debugMessage + "\n";
                m_parameters.debugMessage = "";
            }
#endif
        }

#ifdef DEBUG
        m_debugMessage = debugMessage;
#endif
        if (m_parameters.IsBendEnabled()) {
            // Bend mode
            int32_t b = static_cast<int32_t>(message.bend * 20.0f);
            SetLED(Clamp(20+b, 0, 255), 0, Clamp(-b, 0, 255));
        }
        else {
            // Normal mode
            SetLED(0, 20, 0);
        }
    }

    //--------------
    static void UpdateTask(void *parameter) {
        System *pSystem = static_cast<System *>(parameter);
        TickType_t xLastWakeTime;
        const TickType_t xFrequency = 5 / portTICK_PERIOD_MS; // 5ms
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
void loop() {
  static int loopCount = 0;
  M5.update();
#ifdef HAS_DISPLAY
  canvas.clear(TFT_BLACK);
  canvas.setCursor(0, 0);
  canvas.setTextSize(2);
#ifdef DEBUG
  canvas.printf("PLAYING\n%s", sys.m_debugMessage.c_str());
  delay(100);
  canvas.pushSprite(0, 0);
  return;
#else
  static std::string name = "";
  const std::string& currentName = sys.GetWaveName();
  const std::string& dispMessage = sys.GetDispMessage();
  if (name != currentName || !dispMessage.empty()) {
    canvas.clear(TFT_BLACK);
    canvas.setTextSize(2);
    canvas.pushImage(-5, -10, 120, 105, bitmap_logo);
    canvas.setCursor(0, 120 - 30);
    if (!dispMessage.empty()) {
        canvas.printf("\n%s", dispMessage.c_str());
    }
    else {
        if (name != currentName) {
            name = currentName;
            canvas.printf("\n%s", name.c_str());
        }
    }
    canvas.pushSprite(0, 0);
  }
  //canvas.printf("PLAYING\n%3.2f%%\n", sys.m_cpuLoad * 100.0f);
  //canvas.printf("\n%s", sys.GetDispMessage().c_str());
#endif
#endif //HAS_DISPLAY

#ifdef BUTTON_PIN
  if (digitalRead(BUTTON_PIN) == LOW) {
    sys.m_btnPressed = true;
  }
  else {
    sys.m_btnPressed = false;
  }
#endif
  delay(50);
}
