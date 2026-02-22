#pragma once

#include "SoundProcessorBase.h"
#include <math.h>

class LFO : public SoundProcessorBase {
public:
    //--------------
    LFO()
        : m_power(0.0f)
        , m_freq(1.0f)
    {
    }

    //--------------
    void Initialize(const Parameters& params) override {
    }

    //--------------
    // 波形更新（高速呼び出しされる）
    void ProcessAudio(SoundInfo& info) override {
    }
    //--------------
    // パラメータ更新（低速呼び出しされる)
    void UpdateParameter(const Parameters& params, Message& message) override {
        m_power = params.info.lfoPower * message.volume;
        m_freq = params.info.lfoFreq;
        if (m_power > 0.0f) {
            float t = static_cast<int32_t>(millis()) / 1000.0f;
            float nn = message.note + m_power * TableSine(m_freq * t);
            float n = Clamp(nn, 1.0f, 127.0f);
            message.note = n;
        }
    }

private:
    float m_power;
    float m_freq;
};