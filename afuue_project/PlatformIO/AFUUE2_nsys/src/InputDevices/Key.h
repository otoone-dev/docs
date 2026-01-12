#pragma once
#include "InputDeviceBase.h"

class Key : public InputDeviceBase {
public:
    Key(TwoWire &wire)
        : InputDeviceBase() {
        m_lastChangeTime = micros();
    }

    //--------------
    const char* GetName() const override {
        return "Key";
    }

protected:
    float GetNote(const Parameters& parameter, uint16_t keyData) {
        uint64_t t = micros();

        if (keyData != m_lastKeyData) {
            m_lastKeyData = keyData;
            m_lastChangeTime = t;
        }
        if (t - m_lastChangeTime > 20*1000) { // ピロ音防止
            const Keys keys = Keys(m_lastKeyData);
            m_targetNote = keys.GetNote(m_baseNote);
        }
        m_currentNote += (m_targetNote - m_currentNote) * m_rate;
        return m_currentNote;
    }

private:
    uint64_t m_lastChangeTime = 0;
    uint16_t m_lastKeyData = 0;
    float m_targetNote = 0.0f;
    float m_currentNote = 0.0f;
    float m_rate = 0.9f;
    int m_baseNote = 48; // C
};
