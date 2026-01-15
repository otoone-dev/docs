#pragma once

#include "SoundProcessorBase.h"
#include <cmath>

class LowPassFilter : public SoundProcessorBase {
public:
    void Initialize(const Parameters& params) override {}
    void ProcessAudio(SoundInfo& info) override {
        if (m_lp_a0 != 0.0f) {
            info.wave = LowPass(info.wave);
        }
    }
    void UpdateParameter(const Parameters& params, float volume) override {
        if (m_lp_valueQ > 0.0f) {
            float idQ = 1.0f / (2.0f * (m_lp_valueQ));// + 1.0f * growlRate));

            float a = (tanh(m_lp_valueR * (volume - m_lp_valueP)) + 1.0f) * 0.5f;
            float lp = 100.0f + 20000.0f * a;
            if (lp > 12000.0f) {
                lp = 12000.0f;
            }
            m_lp_value += (lp - m_lp_value) * 0.8f;

            float omega = m_lp_value / params.samplingRate;
            float alpha = InteropL(sinTable, 1024, omega) * idQ;
            float cosv = InteropL(sinTable, 1024, omega + 0.25f);
            float one_minus_cosv = 1.0f - cosv;
            m_lp_a0 =  1.0f + alpha;
            m_lp_a1 = -2.0f * cosv;
            m_lp_a2 =  1.0f - alpha;
            m_lp_b0 = one_minus_cosv * 0.5f;
            m_lp_b1 = one_minus_cosv;
            m_lp_b2 = m_lp_b0;
        }
    }

private:
    float m_lp_valueP = 0.5f;
    float m_lp_valueQ = 0.8f;
    float m_lp_valueR = 5.0f;
    float m_lp_value = 0.0f;

    float m_lp_in1 = 0.0f;
    float m_lp_in2 = 0.0f;
    float m_lp_out1 = 0.0f;
    float m_lp_out2 = 0.0f;

    float m_lp_a0 = 1.0f;
    float m_lp_a1 = 0.0f;
    float m_lp_a2 = 0.0f;
    float m_lp_b0 = 0.0f;
    float m_lp_b1 = 0.0f;
    float m_lp_b2 = 0.0f;

    float LowPass(float value) { 
        float lp = (m_lp_b0 * value 
                    + m_lp_b1 * m_lp_in1 
                    + m_lp_b2 * m_lp_in2
                    - m_lp_a1 * m_lp_out1
                    - m_lp_a2 * m_lp_out2) / m_lp_a0;

        m_lp_in2  = m_lp_in1;
        m_lp_in1  = value;
        m_lp_out2 = m_lp_out1;
        m_lp_out1 = lp;
        return lp;
    }

};
