#ifndef _WAVE_GENERATOR_H_
#define _WAVE_GENERATOR_H_

#include <Arduino.h>
#include "wavedata.h"

//---------------------------------
struct WaveSettings {
  bool isAccControl = false; // (未対応)加速度センサーを使用するかどうか
  int fineTune = 442; // A3(ラ) の周波数 440Hz とか 442Hz とか。
  int transpose = 0;  // 移調 0:C管 +3(-9):Eb管
  int attackSoftness = 0; // 吹き始めの音量立ち上がりの柔らかさ (0:一番硬い) で数字が増えるほど柔らかくなる
  int portamentoRate = 30; // ポルタメント。音の切り替わりを滑らかにする。0 で即時変更。数字が増えるほどゆっくりになる。
  int delayRate = 15; // ディレイのかかり具合。0 でディレイ無し。
  int pitchDropLevel = 0; // 音量でピッチが下がる量 1/10
  int pitchDropPos = 8; // ピッチが正しい音程になる音量位置 0 - 10 で 0.0-1.0

  int lowPassP = 5; // 音量に対してどこのあたりでフィルタがかかるか 5 が音量中レベル、1 は音量最小、10 が音量MAX
  int lowPassR = 5; // 音量に対してどれくらいの範囲でフィルタがかかるか(フィルタの立ち上がりの急峻度) 1-30
  int lowPassQ = 0; // Q factor(1/10単位) 0 でローパス無効、5 で強調されないローパス、5 より大きくなると高い周波数が元の波形より強調されていきます。
};

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
  WaveGenerator(volatile WaveInfo* pInfo);
  void Initialize();
  void Tick(float note, float td);
  void CreateWave(bool enabled);
  //---------------------------------
  float requestedVolume = 0.0f;
  float noiseVolume = 0.0f;
  const float* currentWaveTable = NULL;

  const float* drum_data[2] = {NULL, NULL};
  volatile int drum_size[2] = {0, 0};
  volatile float drum_pos[2] = {0.0f, 0.0f};
  const float drum_wavelength = 11025.0f / 25000.0f;
  int drum_mode = 0;
  float drumVolume = 0.0f;

  float growlLevel = 0.0f; // 検証中につき OFF
  float growlPos = 0.9f;
  float growlBand = 0.1f;
  float growlPhase = 0.0f;

private:
  //------
  volatile WaveInfo* m_pInfo = NULL;
  WaveData m_waveData;
  int32_t m_waveIndex;
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
