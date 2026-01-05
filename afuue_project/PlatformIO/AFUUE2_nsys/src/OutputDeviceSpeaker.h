#pragma once
#include <Arduino.h>
#include <driver/mcpwm_prelude.h>
#include "OutputDeviceBase.h"

#define SOUND_TWOPWM
#define PWMPIN_LOW (GPIO_NUM_5)
#define PWMPIN_HIGH (GPIO_NUM_6)

//---------------------------------
#define MCPWM_PERIOD_TICKS (256)
mcpwm_timer_handle_t timer_handle;
mcpwm_timer_config_t timer_config = {
    .group_id = 0,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = 80*1000*1000, // 80MHz
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    .period_ticks = MCPWM_PERIOD_TICKS, // 312.5kHz
    .intr_priority = 0,
};
mcpwm_oper_handle_t oper_handle;
mcpwm_operator_config_t oper_config = {
    .group_id = 0,
    .intr_priority = 0,
};
mcpwm_gen_handle_t gen_handle_low, gen_handle_high;
mcpwm_generator_config_t gen_config_low = {
    .gen_gpio_num = PWMPIN_LOW,
    .flags = {
        .io_od_mode = false,
        .pull_up = false,
        .pull_down = false,
    }
};
mcpwm_generator_config_t gen_config_high = {
    .gen_gpio_num = PWMPIN_HIGH,
    .flags = {
        .io_od_mode = false,
        .pull_up = false,
        .pull_down = false,
    }
};
mcpwm_cmpr_handle_t cmp_handle_low, cmp_handle_high;
mcpwm_comparator_config_t cmp_config = {
    .flags = {
        .update_cmp_on_tez = true,
    }
};

//---------------------------------
#define CLOCK_DIVIDER (50)//(80)
#define TIMER_ALARM (50)//(40)
//#define SAMPLING_RATE (20000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*50) = 20kHz
//#define SAMPLING_RATE (25000.0f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*40) = 25kHz
#define SAMPLING_RATE (32000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (50*50) = 32kHz
//#define SAMPLING_RATE (44077.13f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (55*33) = 44kHz
//#define SAMPLING_RATE (48019.2f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (49*34) = 48kHz

/*
    |   write     read   |   write
    |   -->|       -->|  |   -->|
                      |<-- d -->|

    |   read      write  |
    |   -->|       -->|  |
    |      |<-- d ^-->|
*/
#define WAVEOUT_BUFFERMAX (512)
#define WAVEOUT_BUFFERING_SIZE (400)
uint16_t waveOutBuffer[WAVEOUT_BUFFERMAX];
int32_t waveOutBufferWritePos = 0;
int32_t waveOutBufferReadPos = WAVEOUT_BUFFERMAX / 2;

hw_timer_t *timer = NULL;
volatile uint8_t waveHigh = 0;
volatile uint8_t waveLow = 0;
void IRAM_ATTR OnTimer() {
#ifdef SOUND_TWOPWM
    mcpwm_comparator_set_compare_value(cmp_handle_low, waveLow);
    mcpwm_comparator_set_compare_value(cmp_handle_high, waveHigh);
#else
    //dacWrite(DACPIN, h);
#endif
#if 1
    uint16_t w = waveOutBuffer[waveOutBufferReadPos];
    waveLow = (w & 0xFF);
    waveHigh = (w >> 8) & 0xFF;
    waveOutBufferReadPos = (waveOutBufferReadPos + 1) % WAVEOUT_BUFFERMAX;
#endif
}

//---------------------------------
class OutputDeviceSpeaker : public OutputDeviceBase {
public:
    float m_tickCount = 0.0f;
    float m_volume = 0.0f;
    uint64_t m_dt = 0;

    //--------------
    const char* GetName() const override {
        return "Speaker";
    }

