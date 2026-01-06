#pragma once

#define SAMPLING_RATE (32000)
#define CLOCK_DIVIDER (55)
#define TIMER_ALARM (33)
// (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*50) = 20kHz
// (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*40) = 25kHz
// (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (50*50) = 32kHz
// (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (55*33) = 44kHz
// (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (49*34) = 48kHz

struct Parameters {
    float freq = 440.0f;
    float samplingRate = SAMPLING_RATE;
};
