#pragma once

#include "MenuBase.h"

class MenuForKey : public MenuBase {
public:
    MenuForKey() = default;
    virtual ~MenuForKey() = default;

    //--------------
    const char* GetName() const override {
        return "Menu";
    }

    //--------------
    InitializeResult Initialize() override {
        return InitializeResult();
    }
    
    //--------------
    void Update(Parameters& parameters, Keys& keys) override {
        if (!keys.KeyGs() || !keys.KeyLowCs()) {
            m_waveIndexNum1 = m_waveIndexNum10 = -1;
#ifdef DEBUG
            //char s[32];
            //sprintf(s, "%04x\n%04x", keys.pressed, keys.clicked);
            //parameters.debugMessage = s;
#endif
            return;
        }
#ifdef DEBUG
            char s[32];
            //sprintf(s, "%04x\n%04x\nMENU", keys.pressed, keys.clicked);
            int v = (m_waveIndexNum1 >= 0 ? m_waveIndexNum1 : 0)
                    + (m_waveIndexNum10 >= 0 ? m_waveIndexNum10 * 10 : 0) - 1;
            sprintf(s, "%d, %d\n%d", m_waveIndexNum1, m_waveIndexNum10, v);
            parameters.debugMessage = s;
#endif
        if (keys.KeyLowC(true)) {
            m_waveIndexNum1 = (m_waveIndexNum1 < 0 ? 1 : (m_waveIndexNum1 + 1) % 10);
            SetBeep(parameters, 60.0f, (m_waveIndexNum1 == 0 ? 500 : 100));
            keys.clicked = 0;
            return;
        }
        if (keys.KeyEb(true)) {
            m_waveIndexNum10 = (m_waveIndexNum10 < 0 ? 1 : (m_waveIndexNum10 + 1) % 13);
            SetBeep(parameters, 72.0f, (m_waveIndexNum10 == 0 ? 500 : 100));
            keys.clicked = 0;
            return;
        }
        if (keys.KeyD(true) || keys.KeyE(true)) {
            if ((keys.KeyD(true) && keys.KeyE(false))
             || (keys.KeyD(false) && keys.KeyE(true))) {
                parameters.baseNote = 48.0f;
            } else {
                parameters.baseNote = Clamp<float>(parameters.baseNote + (keys.KeyE(true) ? 1.0f : -1.0f), 24.0f, 72.0f);
            }
            int32_t n = static_cast<int32_t>(parameters.baseNote) % 12;
            SetBeep(parameters, parameters.baseNote, (n == 0 ? 500 : 100));
            keys.clicked = 0;
            return;
        }
        if (keys.KeyF(true)) {
            if (m_waveIndexNum1 >= 0 || m_waveIndexNum10 >= 0) {
                int i = (m_waveIndexNum1 >= 0 ? m_waveIndexNum1 : 0)
                    + (m_waveIndexNum10 >= 0 ? m_waveIndexNum10 * 10 : 0) - 1;
                parameters.waveTableIndex = i % 128; // MIDI のプログラムチェンジ 0-127 に合わせておく
            }
            else {
                parameters.waveTableIndex++;
            }
            m_waveIndexNum1 = m_waveIndexNum10 = -1;
            SetBeep(parameters, parameters.baseNote, 500);
            keys.clicked = 0;   //後段のメニューにキーを渡さない
            return;
        }
    }
private:
    int32_t m_waveIndexNum1 = -1;
    int32_t m_waveIndexNum10 = -1;

    void SetBeep(Parameters& parameters, float note, int millisecond) {
        parameters.beepTime = micros() + millisecond * 1000;
        parameters.beepNote = note;
    }
};
