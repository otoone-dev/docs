#pragma once

#include "../Parameters.h"
#include "TrigonometricFunctionTable.h"
#include <cfloat>

struct SoundInfo {
    float note;
    float tickCount;
    float volume;
    float filter;
    float wave;
    SoundInfo(float _note, float _tickCount, float _volume, float _filter, float _wave)
    : note(_note), tickCount(_tickCount), volume(_volume), filter(_filter), wave(_wave)
    {}
};

class SoundProcessorBase {
public:
    virtual void Initialize() = 0;
    virtual void ProcessAudio(SoundInfo& info) = 0;
    virtual void UpdateParameter(const Parameters& params, float volume) = 0;

protected:
    float InteropL(const float* table, int tableCount, float p) const {
        float v = (tableCount - FLT_MIN) *p;
        int t = (int)v;
        v -= t;
        float w0 = table[t];
        t = (t + 1) % tableCount;
        float w1 = table[t];
        return (w0 * (1.0f - v)) + (w1 * v);
    }

    //---------------------------------
    // テーブルの補間計算 (Tan などのループしないもの用)
    float InteropC(const float* table, int tableCount, float p) const {
        float v = (tableCount - FLT_MIN)*p;
        int t = (int)v;
        v -= t;
        float w0 = table[t];
        t = (t + 1) % (tableCount + 1);
        float w1 = table[t];
        return (w0 * (1.0f - v)) + (w1 * v);
    }

};
