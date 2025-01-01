#ifndef WAVEDATA_H
#define WAVEDATA_H

#include <cstdint>

class WaveData {
public:
  const int32_t GetDrumLength(int index) const;
  const float* GetDrumData(int index) const;

  const float* GetWaveTable(int index, int side);
  const char* GetWaveName(int index);
  float GetWaveShiftLevel(int index);
};

#endif //WAVEDATA_H