    //--------------
    InitializeResult Initialize() override {
        InitializeResult result;
        pinMode(PWMPIN_LOW, OUTPUT);
        pinMode(PWMPIN_HIGH, OUTPUT);
        //gpio_set_drive_capability(PWMPIN_LOW, GPIO_DRIVE_CAP_0);
        //gpio_set_drive_capability(PWMPIN_HIGH, GPIO_DRIVE_CAP_0);
        // MCPWM set up
        mcpwm_new_timer(&timer_config, &timer_handle);
        mcpwm_new_operator(&oper_config, &oper_handle);
        mcpwm_operator_connect_timer(oper_handle, timer_handle);
        mcpwm_new_generator(oper_handle, &gen_config_low, &gen_handle_low);
        mcpwm_new_generator(oper_handle, &gen_config_high, &gen_handle_high);
        mcpwm_new_comparator(oper_handle, &cmp_config, &cmp_handle_low);
        mcpwm_new_comparator(oper_handle, &cmp_config, &cmp_handle_high);
        mcpwm_comparator_set_compare_value(cmp_handle_low, 0);
        mcpwm_comparator_set_compare_value(cmp_handle_high, 0);

        mcpwm_generator_set_action_on_timer_event(gen_handle_low, 
            MCPWM_GEN_TIMER_EVENT_ACTION(
                MCPWM_TIMER_DIRECTION_UP,
                MCPWM_TIMER_EVENT_EMPTY,
                MCPWM_GEN_ACTION_HIGH
            )
        );
        mcpwm_generator_set_action_on_compare_event(gen_handle_low, 
            MCPWM_GEN_COMPARE_EVENT_ACTION(
                MCPWM_TIMER_DIRECTION_UP,
                cmp_handle_low,
                MCPWM_GEN_ACTION_LOW
            )
        );
        mcpwm_generator_set_action_on_timer_event(gen_handle_high, 
            MCPWM_GEN_TIMER_EVENT_ACTION(
                MCPWM_TIMER_DIRECTION_UP,
                MCPWM_TIMER_EVENT_EMPTY,
                MCPWM_GEN_ACTION_HIGH
            )
        );
        mcpwm_generator_set_action_on_compare_event(gen_handle_high, 
            MCPWM_GEN_COMPARE_EVENT_ACTION(
                MCPWM_TIMER_DIRECTION_UP,
                cmp_handle_high,
                MCPWM_GEN_ACTION_LOW
            )
        );
        mcpwm_timer_enable(timer_handle);
        mcpwm_timer_start_stop(timer_handle, MCPWM_TIMER_START_NO_STOP);

        mcpwm_comparator_set_compare_value(cmp_handle_low, 0);
        mcpwm_comparator_set_compare_value(cmp_handle_high, 0x80);

        for (int i = 0; i < WAVEOUT_BUFFERMAX; i++) {
            waveOutBuffer[i] = 0x8000;
        }
        xTaskCreatePinnedToCore(CreateWaveTask, "CreateWaveTask", 16384, this, 3, NULL, CORE0); // 波形生成は Core0 が専念

        timer = timerBegin(80*1000*1000 / CLOCK_DIVIDER);
        timerAttachInterrupt(timer, &OnTimer);
        timerAlarm(timer, TIMER_ALARM, true, 0);
        return result;
    }

    //--------------
    OutputResult Update(float note, float volume) override {
        m_tickCount = CalcFrequency(note) / SAMPLING_RATE;
        m_volume = volume;
        //float load = m_dt / 1000000.0f;
        //return OutputResult{ true, load / (1.0f / SAMPLING_RATE) };
        return OutputResult{ true, (float)m_dt };
    }

    //--------------
    static float CalcFrequency(float note) {
        return 440.0f * pow(2, (note - (69.0f-12.0f))/12.0f);
    }

    //--------------
    static void CreateWaveTask(void *parameter) {
        OutputDeviceSpeaker *pSystem = static_cast<OutputDeviceSpeaker *>(parameter);

        TickType_t xLastWakeTime;
        const TickType_t xFrequency = 2 / portTICK_PERIOD_MS; // 2ms

        float phase = 0.0f;
        float fmid = 32768.0f;
        const int32_t umid = 32768;
        while (1) {
            int createCount = 0;
            uint64_t t0 = micros();
            while (1) {
                int d = (waveOutBufferWritePos > waveOutBufferReadPos)
                ? waveOutBufferWritePos - waveOutBufferReadPos
                : WAVEOUT_BUFFERMAX + waveOutBufferWritePos - waveOutBufferReadPos;
                if (d > WAVEOUT_BUFFERING_SIZE) {
                    break;
                }
                phase += pSystem->m_tickCount;
                if (phase >= 1.0f) phase -= 1.0f;

                float f = (fmid*1.8f) * (pSystem->m_volume * (-0.5f + phase));
                //float f = (fmid*0.9f)* (pSystem->m_volume * sinf(phase * 2.0f * 3.14159265f));
                if ( (-0.000002f < f) && (f < 0.000002f) ) {
                    f = 0.0f;
                }
                uint16_t w = static_cast<uint16_t>(umid + static_cast<int32_t>(f));
                waveOutBuffer[waveOutBufferWritePos] = w;
                waveOutBufferWritePos = (waveOutBufferWritePos + 1) % WAVEOUT_BUFFERMAX;
                createCount++;
            }
            if (createCount > 0) {
                pSystem->m_dt = (micros() - t0) / createCount;
            }
            xTaskDelayUntil( &xLastWakeTime, xFrequency );
        }
    }
};
