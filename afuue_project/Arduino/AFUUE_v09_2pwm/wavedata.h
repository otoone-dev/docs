#ifndef WAVEDATA_H
#define WAVEDATA_H

class WaveData {
public:
  const float* GetWaveTable(int index, int side) const;
  const char* GetWaveName(int index) const;
  const float* GetSinTable() const;
  const float* GetTanhTable() const;
};

#endif //WAVEDATA_H
