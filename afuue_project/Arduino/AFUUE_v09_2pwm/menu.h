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
  int portamentoRate = 15;
  int delayRate = 15;

  int distortion = 0;
  int flanger = 80; // 0 - 50
  int flangerTime = 50; // 0 - 100
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
  void DrawBattery(int x, int y);
  void DrawString(const char* str, int sx, int sy);
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
  bool Update(uint16_t key, int pressure);
  void SavePreferences(Preferences pref);
  void LoadPreferences(Preferences pref);
  void Display();

  WaveData waveData;
  int waveIndex = 0;
  bool isEnabled = false;

  bool isAccControl = false;
  int fineTune = 440;
  int transpose = 0;
  int portamentoRate = 0;
  int delayRate = 0;
  int keySense = 0;
  int breathSense = 0;

  int distortion = 0;
  int flanger = 0;
  int flangerTime = 0;

  WaveSettings waveSettings[WAVE_MAX];
  int forcePlayNote = -1;
};


#endif //MENU_H
