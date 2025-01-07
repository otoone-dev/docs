#ifndef WAVEDATA_H
#define WAVEDATA_H

class WaveData {
public:
  const float* GetWaveTable(int index);
  const char* GetWaveName(int index);
  int GetWaveLowPassQ(int index);
  int GetWaveTranspose(int index);
  const float* GetSinTable() const;
  const float* GetTanhTable() const;
};

#endif //WAVEDATA_H
