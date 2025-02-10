#pragma once

#include "menu.h"

class WaveGenerator {
public:
  //---------------------------------
  WaveGenerator(volatile WaveInfo* pInfo);
  void Initialize(Menu& menu);
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
  bool drum_mode = 0;
  float drumVolume = 0.0f;

  float growlLevel = 0.0f; // 検証中につき OFF
  float growlPos = 0.9f;
  float growlBand = 0.1f;
  float growlPhase = 0.0f;

private:
  //---------------------------------
  volatile WaveInfo* m_pInfo = NULL;
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

