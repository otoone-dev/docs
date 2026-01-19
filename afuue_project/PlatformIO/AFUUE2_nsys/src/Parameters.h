#pragma once

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
#define SOUND_TWOPWM
#define PWMPIN_LOW (GPIO_NUM_5) // AFUUE2R Gen2
#define PWMPIN_HIGH (GPIO_NUM_6)
#define HAS_DISPLAY
#ifndef HAS_DISPLAY
#define NEOPIXEL_PIN (GPIO_NUM_35)
#endif
#define MIDI_IN_PIN (GPIO_NUM_9)  // not use
#define MIDI_OUT_PIN (GPIO_NUM_7)
#define BUTTON_PIN (GPIO_NUM_41)
#define I2CPIN_SDA (GPIO_NUM_38)
#define I2CPIN_SCL (GPIO_NUM_39)
#else
#define SOUND_TWOPWM
#define PWMPIN_LOW (GPIO_NUM_39) // AFUUE2R 初代
#define PWMPIN_HIGH (GPIO_NUM_40)
#define ADCPIN_BREATH (GPIO_NUM_11)
#define ADCPIN_BEND (GPIO_NUM_12)
#define NEOPIXEL_PIN   (GPIO_NUM_21)
#define MIDI_IN_PIN (GPIO_NUM_42)  // not use
#define MIDI_OUT_PIN (GPIO_NUM_41)
#define BUTTON_PIN (GPIO_NUM_0)
#endif // HW_AFUUE2R_GEN2
#endif // HW_AFUUE2R

struct Parameters {
    float fineTune = 440.0f;
    float samplingRate = 44077.135f;
    int waveTableIndex = 0;
};

struct Message {
    float volume;
    float note;
    float bend;
    uint16_t keyData;
};

template<typename T>
T Clamp(T v, T min, T max) {
    if (v < min) {
        return min;
    }
    else if (v > max) {
        return max;
    }
    return v;
}
