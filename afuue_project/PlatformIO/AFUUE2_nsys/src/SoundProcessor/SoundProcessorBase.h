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
    float deltaTime;
    SoundInfo(float _note, float _tickCount, float _volume, float _filter, float _wave, float _deltaTime)
    : note(_note), tickCount(_tickCount), volume(_volume), filter(_filter), wave(_wave), deltaTime(_deltaTime)
    {}
};

class SoundProcessorBase {
public:
    virtual void Initialize(const Parameters& params) = 0;
    virtual void ProcessAudio(SoundInfo& info) = 0;
    virtual void UpdateParameter(const Parameters& params, Message& message) = 0;

protected:
    //---------------------------------
    static float TableSine(float p) {
        return InteropL(sinTable, 1024, p);
    }

    //---------------------------------
    // テーブルの補間計算 (Sin などのループするもの用)
    static float InteropL(const float* table, int tableCount, float p) {
        float v = (tableCount - FLT_MIN) * p;
        if (v < 0) {
            v = -v; // マイナスで反転するので注意
        }
        int t = (int)v;
        v -= t;
        float w0 = table[t % tableCount];
        t = (t + 1) % tableCount;
        float w1 = table[t];
        return (w0 * (1.0f - v)) + (w1 * v);
    }

    //---------------------------------
    // テーブルの補間計算 (Tan などのループしないもの用)
    static float InteropC(const float* table, int tableCount, float p) {
        float v = (tableCount - FLT_MIN) * p;
        if (v < 0) {
            v = -v; // マイナスで反転するので注意
        }
        int t = (int)v;
        v -= t;
        float w0 = table[(t < 0 ? 0 : (t >= tableCount ? tableCount - 1 : t))];
        t++;
        float w1 = table[(t < 0 ? 0 : (t >= tableCount ? tableCount - 1 : t))];
        return (w0 * (1.0f - v)) + (w1 * v);
    }

};
