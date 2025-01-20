#pragma once

class WaveData {
public:
  const float* GetWaveTable(int index);
  const char* GetWaveName(int index);
  int GetWaveLowPassQ(int index);
  int GetWaveTranspose(int index);
  int GetWavePortamento(int index);
  int GetWaveAttackSoftness(int index);
  const float* GetSinTable() const;
  const float* GetTanhTable() const;
};

