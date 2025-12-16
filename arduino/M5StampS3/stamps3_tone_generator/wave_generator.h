#ifndef _WAVE_GENERATOR_H_
#define _WAVE_GENERATOR_H_

#include <Arduino.h>
#include "wavedata.h"
#include "menu.h"

// 再生用
struct WaveInfo {
  int baseNote = 61; // 49 61 73
  float fineTune = 440.0f;
  float attackSoftness = 0;
  float portamentoRate = 0.99f;
  float delayRate = 0.15f;
  float pitchDropLevel = 0.0f;
  float pitchDropPos = 0.0f;

  float lowPassP = 0.1f;
  float lowPassR = 5.0f;
  float lowPassQ = 0.5f;
  float lowPassIDQ = 1.0f / (2 * lowPassQ);
  void ApplyFromWaveSettings(WaveSettings waveSettings) {
    lowPassP = waveSettings.lowPassP * 0.1f;
    lowPassR = waveSettings.lowPassR;
    lowPassQ = waveSettings.lowPassQ * 0.1f;
    fineTune = waveSettings.fineTune;
    baseNote = 61 + waveSettings.transpose;
    portamentoRate = 1 - (waveSettings.portamentoRate * 0.01f);
    delayRate = waveSettings.delayRate * 0.01f;
    attackSoftness = waveSettings.attackSoftness * 0.01f;
    pitchDropPos = waveSettings.pitchDropPos / 10.0f;
    pitchDropLevel = -waveSettings.pitchDropLevel / 10.0f;
  }
};

//---------------------------------
class WaveGenerator {
public:
  WaveGenerator();
  void Initialize(WaveData* pWaveData);
  WaveInfo* GetWaveInfo() {
    return &m_info;
  }
  void Tick(float note, float td);
  void CreateWave(bool enabled);
  //---------------------------------
  float requestedVolume = 0.0f;
  float noiseVolume = 0.0f;
  const float* currentWaveTable = NULL;

  float growlLevel = 0.0f; // 検証中につき OFF
  float growlPos = 0.9f;
  float growlBand = 0.1f;
  float growlPhase = 0.0f;

private:
  //------
  WaveInfo m_info;
  float phase = 0.0f;
  float currentWaveLevel = 0.0f;
  float volumeShift = 1.0f;

  const static int DELAY_BUFFER_SIZE = 7993;
  float delayBuffer[DELAY_BUFFER_SIZE];
  int delayPos = 0;

  const float* sinTable = NULL;
  const float* tanhTable = NULL;

  float lp_a0 = 1.0f;
  float lp_a1 = 0.0f;
  float lp_a2 = 0.0f;
  float lp_b0 = 0.0f;
  float lp_b1 = 0.0f;
  float lp_b2 = 0.0f;
  float currentWavelength = 0.0f;
  float currentWavelengthTickCount = 0.0f;
  float waveShift = 0.0f;
  float lowPassValue = 0.0f;
  //---------------------------------
  float CalcFrequency(float note) const;
  float InteropL(volatile const float* table, int tableCount, float p) const;
  float InteropC(volatile const float* table, int tableCount, float p) const;
  float LowPass(float value);
};

#define SOUND_TWOPWM
#define PWMPIN_LOW (39)
#define PWMPIN_HIGH (40)

#define CLOCK_DIVIDER (80)
#define TIMER_ALARM (40)
//#define SAMPLING_RATE (20000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*50) = 20kHz
#define SAMPLING_RATE (25000.0f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*40) = 25kHz
//#define SAMPLING_RATE (32000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (50*50) = 32kHz
//#define SAMPLING_RATE (44077.13f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (55*33) = 44kHz
//#define SAMPLING_RATE (48019.2f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (49*34) = 48kHz

#define WAVEOUT_BUFFERSIZE (3)
extern volatile uint16_t* waveOut;
extern volatile uint32_t waveOutWriteIdx;

#endif // _WAVE_GENERATOR_H_
