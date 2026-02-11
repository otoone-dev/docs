#pragma once
#include "InputDeviceBase.h"
#include <Arduino.h>
//#include <driver/adc.h>
#include <esp_adc/adc_oneshot.h>
#include <string>
#include <format>

class PressureADC : public InputDeviceBase {
public:
    enum class ReadType : uint8_t {
        BREATH, // addr=0x5C
        BREATH_AND_BEND,   // addr=0x5D
    };

    enum class PressureType : uint8_t {
        BREATH,
        BEND,
    };

    //--------------
    PressureADC(int32_t breath, int32_t bend, ReadType readType) 
        : m_pinBreath(breath)
        , m_pinBend(bend)
        , m_readType(readType) {}

    //--------------
    const char* GetName() const override {
        return "ADC";
    }

    //--------------
    InitializeResult Initialize() override {
        InitializeResult result;

        adc_oneshot_unit_init_cfg_t init_cfg = {
            .unit_id = ADC_UNIT_2,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
        };
        adc_oneshot_new_unit(&init_cfg, &m_adc1);
        adc_oneshot_new_unit(&init_cfg, &m_adc2);

        adc_oneshot_chan_cfg_t chan_cfg = {
            .atten = ADC_ATTEN_DB_0,
            .bitwidth = ADC_BITWIDTH_12,
        };
        adc_oneshot_config_channel(m_adc1, ADC_CHANNEL_0, &chan_cfg);
        adc_oneshot_config_channel(m_adc2, ADC_CHANNEL_1, &chan_cfg);

        /*
#if 0 //(MAINUNIT == M5STICKC_PLUS)
        gpio_pulldown_dis(GPIO_NUM_25);
        gpio_pullup_dis(GPIO_NUM_25);
#endif
        analogSetAttenuation(ADC_0db);
        analogReadResolution(12); // 4096
#if 1 //(MAINUNIT == M5STAMP_S3)
        adc2_config_channel_atten(ADC2_CHANNEL_0, ADC_ATTEN_DB_0);
        adc2_config_channel_atten(ADC2_CHANNEL_1, ADC_ATTEN_DB_0);
#endif
        */

        m_defaultPressure = GetPressure(PressureType::BREATH) + 50.0f;
        m_currentPressure = m_defaultPressure;
        if (m_readType == ReadType::BREATH_AND_BEND) {
            m_defaultBendPressure = GetPressure(PressureType::BEND) + 50.0f;
            m_currentBendPressure = m_defaultBendPressure;
        }
        return result;
    }

    //--------------
    bool Update(Parameters& params, Message& message) override {
        m_currentPressure += (GetPressure(PressureType::BREATH) - m_currentPressure) * params.breathDelay;
        float p = (m_currentPressure - m_defaultPressure) / 400.0f;
        float v = Clamp(p, 0.0f, 1.0f);
        message.volume = v * v;
        if (m_readType == ReadType::BREATH_AND_BEND) {
            m_currentBendPressure += (GetPressure(PressureType::BEND) - m_currentBendPressure) * params.bendDelay;
            float pb = (m_currentBendPressure - m_defaultBendPressure) / 400.0f;
            float b = Clamp(pb, 0.0f, v);
            float bendNoteShiftTarget = 0.0f;
            if (v > 0.0001f) {
                bendNoteShiftTarget = -1.0f + ((v - b) / v) * 1.2f;
                if (bendNoteShiftTarget > 0.0f) {
                    bendNoteShiftTarget = 0.0f;
                }
            }
            else {
                bendNoteShiftTarget = 0.0f;
            }
            message.bend = bendNoteShiftTarget;
        }
#ifdef DEBUG
        //char s[32];
        //sprintf(s, "%1.1f\n%1.1f", m_currentPressure, m_currentBendPressure);
        //params.debugMessage = s;
#endif
        return true;
    }
private:
    adc_oneshot_unit_handle_t m_adc1;
    adc_oneshot_unit_handle_t m_adc2;

    float m_defaultPressure = 0.0f;
    float m_currentPressure = 0.0f;
    float m_defaultBendPressure = 0.0f;
    float m_currentBendPressure = 0.0f;
    int32_t m_pinBreath = 0;
    int32_t m_pinBend = 0;
    ReadType m_readType;

    //--------------
    float GetPressure(PressureType pressureType) {
        int average_count = 0;
        int averaged = 0;
        for (int i = 0; i < 5; i++) {
            switch (pressureType) {
                case PressureType::BREATH:
                    {
                        int value;
                        if (adc_oneshot_read(m_adc1, ADC_CHANNEL_0, &value) == ESP_OK) {
                        //if (adc2_get_raw(ADC2_CHANNEL_0, ADC_WIDTH_BIT_12, &value) == ESP_OK) {
                        averaged += value;
                        average_count++;
                        }
                    }
                    break;
                case PressureType::BEND:
                    {
                        int value;
                        if (adc_oneshot_read(m_adc2, ADC_CHANNEL_1, &value) == ESP_OK) {
                        //if (adc2_get_raw(ADC2_CHANNEL_1, ADC_WIDTH_BIT_12, &value) == ESP_OK) {
                        averaged += value;
                        average_count++;
                        }
                    }
                    break;
            }
        }
        if (average_count > 0) {
            return averaged / average_count;
        }
        return 0.0f;
    }
};
