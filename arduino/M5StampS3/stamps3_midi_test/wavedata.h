#pragma once

class WaveData {
public:
  const float* GetWaveTable(int index);
  const char* GetWaveName(int index);
  int GetWaveLowPass(int index, int p);
  int GetWaveTranspose(int index);
  int GetWavePortamento(int index);
  int GetWaveAttackSoftness(int index);
  float GetWaveNoiseLevel(int index);
  float GetWaveAttackNoiseLevel(int index);
  int GetWavePitchDropPos(int index);
  int GetWavePitchDropLevel(int index);

  const float* GetSinTable() const;
  const float* GetTanhTable() const;

  const int GetDrumLength(int index) const;
  const float* GetDrumData(int index) const;
};

