#pragma once

#include <string>
#include <functional>
#include <utility>
#include <Preferences.h>
#include "wavedata.h"
#include "key_system.h"
#include "afuue_midi.h"

#define WAVE_MAX (16)
#define FORCEPLAYTIME_LENGTH (200.0f) //ms

// 保存用
struct WaveSettings {
  bool isAccControl = false; // (未対応)加速度センサーを使用するかどうか
  int fineTune = 442; // A3(ラ) の周波数 440Hz とか 442Hz とか。
  int transpose = 0;  // 移調 0:C管 +3(-9):Eb管
  int attackSoftness = 0; // 吹き始めの音量立ち上がりの柔らかさ (0:一番硬い) で数字が増えるほど柔らかくなる
  int portamentoRate = 30; // ポルタメント。音の切り替わりを滑らかにする。0 で即時変更。数字が増えるほどゆっくりになる。
  int delayRate = 15; // ディレイのかかり具合。0 でディレイ無し。
  int pitchDropLevel = 0; // 音量でピッチが下がる量 1/10
  int pitchDropPos = 8; // ピッチが正しい音程になる音量位置 0 - 10 で 0.0-1.0

  int lowPassP = 5; // 音量に対してどこのあたりでフィルタがかかるか 5 が音量中レベル、1 は音量最小、10 が音量MAX
  int lowPassR = 5; // 音量に対してどれくらいの範囲でフィルタがかかるか(フィルタの立ち上がりの急峻度) 1-30
  int lowPassQ = 0; // Q factor(1/10単位) 0 でローパス無効、5 で強調されないローパス、5 より大きくなると高い周波数が元の波形より強調されていきます。
};

// 再生用
struct WaveInfo {
  int baseNote = 61; // 49 61 73
  float fineTune = 440.0f;
  float attackSoftness = 0;
  float portamentoRate = 0.99f;
  float delayRate = 0.15f;
  float pitchDropLevel = 0.0f;
  float pitchDropPos = 0.0f;

  float lowPassP = 0.1f;
  float lowPassR = 5.0f;
  float lowPassQ = 0.5f;
  float lowPassIDQ = 1.0f / (2 * lowPassQ);
  void ApplyFromWaveSettings(WaveSettings waveSettings) {
    lowPassP = waveSettings.lowPassP * 0.1f;
    lowPassR = waveSettings.lowPassR;
    lowPassQ = waveSettings.lowPassQ * 0.1f;
    fineTune = waveSettings.fineTune;
    baseNote = 61 + waveSettings.transpose;
    portamentoRate = 1 - (waveSettings.portamentoRate * 0.01f);
    delayRate = waveSettings.delayRate * 0.01f;
    attackSoftness = waveSettings.attackSoftness * 0.01f;
    pitchDropPos = waveSettings.pitchDropPos / 10.0f;
    pitchDropLevel = -waveSettings.pitchDropLevel / 10.0f;
  }
};

// メニュー項目
struct MenuProperties {
  std::string name; // 項目名
  std::function<std::string()> valueFunc; // 項目の値を返す
  std::function<void()> plusFunc; // 値を増やす
  std::function<void()> minusFunc; // 値を減らす
  MenuProperties(std::string _name, std::function<std::string()> _valueFunc, std::function<void()> _plusFunc, std::function<void()> _minusFunc) {
    name = _name;
    valueFunc = _valueFunc;
    plusFunc = _plusFunc;
    minusFunc = _minusFunc;
  }
  MenuProperties() {
    name = "------";
    valueFunc = std::function<std::string()>();
    plusFunc = std::function<void()>();
    minusFunc = std::function<void()>();
  }
};

class Menu {
public:
  Menu(M5Canvas* _canvas, AfuueMIDI* _midi);
  void Initialize();
  void SetTimer(hw_timer_t * _timer);
  void SetNextWave();
  bool SetWaveIndex(int widx);
  bool SetNextLowPassQ();
  void ResetPlaySettings(int widx = -1);
  bool Update(uint16_t key, int pressure);
  bool Update2R(const KeySystem* pKey);
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

  int keySense = 0;
  int breathSense = 0;
  int breathZero = 0;

  WaveSettings currentWaveSettings;
  WaveSettings waveSettings[WAVE_MAX];
  int forcePlayNote = -1;
  int forcePlayTime = 0;

  int pgNumHigh = 0;
  int pgNumLow = 0;
  int ctrlMode = 0;
  bool isLipSensorEnabled = false;
  bool isMidiEnabled = false;
  bool isUSBMidiMounted = false;
  int drumVolume = 0;

private:
  void WritePlaySettings(int widx);
  void ReadPlaySettings(int widx);
  void WriteRtc();
  void ReadRtc();
  std::string TransposeToStr() const;
  std::string TimeToStr() const;
  std::string NoteNumberToStr() const;
  void DisplayLine(int line, bool selected, const std::string& title, const std::string& value);
  void DisplayMenu();
  void DisplayPerform(bool onlyRefreshTime = false);

  M5Canvas* canvas;
  AfuueMIDI* afuueMidi;
  hw_timer_t * timer;

  int usePreferencesDepth = 0;
  int cursorPos = 0;
  bool isRtcChanged = false;
  int hour = 0;
  int minute = 0;
  std::string currentKey = "";
  int currentPressure = 0;
  int testNote = 60;
  float bat_per = 0.0f;
  int funcDownCount = 0;

  std::vector<MenuProperties> m_menus;
};

