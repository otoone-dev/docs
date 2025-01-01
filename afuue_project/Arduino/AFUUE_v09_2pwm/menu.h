#ifndef MENU_H
#define MENU_H
#include <string>
#include <Preferences.h>
#include "wavedata.h"

#define WAVE_MAX (16)

struct WaveSettings {
  bool isAccControl = false;
  int fineTune = 442;
  int transpose = 0;
  int portamentoRate = 30;
  int delayRate = 15;

  int lowPassP = 5; // 音量に対してどこのあたりでフィルタがかかるか。1/10 で 0.0-1.0
  int lowPassR = 5; // 音量に対してどれくらいの範囲でフィルタがかかるか 1-30
  int lowPassQ = 0; // Q factor 0 でローパス無効
};

class Menu {
private:
  void WritePlaySettings(int widx);
  void ReadPlaySettings(int widx);
  void WriteRtc();
  void ReadRtc();
  std::string TransposeToStr();
  std::string TimeToStr();
  std::string NoteNumberToStr();
  void DisplayLine(int line, bool selected, const std::string& title, const std::string& value);
  void DisplayMenu();
  void DisplayPerform(bool onlyRefreshTime = false);

  int cursorPos = 0;
  bool isRtcChanged = false;
  int hour = 0;
  int minute = 0;
  std::string currentKey = "";
  int currentPressure = 0;
  int testNote = 60;

public:
  Menu();
  void Initialize(Preferences pref);
  void SetNextWave();
  bool SetNextLowPassQ();
  bool Update(Preferences pref, uint16_t key, int pressure);
  void SavePreferences(Preferences pref);
  void LoadPreferences(Preferences pref);
  void Display();
  void DrawBattery(int x, int y);
  void DrawString(const char* str, int sx, int sy);

  WaveData waveData;
  int waveIndex = 0;
  bool isEnabled = false;
  bool factoryResetRequested = false;

  int fineTune = 440;
  int transpose = 0;
  int portamentoRate = 0;
  int delayRate = 0;
  int keySense = 0;
  int breathSense = 0;
  int breathZero = 0;

  int lowPassP = 0;
  int lowPassR = 0;
  int lowPassQ = 0;

  WaveSettings waveSettings[WAVE_MAX];
  int forcePlayNote = -1;
};


#endif //MENU_H
