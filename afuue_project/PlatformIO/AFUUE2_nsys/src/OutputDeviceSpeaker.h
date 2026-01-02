#pragma once
#include <Arduino.h>
#include <driver/mcpwm_prelude.h>
#include "OutputDeviceBase.h"

#define SOUND_TWOPWM
#define PWMPIN_LOW (5)
#define PWMPIN_HIGH (6)

//---------------------------------
#define MCPWM_PERIOD_TICKS (1066)
mcpwm_timer_handle_t timer_handle;
mcpwm_timer_config_t timer_config = {
    .group_id = 0,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = 160*1000*1000, // 160MHz
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    .period_ticks = MCPWM_PERIOD_TICKS, // 150kHz
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
#define CLOCK_DIVIDER (80)
#define TIMER_ALARM (40)
//#define SAMPLING_RATE (20000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*50) = 20kHz
#define SAMPLING_RATE (25000.0f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*40) = 25kHz
//#define SAMPLING_RATE (32000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (50*50) = 32kHz
//#define SAMPLING_RATE (44077.13f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (55*33) = 44kHz
//#define SAMPLING_RATE (48019.2f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (49*34) = 48kHz

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
TaskHandle_t taskHandle;

void IRAM_ATTR OnTimer() {
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(taskHandle, 0, eNoAction, &higherPriorityTaskWoken);
}

//---------------------------------
class OutputDeviceSpeaker : public OutputDeviceBase {
public:
    float m_tickCount = 0.0f;
    float m_volume = 0.0f;

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

        xTaskCreatePinnedToCore(CreateWaveTask, "CreateWaveTask", 16384, this, configMAX_PRIORITIES-1, &taskHandle, CORE0); // 波形生成は Core0 が専念

        timer = timerBegin(80*1000*1000 / CLOCK_DIVIDER);
        timerAttachInterrupt(timer, &OnTimer);
        timerAlarm(timer, TIMER_ALARM, true, 0);
        return result;
    }

    //--------------
    void Update(float note, float volume) override {
        m_tickCount = CalcFrequency(note) / SAMPLING_RATE;
        m_volume = volume;
    };

    //--------------
    static float CalcFrequency(float note) {
        return 440.0f * pow(2, (note - (69.0f-12.0f))/12.0f);
    }

    //--------------
    static void CreateWaveTask(void *parameter) {
        OutputDeviceSpeaker *pSystem = static_cast<OutputDeviceSpeaker *>(parameter);
        float phase = 0.0f;
        uint64_t t0 = micros();
        while (1) {
            uint64_t t1 = micros();
            //dt1 = t1 - t0;
            t0 = t1;

            phase += pSystem->m_tickCount;
            if (phase >= 1.0f) phase -= 1.0f;

            float f = 60000.0f * (pSystem->m_volume * (-0.5f + phase));
            //float f = 32000.0f * (pSystem->m_volume * sinf(phase * 2.0f * 3.14159265f));
            if ( (-0.00002f < f) && (f < 0.00002f) ) {
                f = 0.0f;
            }
            uint16_t d = static_cast<uint16_t>(32768.0f + f);

            uint8_t h = (d >> 8) & 0xFF;
            uint8_t l = d & 0xFF;
            t1 = micros();
            //dt2 = t1 - t0;

            uint32_t data;
            xTaskNotifyWait(0, 0, &data, portMAX_DELAY);

            t1 = micros();
#ifdef SOUND_TWOPWM
            mcpwm_comparator_set_compare_value(cmp_handle_low, (l * MCPWM_PERIOD_TICKS) / 255);
            mcpwm_comparator_set_compare_value(cmp_handle_high, (h * MCPWM_PERIOD_TICKS) / 255);
#else
            //dacWrite(DACPIN, h);
#endif
            //dt3 = micros() - t1;
        }
    }
};
