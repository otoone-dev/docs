#include "SoundProcessorBase.h"

class WaveGenerator : public SoundProcessorBase {
public:
    void Initialize(const Parameters& params) override {
        // Preferences からデータ復帰したり？
        m_pWaveTable = params.pWaveTable;
    }

    void ProcessAudio(SoundInfo& info) override {
        m_phase += info.tickCount;
        if (m_phase >= 1.0f) m_phase -= 1.0f;

        info.wave = InteropL(m_pWaveTable, 256, m_phase) * info.volume;
    }

    void UpdateParameter(const Parameters& params, float volume) override {
        if (m_pWaveTable != params.pWaveTable) {
            m_pWaveTable = params.pWaveTable;
        }
    }
private:
    float m_phase = 0.0f;
    const float* m_pWaveTable = nullptr;
};
