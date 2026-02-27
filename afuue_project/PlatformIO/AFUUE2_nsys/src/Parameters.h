#pragma once
#include "WaveTable.h"
#include <Arduino.h>
#include <Preferences.h>
#include <string>
#include <cmath>

#define AFUUE_VER (2000)

#define CORE0 (0)
#define CORE1 (1)
#define CLOCK_DIVIDER (55)
#define TIMER_ALARM (33)
// (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*50) = 20kHz
// (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*40) = 25kHz
// (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (50*50) = 32kHz
// (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (55*33) = 44kHz
// (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (49*34) = 48kHz

#ifdef HW_AFUUE2R
#ifdef HW_AFUUE2R_GEN2
#define PWMPIN_LOW (GPIO_NUM_5) // AFUUE2R Gen2
#define PWMPIN_HIGH (GPIO_NUM_6)
#ifndef HAS_DISPLAY
#define NEOPIXEL_PIN (GPIO_NUM_35)
#endif
#define MIDI_IN_PIN (GPIO_NUM_9)  // not use
#define MIDI_OUT_PIN (GPIO_NUM_7)
#define BUTTON_PIN (GPIO_NUM_41)
#define LED_PIN (GPIO_NUM_8)
#define I2CPIN_SDA (GPIO_NUM_38)
#define I2CPIN_SCL (GPIO_NUM_39)
#else
#define PWMPIN_LOW (GPIO_NUM_39) // AFUUE2R 初代
#define PWMPIN_HIGH (GPIO_NUM_40)
#define ADCPIN_BREATH (GPIO_NUM_11)
#define ADCPIN_BEND (GPIO_NUM_12)
#define NEOPIXEL_PIN   (GPIO_NUM_21)
#define MIDI_IN_PIN (GPIO_NUM_42)  // not use
#define MIDI_OUT_PIN (GPIO_NUM_41)
#define BUTTON_PIN (GPIO_NUM_0)
#endif // HW_AFUUE2R_GEN2
#elif defined(HW_TONEGENERATOR) // HW_AFFUE2R
#define NEOPIXEL_PIN   (GPIO_NUM_21)
#define MIDI_IN_PIN (GPIO_NUM_42)
#define MIDI_OUT_PIN (GPIO_NUM_41)
#endif // HW_AFUUE2R

//-------------
enum class PlayMode : uint8_t {
    Normal,
    Bend,
    MIDI_Normal,
    MIDI_Bend,
    USBMIDI_Normal,
    USBMIDI_Bend,
};

//-------------
enum class MIDIBreathMode : uint8_t {
    BreathControl,
    Expression,
    AfterTouch,
    MainVolume,
    CutOff,
    MIDIBreathMode_Max,
};
std::string MIDIBreathModeToStr(MIDIBreathMode mode) {
    switch (mode) {
        case MIDIBreathMode::BreathControl:
        return "BrCt";
        case MIDIBreathMode::Expression:
        return "Expr";
        case MIDIBreathMode::AfterTouch:
        return "Aftr";
        case MIDIBreathMode::MainVolume:
        return "MVol";
        case MIDIBreathMode::CutOff:
        return "COff";
    }
    return "??";
}

//-------------
class Parameters {
public:
    Parameters()
        : info()
    {
    }
    const float samplingRate = 44077.135f;
    float beepNote = 48.0f;
    uint64_t beepTime = 0;
    uint64_t dispTime = 0;
    std::string dispMessage = "";

    float fineTune = 440.0f;
    float baseNote;
    float delayAmount = 0.15f;
    float delayTime = 0.3f;
    float breathSense = 0.8f;
    uint64_t keyDelay = 20*1000; // microseconds
    WaveInfo info;
    PlayMode playMode = PlayMode::Normal;
    MIDIBreathMode breathMode = MIDIBreathMode::BreathControl;
#ifdef DEBUG
    std::string debugMessage = "";
#endif
    void Initialize() {
        int ver = pref.getInt("AfuueVer", -1);
        if (ver != AFUUE_VER) {
            pref.clear(); // CLEAR ALL FLASH MEMORY
            pref.putInt("AfuueVer", AFUUE_VER);
        }
        LoadPreferences();
        int i = waveTableIndex % waveInfos.size();
        info = waveInfos[i];
    }

