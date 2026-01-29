#pragma once

#include "MenuBase.h"
#include "Parameters.h"

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
    void Update(Parameters& params, Keys& keys) override {
        if (!keys.KeyGs() || !keys.KeyLowCs()) {
            m_waveIndexNum1 = m_waveIndexNum10 = -1;
#ifdef DEBUG
            //char s[32];
            //sprintf(s, "%04x\n%04x", keys.pressed, keys.clicked);
            //params.debugMessage = s;
#endif
            return;
        }
#ifdef DEBUG
            //char s[32];
            //sprintf(s, "%04x\n%04x\nMENU", keys.pressed, keys.clicked);
            //int v = (m_waveIndexNum1 >= 0 ? m_waveIndexNum1 : 0)
            //        + (m_waveIndexNum10 >= 0 ? m_waveIndexNum10 * 10 : 0) - 1;
            //sprintf(s, "%d, %d\n%d", m_waveIndexNum1, m_waveIndexNum10, v);
            //params.debugMessage = s;
#endif
        if (keys.KeyLowC(true)) {
            // 音色番号指定１の桁
            m_waveIndexNum1 = (m_waveIndexNum1 < 0 ? 1 : (m_waveIndexNum1 + 1) % 10);
            SetBeep(params, 60.0f, (m_waveIndexNum1 == 0 ? 500 : 100));
            keys.clicked = 0;
            return;
        }
        if (keys.KeyEb(true)) {
            // 音色番号指定10の桁
            m_waveIndexNum10 = (m_waveIndexNum10 < 0 ? 1 : (m_waveIndexNum10 + 1) % 13);
            SetBeep(params, 72.0f, (m_waveIndexNum10 == 0 ? 500 : 100));
            keys.clicked = 0;
            return;
        }
        if (keys.KeyD(true) || keys.KeyE(true)) {
            // トランスポーズ
            if ((keys.KeyD(true) && keys.KeyE(false))
             || (keys.KeyD(false) && keys.KeyE(true))) {
                params.baseNote = 48.0f;
            } else {
                params.baseNote = Clamp<float>(params.baseNote + (keys.KeyE(true) ? 1.0f : -1.0f), 24.0f, 72.0f);
            }
            int32_t n = static_cast<int32_t>(params.baseNote) % 12;
            params.menuMessage = "Transpose:\n  " + TransposeToStr(static_cast<int32_t>(params.baseNote - 48.0f));
            SetBeep(params, params.baseNote, (n == 0 ? 500 : 100));
            keys.clicked = 0;
            return;
        }
        if (keys.KeyF(true)) {
            // 音色変更決定（数値指定がない場合は +1）
            if (m_waveIndexNum1 >= 0 || m_waveIndexNum10 >= 0) {
                int i = (m_waveIndexNum1 >= 0 ? m_waveIndexNum1 : 0)
                    + (m_waveIndexNum10 >= 0 ? m_waveIndexNum10 * 10 : 0) - 1;
                params.SetWaveTableIndex(i % 128); // MIDI のプログラムチェンジ 0-127 に合わせておく
            }
            else {
                params.SetWaveTableIndex(params.GetWaveTableIndex() + 1);
            }
            m_waveIndexNum1 = m_waveIndexNum10 = -1;
            params.menuMessage = params.info.name;
            SetBeep(params, params.baseNote, 500);
            keys.clicked = 0;   //後段のメニューにキーを渡さない
            return;
        }
        if (keys.KeyG(true)) {
            // ローパスフィルタＱ値変更
            params.info.lowPassQ = Wrap<float>(Step(params.info.lowPassQ, 0.5f) + 0.5f, 0.5f, 3.0f);
            params.menuMessage = Format("LowPassQ:\n  ", params.info.lowPassQ, 1);
            SetBeep(params, 67.0f, (params.info.lowPassQ == 0.5f ? 500 : 100));
            keys.clicked = 0;   //後段のメニューにキーを渡さない
            return;
        }
        if (keys.KeyA(true)) {
            // ブレス感度変更
            params.breathDelay = Wrap<float>(Step(params.breathDelay, 0.1f) + 0.1f, 0.5f, 1.0f);
            params.menuMessage = Format("BreathSense:\n  ", params.breathDelay * 100.0f, 0) + "%";
            SetBeep(params, 71.0f, (params.breathDelay == 0.8f ? 500 : 200));
            keys.clicked = 0;   //後段のメニューにキーを渡さない
            return;
        }
        if (keys.KeyB(true)) {
            // MIDI ブレス信号変更
        }
        if (keys.KeyDown(true)) {
            // 基準音変更
            params.fineTune = Wrap<float>(Step(params.fineTune, 2.0f) + 2.0f, 438.0f, 444.0f);
            params.menuMessage = Format("Fine:\n  ", params.fineTune, 0) + "Hz";
            SetBeep(params, 69.0f, (params.fineTune == 440.0f ? 500 : 200));
            keys.clicked = 0;   //後段のメニューにキーを渡さない
            return;
        }
        if (keys.KeyUp(true)) {
            // ディレイ変更
            params.delayAmount = Wrap<float>(Step(params.delayAmount, 0.15f) + 0.15f, 0.0f, 0.45f);
            params.menuMessage = Format("Delay:\n  ", params.delayAmount * 100.0f, 0) + "%";
            SetBeep(params, 62.0f, (params.delayAmount == 0.15f ? 500 : 100));
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
