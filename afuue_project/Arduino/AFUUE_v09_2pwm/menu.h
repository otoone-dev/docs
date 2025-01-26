#pragma once

#include <string>
#include <Preferences.h>
#include "wavedata.h"
#include "key_system.h"

#define WAVE_MAX (16)
#define FORCEPLAYTIME_LENGTH (200.0f) //ms

// こちらは再生用
struct WaveInfo {
  float lowPassP = 0.1f;
  float lowPassR = 5.0f;
  float lowPassQ = 0.5f;
  float lowPassIDQ = 1.0f / (2 * lowPassQ);

  int baseNote = 61; // 49 61 73
  float fineTune = 440.0f;
  float delayRate = 0.15f;
  float portamentoRate = 0.99f;
};

// こちらは保存用
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
public:
  Menu(M5Canvas* _canvas);
  void Initialize();
  void SetTimer(hw_timer_t * _timer);
  void SetNextWave();
  bool SetNextLowPassQ();
  void ResetPlaySettings(int widx = -1);
#ifdef _M5STICKC_H_
  bool Update(uint16_t key, int pressure);
#endif
#ifdef _STAMPS3_H_
  bool Update2R(volatile WaveInfo* pInfo, const KeySystem* pKey);
#endif
  void BeginPreferences();
  void EndPreferences();
  void SavePreferences();
  void LoadPreferences();
  void ClearAllFlash();
  void Display();
  void DrawBattery(int x, int y);
  void DrawString(const char* str, int sx, int sy);

  WaveData waveData;
  Preferences pref;

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
  int forcePlayTime = 0;

  int ctrlMode = 0;
  bool isLipSensorEnabled = false;
  bool isMidiEnabled = false;
  bool isUSBMidiMounted = false;

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
  hw_timer_t * timer;

  int usePreferencesDepth = 0;
  int cursorPos = 0;
  bool isRtcChanged = false;
  int hour = 0;
  int minute = 0;
  std::string currentKey = "";
  int currentPressure = 0;
  int testNote = 60;

#ifdef _STAMPS3_H_
  int funcDownCount = 0;
#endif
};

