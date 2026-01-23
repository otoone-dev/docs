#include "SoundProcessorBase.h"
#include <vector>

class WaveGenerator : public SoundProcessorBase {
public:
    //--------------
    WaveGenerator(const std::vector<const float*>& waveTable) 
        : m_phase(0.0f)
        , m_waveTableIndex(0)
        , m_waveTable(waveTable)
        , m_pWaveTable(nullptr) {
    }

    //--------------
    void Initialize(const Parameters& params) override {
        // Preferences からデータ復帰したり？
        m_waveTableIndex = params.waveTableIndex;
        m_pWaveTable = m_waveTable[m_waveTableIndex % m_waveTable.size()];
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
        if (m_waveTableIndex != params.waveTableIndex) {
            m_waveTableIndex = params.waveTableIndex;
            m_pWaveTable = m_waveTable[m_waveTableIndex % m_waveTable.size()];
        }
    }
private:
    float m_phase;
    int m_waveTableIndex;
    const std::vector<const float*> m_waveTable;
    const float* m_pWaveTable;
};
