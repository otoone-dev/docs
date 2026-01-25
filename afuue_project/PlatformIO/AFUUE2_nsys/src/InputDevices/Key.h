#pragma once
#include "InputDeviceBase.h"

class Key : public InputDeviceBase {
public:
    Key()
        : InputDeviceBase() {
        m_lastChangeTime = micros();
    }

    //--------------
    const char* GetName() const override {
        return "Key";
    }

    //--------------
    bool Update(Parameters& params, Message& message) override {
        message.keyData = GetKeyData();
        uint64_t t = micros();

        if (message.keyData != m_lastKeyData) {
            m_lastKeyData = message.keyData;
            m_lastChangeTime = t;
        }
        if (t - m_lastChangeTime > params.keyDelay) { // ピロ音防止
            const Keys keys = Keys(m_lastKeyData);
            m_targetNote = keys.GetNote(params.baseNote) + message.bend;
            if (params.info.dropPos > 0.0f && message.volume < params.info.dropPos) { // 音量によるピッチダウン
                m_targetNote -= (1.0f-(message.volume / params.info.dropPos)) * params.info.dropAmount;
            }
        }
        m_currentNote += (m_targetNote - m_currentNote) * params.info.portamento;
        message.note = m_currentNote;
        return true;
    }

protected:
    //--------------
    virtual uint16_t GetKeyData() = 0;

private:
    uint64_t m_lastChangeTime = 0;
    uint16_t m_lastKeyData = 0;
    float m_targetNote = 0.0f;
    float m_currentNote = 0.0f;
};
