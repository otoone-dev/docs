#ifndef WAVEDATA_H
#define WAVEDATA_H

class WaveData {
public:
  const short* GetWaveTable(int index);
  const char* GetWaveName(int index);
};

#endif //WAVEDATA_H
