#pragma once

#include "SoundProcessorBase.h"

#define DELAY_BUFFER_SIZE (7993)

class Delay : public SoundProcessorBase {
private:
    float m_buffer[DELAY_BUFFER_SIZE];
    float m_rate = 0.15f;
    int m_pos = 0;
public:
    void Initialize(const Parameters& params) override {}
    void ProcessAudio(SoundInfo& info) override {
        float w = info.wave + m_buffer[m_pos];
        m_buffer[m_pos] = w * m_rate;
        m_pos = (m_pos + 1) % DELAY_BUFFER_SIZE;
        info.wave = w;
    }
    void UpdateParameter(const Parameters& params, float volume) override {
        // Update parameters based on the provided Parameters struct
    }
};