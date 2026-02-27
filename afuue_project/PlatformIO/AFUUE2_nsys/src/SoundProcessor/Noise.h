#pragma once

#include "SoundProcessorBase.h"
#include <math.h>

class Noise : public SoundProcessorBase {
public:
    //--------------
    Noise()
        : m_power(0.0f)
    {
    }

    //--------------
    void Initialize(const Parameters& params) override {
    }

    //--------------
    // 波形更新（高速呼び出しされる）
    void ProcessAudio(SoundInfo& info) override {
        float n = (static_cast<float>(rand()) / RAND_MAX);
        info.wave = (info.wave * (1.0f-m_power) + n * m_power);
    }
    //--------------
    // パラメータ更新（低速呼び出しされる)
    void UpdateParameter(const Parameters& params, Message& message) override {

        float p = 0.0f;
        if (params.info.noiseDecay > 0.0f) {
            p = Clamp(params.info.noiseInitial - message.playTime / params.info.noiseDecay, params.info.noiseSustain, params.info.noiseInitial);
        }
        else {
            p = Clamp(params.info.noiseInitial, 0.0f, 1.0f);
        }
        m_power = p * Clamp(message.volume, 0.0f, 1.0f);
    }

private:
    float m_power;
};