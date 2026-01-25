#include "SoundProcessorBase.h"
#include <vector>

class WaveGenerator : public SoundProcessorBase {
public:
    //--------------
    WaveGenerator() 
        : m_waveIndex(0)
        , m_phase(0.0f) {}

    //--------------
    void Initialize(const Parameters& params) override {
        // Preferences からデータ復帰したり？
        m_waveIndex = params.GetWaveTableIndex();
        m_pWaveTable = params.info.pWave;
    }

    //--------------
    // 波形更新（高速呼び出しされる）
    void ProcessAudio(SoundInfo& info) override {
        m_phase += info.tickCount;
        if (m_phase >= 1.0f) m_phase -= 1.0f;

        info.wave = InteropL(m_pWaveTable, 256, m_phase) * info.volume;
    }

    //--------------
    // パラメータ更新（低速呼び出しされる)
    void UpdateParameter(const Parameters& params, float volume) override {
        int i = params.GetWaveTableIndex() % params.GetWaveTableCount();
        if (m_waveIndex != i) {
            m_waveIndex = i;
            m_pWaveTable = params.info.pWave;
        }
    }
private:
    int m_waveIndex;
    float m_phase;
    const float* m_pWaveTable;
};
