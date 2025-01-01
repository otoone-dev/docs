#ifndef WAVEDATA_H
#define WAVEDATA_H

class WaveData {
public:
  const float* GetWaveTable(int index, int side);
  const char* GetWaveName(int index);
  float GetWaveShiftLevel(int index);
  int GetWaveLowPassQ(int index);
  int GetWaveTranspose(int index);
};

#endif //WAVEDATA_H
