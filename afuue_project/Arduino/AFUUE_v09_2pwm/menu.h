#ifndef MENU_H
#define MENU_H
#include <string>
#include <Preferences.h>
#include "wavedata.h"

#define WAVE_MAX (16)

struct WaveSettings {
  bool isAccControl = false; // (未対応)加速度センサーを使用するかどうか
  int fineTune = 442; // A3(ラ) の周波数 440Hz とか 442Hz とか。
  int transpose = 0;  // 移調 0:C管 +3(-9):Eb管
  int attackSoftness = 0; // 吹き始めの音量立ち上がりの柔らかさ (0:一番硬い) で数字が増えるほど柔らかくなる
  int portamentoRate = 30; // ポルタメント。音の切り替わりを滑らかにする。0 で即時変更。数字が増えるほどゆっくりになる。
  int delayRate = 15; // ディレイのかかり具合。0 でディレイ無し。

  int lowPassP = 5; // 音量に対してどこのあたりでフィルタがかかるか 5 が音量中レベル、1 は音量最小、10 が音量MAX
  int lowPassR = 5; // 音量に対してどれくらいの範囲でフィルタがかかるか(フィルタの立ち上がりの急峻度) 1-30
  int lowPassQ = 0; // Q factor(1/10単位) 0 でローパス無効、5 で強調されないローパス、5 より大きくなると高い周波数が元の波形より強調されていきます。
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

  M5Canvas* canvas;

  int cursorPos = 0;
  bool isRtcChanged = false;
  int hour = 0;
  int minute = 0;
  std::string currentKey = "";
  int currentPressure = 0;
  int testNote = 60;

public:
  Menu(M5Canvas* _canvas);
  void Initialize(Preferences pref);
  void SetNextWave();
  bool SetNextLowPassQ();
  void ResetPlaySettings(int widx = -1);
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
  int attackSoftness = 0;
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
