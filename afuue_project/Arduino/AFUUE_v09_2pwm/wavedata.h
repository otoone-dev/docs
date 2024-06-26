#ifndef WAVEDATA_H
#define WAVEDATA_H

class WaveData {
public:
  const double* GetWaveTable(int index, int side);
  const char* GetWaveName(int index);
  double GetWaveShiftLevel(int index);
};

#endif //WAVEDATA_H
