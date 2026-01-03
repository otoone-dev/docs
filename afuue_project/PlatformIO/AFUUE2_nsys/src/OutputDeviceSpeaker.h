#pragma once
#include <Arduino.h>
#include <driver/mcpwm_prelude.h>
#include "OutputDeviceBase.h"

#define SOUND_TWOPWM
#define PWMPIN_LOW (5)
#define PWMPIN_HIGH (6)

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
};
mcpwm_generator_config_t gen_config_high = {
    .gen_gpio_num = PWMPIN_HIGH,
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
void IRAM_ATTR OnTimer() {
#if 1
    uint16_t w = waveOutBuffer[waveOutBufferReadPos];
#ifdef SOUND_TWOPWM
    mcpwm_comparator_set_compare_value(cmp_handle_low, w % MCPWM_PERIOD_TICKS);
    mcpwm_comparator_set_compare_value(cmp_handle_high, w / MCPWM_PERIOD_TICKS);
#else
    //dacWrite(DACPIN, h);
#endif
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

        xTaskCreatePinnedToCore(CreateWaveTask, "CreateWaveTask", 16384, this, configMAX_PRIORITIES-1, NULL, CORE0); // 波形生成は Core0 が専念

        timer = timerBegin(80*1000*1000 / CLOCK_DIVIDER);
        timerAttachInterrupt(timer, &OnTimer);
        timerAlarm(timer, TIMER_ALARM, true, 0);
        return result;
    }

    //--------------
    OutputResult Update(float note, float volume) override {
        OutputResult result;
        m_tickCount = CalcFrequency(note) / SAMPLING_RATE;
        m_volume = volume;
        //float load = m_dt / 1000000.0f;
        //return OutputResult{ true, load / (1.0f / SAMPLING_RATE) };
        return OutputResult{ true, (float)m_dt };
    };

    //--------------
    static float CalcFrequency(float note) {
        return 440.0f * pow(2, (note - (69.0f-12.0f))/12.0f);
    }

    //--------------
    static void CreateWaveTask(void *parameter) {
        OutputDeviceSpeaker *pSystem = static_cast<OutputDeviceSpeaker *>(parameter);

        TickType_t xLastWakeTime;
        const TickType_t xFrequency = 1; // 1ms

        float phase = 0.0f;
        float fmid = (MCPWM_PERIOD_TICKS * MCPWM_PERIOD_TICKS) / 2.0f;
        const int32_t umid = MCPWM_PERIOD_TICKS * MCPWM_PERIOD_TICKS / 2;
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
                uint32_t w = static_cast<uint32_t>(umid + static_cast<int32_t>(f));
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
