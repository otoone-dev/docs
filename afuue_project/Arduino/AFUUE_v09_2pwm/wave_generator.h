#pragma once

#include "menu.h"

class WaveGenerator {
public:
  //---------------------------------
  WaveGenerator(volatile WaveInfo* pInfo);
  void Initialize(Menu& menu);
  void Tick(float note);
  void CreateWave(bool enabled);
  //---------------------------------
  float requestedVolume = 0.0f;
  const float* currentWaveTable = NULL;
  volatile uint8_t outH = 0;
  volatile uint8_t outL = 0;

private:
  //---------------------------------
  volatile WaveInfo* m_pInfo = NULL;
  float phase = 0.0f;
  float currentWaveLevel = 0.0f;
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
  float CalcFrequency(float note);
  float InteropL(volatile const float* table, int tableCount, float p);
  float InteropC(volatile const float* table, int tableCount, float p);
  float LowPass(float value);
};

