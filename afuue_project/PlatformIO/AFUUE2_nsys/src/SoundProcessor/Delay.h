#pragma once

#include "SoundProcessorBase.h"

class Delay : public SoundProcessorBase {
public:
    //--------------
    Delay(int bufferSize)
        : m_amount(0.0f)
        , m_time(1)
        , m_pos(0.0f)
    {
        m_buffer.resize(bufferSize, 0.0f);
    }

    //--------------
    void Initialize(const Parameters& params) override {
        m_amount = params.delayAmount;
        m_time = Clamp<int32_t>(static_cast<int32_t>(params.samplingRate * params.delayTime), 1, m_buffer.size());
    }

    //--------------
    // 波形更新（高速呼び出しされる）
    void ProcessAudio(SoundInfo& info) override {
        float w = info.wave + m_buffer[m_pos];
        m_buffer[m_pos] = w * m_amount;
        m_pos = (m_pos + 1) % m_time;
        info.wave = w;
    }
    //--------------
    // パラメータ更新（低速呼び出しされる)
    void UpdateParameter(const Parameters& params, float volume) override {
        m_amount = params.delayAmount;
    }

private:
    float m_amount;
    int m_time;
    int m_pos;
    std::vector<float> m_buffer;
};