    int GetWaveTableIndex() const {
        return waveTableIndex;
    }
    void SetWaveTableIndex(int index) {
        waveTableIndex = index;
        int i = index % waveInfos.size();
        info = waveInfos[i];
    }
    int GetWaveTableCount() const {
        return waveInfos.size();
    }
    void SetBeep(float note, int32_t milliseconds) {
        beepNote = note;
        beepTime = millis() + milliseconds;
    }
    void SetDispMessage(const char* message, int32_t milliseconds) {
        dispMessage = message;
        dispTime = millis() + milliseconds;
    }
    bool IsBendEnabled() const {
        return playMode == PlayMode::Bend || playMode == PlayMode::MIDI_Bend || playMode == PlayMode::USBMIDI_Bend;
    }
    bool IsMidiEnabled() const {
        return playMode == PlayMode::MIDI_Normal
        || playMode == PlayMode::MIDI_Bend
        || playMode == PlayMode::USBMIDI_Normal
        || playMode == PlayMode::USBMIDI_Bend;
    }
    bool IsUSBMIDIEnabled() const {
        return playMode == PlayMode::USBMIDI_Normal
        || playMode == PlayMode::USBMIDI_Bend;
    }
    void NextPlayMode() {
        if (static_cast<uint8_t>(playMode) <= static_cast<uint8_t>(PlayMode::MIDI_Bend)) {
            playMode = static_cast<PlayMode>((static_cast<uint8_t>(playMode) + 1) % 4);
        }
        else {
            if (playMode == PlayMode::USBMIDI_Bend) {
                playMode = PlayMode::Normal;
            }
            else {
                playMode = PlayMode::USBMIDI_Bend;
            }
        }
    }
    //-------------------------------------
    // Flash書き込み開始
    void BeginPreferences(bool writable) {
        usePreferencesDepth++;
        if (usePreferencesDepth > 1) return;
        //if (timer) timerAlarmDisable(timer);
        pref.begin("Afuue/Settings", !writable);
    }

    //--------------------------
    // Flash書き込み終了
    void EndPreferences() {
        usePreferencesDepth--;
        if (usePreferencesDepth == 0) {
            pref.end();  
            delay(10);
            //if (timer) timerAlarmEnable(timer);
        }
        if (usePreferencesDepth < 0) usePreferencesDepth = 0;
    }
    // Clear
    void ClearPreferences() {
        pref.clear();
    }
    // Save
    void SavePreferences() {
        BeginPreferences(true); {
            pref.putInt("WaveIndex", waveTableIndex);
            pref.putFloat("BreathSense", breathSense);
            pref.putFloat("BaseNote", baseNote);
            pref.putFloat("FineTune", fineTune);
            pref.putFloat("DelayAmount", delayAmount);
            pref.putInt("MIDIBreathMode", static_cast<int32_t>(breathMode));
        } EndPreferences();
    }
    // Load
    void LoadPreferences() {
        BeginPreferences(false); {
            waveTableIndex = pref.getInt("WaveIndex", 0);
            breathSense = pref.getFloat("BreathSense", 0.8f);
            baseNote = pref.getFloat("BaseNote", 48.0f);
            fineTune = pref.getFloat("FineTune", 440.0f);
            breathMode = static_cast<MIDIBreathMode>(pref.getInt("MIDIBreathMode", 0));
        } EndPreferences();
    }

private:
    int waveTableIndex = 0;
    int usePreferencesDepth = 0;
    Preferences pref;
};

//-------------
struct Message {
    float volume = 0.0f;
    float note = 48.0f;
    float bend = 0.0f;
    float playTime = 0.0f;
    float keepNoteTime = 0.0f;
    uint16_t keyData = 0;
};

//-------------
#define Clamp(v, min, max) ((v) < (min) ? (min) : ((v) > (max) ? (max) : (v)))

//-------------
template<typename T>
T Wrap(T v, T min, T max) {
    if (max <= min) {
        return min;
    }
    T c = v;
    T range = max - min;
    while (c < min) {
        c += range;
    }
    while (c >= max) {
        c -= range;
    }
    return c;
}

//-------------
float Step(float f, float step) {
    int32_t i = static_cast<int32_t>(f / step);
    return i * step;
}

//--------------
float CalcFrequency(float fine, float note) {
    return fine * pow(2, (note - (69.0f-12.0f))/12.0f);
}

