#include "afuue_common.h"
#include "afuue_midi.h"
#include "logo.h"
#include "ascii_fonts.h"
#include "wavedata.h"
#include "menu.h"
#ifdef HAS_IOEXPANDER
#include "io_expander.h"
#endif

//--------------------------
Menu::Menu(M5Canvas* _canvas, AfuueMIDI* _midi) {
  canvas = _canvas;
  afuueMidi = _midi;
}

//--------------------------
// 初期化
void Menu::Initialize() {
  BeginPreferences();
  {
    int ver = pref.getInt("AfuueVer", -1);
    if (ver != AFUUE_VER) {
      pref.clear(); // CLEAR ALL FLASH MEMORY
      pref.putInt("AfuueVer", AFUUE_VER);
    }
    LoadPreferences();
  } EndPreferences();
#ifdef HAS_DISPLAY
  canvas->setColorDepth(16);
  canvas->createSprite(DISPLAY_HEIGHT, DISPLAY_WIDTH);
#endif

  // プロパティ編集定義 MenuProperties(表示名, 表示値取得, 増やす処理, 減らす処理)
  m_menus.emplace_back(MenuProperties("Transpose",
    [&](){ return TransposeToStr(); },
    [&](){
      currentWaveSettings.transpose++;
      if (currentWaveSettings.transpose > 12) currentWaveSettings.transpose = 12;
    },
    [&](){
      currentWaveSettings.transpose--;
      if (currentWaveSettings.transpose < -12) currentWaveSettings.transpose = -12;
    } ));
  m_menus.emplace_back(MenuProperties("LP_Pos",
    [&](){ return std::to_string(currentWaveSettings.lowPassP); },
    [&](){
      currentWaveSettings.lowPassP++;
      if (currentWaveSettings.lowPassP > 10) currentWaveSettings.lowPassP = 10;
    },
    [&](){
      currentWaveSettings.lowPassP--;
      if (currentWaveSettings.lowPassP < 1) currentWaveSettings.lowPassP = 1;
    } ));
  m_menus.emplace_back(MenuProperties("LP_Rate",
    [&](){ return std::to_string(currentWaveSettings.lowPassR); },
    [&](){
      currentWaveSettings.lowPassR++;
      if (currentWaveSettings.lowPassR > 30) currentWaveSettings.lowPassR = 30;
    },
    [&](){
      currentWaveSettings.lowPassR--;
      if (currentWaveSettings.lowPassR < 1) currentWaveSettings.lowPassR = 1;
    } ));
  m_menus.emplace_back(MenuProperties("LP_Power",
    [&](){ return std::to_string(currentWaveSettings.lowPassQ); },
    [&](){
      currentWaveSettings.lowPassQ++;
      if (currentWaveSettings.lowPassQ > 50) currentWaveSettings.lowPassQ = 50;
    },
    [&](){
      currentWaveSettings.lowPassQ--;
      if (currentWaveSettings.lowPassQ < 0) currentWaveSettings.lowPassQ = 0;
    } ));
  m_menus.emplace_back(MenuProperties("Attack",
    [&](){ return std::to_string(currentWaveSettings.attackSoftness); },
    [&](){
      currentWaveSettings.attackSoftness++;
      if (currentWaveSettings.attackSoftness > 99) currentWaveSettings.attackSoftness = 99;
    },
    [&](){
      currentWaveSettings.attackSoftness--;
      if (currentWaveSettings.attackSoftness < 0) currentWaveSettings.attackSoftness = 0;
    } ));
  m_menus.emplace_back(MenuProperties("Drop",
    [&]() -> std::string {
      if (currentWaveSettings.pitchDropLevel > 0) {
        return std::to_string(currentWaveSettings.pitchDropLevel);
      }
      return "OFF";
    },
    [&](){
      currentWaveSettings.pitchDropLevel++;
      if (currentWaveSettings.pitchDropLevel > 20) currentWaveSettings.pitchDropLevel = 20;
    },
    [&](){
      currentWaveSettings.pitchDropLevel--;
      if (currentWaveSettings.pitchDropLevel < 0) currentWaveSettings.pitchDropLevel = 0;
    } ));
  m_menus.emplace_back(MenuProperties("DropPos",
    [&](){ return std::to_string(currentWaveSettings.pitchDropPos); },
    [&](){
      currentWaveSettings.pitchDropPos++;
      if (currentWaveSettings.pitchDropPos > 10) currentWaveSettings.pitchDropPos = 10;
    },
    [&](){
      currentWaveSettings.pitchDropPos--;
      if (currentWaveSettings.pitchDropPos < 0) currentWaveSettings.pitchDropPos = 0;
    } ));
  m_menus.emplace_back(MenuProperties("FineTune",
    [&](){ return std::to_string(currentWaveSettings.fineTune); },
    [&](){
      currentWaveSettings.fineTune++;
      if (currentWaveSettings.fineTune > 480) currentWaveSettings.fineTune = 480;
    },
    [&](){
      currentWaveSettings.fineTune--;
      if (currentWaveSettings.fineTune < 400) currentWaveSettings.fineTune = 400;
    } ));
  m_menus.emplace_back(MenuProperties("Portamento",
    [&](){ return std::to_string(currentWaveSettings.portamentoRate); },
    [&](){
      currentWaveSettings.portamentoRate++;
      if (currentWaveSettings.portamentoRate > 99) currentWaveSettings.portamentoRate = 99;
    },
    [&](){
      currentWaveSettings.portamentoRate--;
      if (currentWaveSettings.portamentoRate < 0) currentWaveSettings.portamentoRate = 0;
    } ));
  m_menus.emplace_back(MenuProperties("Delay",
    [&](){ return std::to_string(currentWaveSettings.delayRate); },
    [&](){
      currentWaveSettings.delayRate++;
      if (currentWaveSettings.delayRate > 99) currentWaveSettings.delayRate = 99;
    },
    [&](){
      currentWaveSettings.delayRate--;
      if (currentWaveSettings.delayRate < 0) currentWaveSettings.delayRate = 0;
    } ));
  m_menus.emplace_back(MenuProperties());
  m_menus.emplace_back(MenuProperties("AccDrum",
    [&]() { if (drumVolume==0) return std::string("OFF"); return std::to_string(drumVolume); },
    [&]() {
      if (drumVolume < 9) drumVolume++;
    },
    [&]() {
      if (drumVolume > 0) drumVolume--;
    }));
  m_menus.emplace_back(MenuProperties("KeySense",
    [&](){ return std::to_string(keySense); },
    [&](){
      keySense += 5;
      if (keySense > 100) keySense = 100;
    },
    [&](){
      keySense -= 5;
      if (keySense < 0) keySense = 0;
    } ));
  m_menus.emplace_back(MenuProperties("BrthSense", 
    [&](){ return std::to_string(breathSense); },
    [&](){
#ifdef USE_MCP3425
      breathSense += 50;
      if (breathSense > 500) breathSense = 500;
#else
      breathSense += 10;
      if (breathSense > 500) breathSense = 500;
#endif
    },
    [&](){
#ifdef USE_MCP3425
      breathSense -= 50;
      if (breathSense < 0) breathSense = 0;
#else
      breathSense -= 10;
      if (breathSense < 100) breathSense = 100;
#endif
    } ));
  m_menus.emplace_back(MenuProperties("BrthZero",
    [&](){ return std::to_string(breathZero); },
    [&](){
      breathZero += 10;
      if (breathZero > 300) breathZero = 300;
    },
    [&](){
      breathZero -= 10;
      if (breathZero < 0) breathZero = 0;
    } ));
#ifdef HAS_RTC
  m_menus.emplace_back(MenuProperties("Clock",
    [&](){ return TimeToStr(); },
    [&](){
      minute = (minute + 1)%60;
      if (minute == 0) {
        hour = (hour + 1)%24;
      }
      isRtcChanged = true;
    },
    [&](){
      minute = (minute + 59) % 60;
      if (minute == 59) {
        hour = (hour + 23)%24;
      }
      isRtcChanged = true;
    } ));
#endif
  m_menus.emplace_back(MenuProperties());
  m_menus.emplace_back(MenuProperties("KeyTest",
    [&](){ return currentKey; },
    [&](){},
    [&](){} ));
  m_menus.emplace_back(MenuProperties("BrthTest",
    [&](){ return std::to_string(currentPressure); },
    [&](){},
    [&](){} ));
  m_menus.emplace_back(MenuProperties("SpkTest",
    [&](){ return NoteNumberToStr(); },
    [&](){
      testNote++;
      if (testNote > 96) testNote = 96;
      forcePlayNote = testNote;
      forcePlayTime = FORCEPLAYTIME_LENGTH;
    },
    [&](){
      testNote--;
      if (testNote < 12) testNote = 12;
      forcePlayNote = testNote;
      forcePlayTime = FORCEPLAYTIME_LENGTH;
    } ));
}

//-------------------------------------
void Menu::SetTimer(hw_timer_t * _timer) {
  timer = _timer;
}

//-------------------------------------
// Flash書き込み開始
void Menu::BeginPreferences() {
  usePreferencesDepth++;
  if (usePreferencesDepth > 1) return;
  if (timer) timerAlarmDisable(timer);
  delay(10);
  pref.begin("Afuue/Settings", false);
}

//--------------------------
// Flash書き込み終了
void Menu::EndPreferences() {
  usePreferencesDepth--;
  if (usePreferencesDepth == 0) {
    pref.end();  
    delay(10);
    if (timer) timerAlarmEnable(timer);
  }
  if (usePreferencesDepth < 0) usePreferencesDepth = 0;
}

//--------------------------
// Flashに保存
void Menu::SavePreferences() {
#ifdef HAS_DISPLAY
  for (int i = 0; i < WAVE_MAX; i++) {
    if (waveData.GetWaveTable(i) == NULL) {
      break;
    }

    char s[32];
    //sprintf(s, "AccControl%d", i);
    //pref.putBool(s, waveSettings[i].isAccControl);
    sprintf(s, "FineTune%d", i);
    pref.putInt(s, waveSettings[i].fineTune);
    sprintf(s, "Transpose%d", i);
    pref.putInt(s, waveSettings[i].transpose);
    sprintf(s, "AttackSoftness%d", i);
    pref.putInt(s, waveSettings[i].attackSoftness);
    sprintf(s, "PitchDropLevel%d", i);
    pref.putInt(s, waveSettings[i].pitchDropLevel);
    sprintf(s, "PitchDropPos%d", i);
    pref.putInt(s, waveSettings[i].pitchDropPos);
    sprintf(s, "PortamentoRate%d", i);
    pref.putInt(s, waveSettings[i].portamentoRate);
    sprintf(s, "DelayRate%d", i);
    pref.putInt(s, waveSettings[i].delayRate);
    sprintf(s, "LowPassP%d", i);
    pref.putInt(s, waveSettings[i].lowPassP);
    sprintf(s, "LowPassR%d", i);
    pref.putInt(s, waveSettings[i].lowPassR);
    sprintf(s, "LowPassQ%d", i);
    pref.putInt(s, waveSettings[i].lowPassQ);
  }
#endif
  pref.putInt("KeySense", keySense);
  pref.putInt("BreathSense", breathSense);
#ifdef HAS_LIPSENSOR
  pref.putInt("BreathSense2", breathSense);
#endif
  pref.putInt("BreathZero", breathZero);
  pref.putInt("WaveIndex", waveIndex);
}

//--------------------------
// Flash初期化
void Menu::ClearAllFlash(){
  pref.clear(); // CLEAR ALL FLASH MEMORY
}

//--------------------------
// 現在の値を記録バッファから読み出し
void Menu::ReadPlaySettings(int widx) {
  currentWaveSettings = waveSettings[widx];
}

//--------------------------
// 現在の値を記録用バッファに書き込み
void Menu::WritePlaySettings(int widx) {
  waveSettings[widx] = currentWaveSettings;
}

//--------------------------
// 現在の設定をリセット
void Menu::ResetPlaySettings(int widx) {
  int idx = widx;
  if (idx < 0) {
    idx = waveIndex;
  }
  // currentWaveSettings.isAccControl = false;
  currentWaveSettings.fineTune = 442;
  currentWaveSettings.transpose = waveData.GetWaveTranspose(idx);
  currentWaveSettings.attackSoftness = waveData.GetWaveAttackSoftness(idx);
  currentWaveSettings.pitchDropLevel = waveData.GetWavePitchDropLevel(idx);
  currentWaveSettings.pitchDropPos = waveData.GetWavePitchDropPos(idx);
  currentWaveSettings.portamentoRate = 15;
  currentWaveSettings.delayRate = 15;

  currentWaveSettings.lowPassP = waveData.GetWaveLowPass(idx,0);
  currentWaveSettings.lowPassR = waveData.GetWaveLowPass(idx,1);
  currentWaveSettings.lowPassQ = waveData.GetWaveLowPass(idx,2);
  WritePlaySettings(idx);
}

//--------------------------
// Flash から読み出し
void Menu::LoadPreferences() {
  keySense = pref.getInt("KeySense", 50);

  breathSense = pref.getInt("BreathSense", 150);
  breathZero = pref.getInt("BreathZero", 170);
#ifdef HAS_LIPSENSOR
  breathSense = pref.getInt("BreathSense2", 200);
  breathZero = pref.getInt("BreathZero", 50);
#endif

  for (int i = 0; i < WAVE_MAX; i++) {
    if (waveData.GetWaveTable(i) == NULL) {
      break;
    }

    char s[32];
    //sprintf(s, "AccControl%d", i);
    //waveSettings[i].isAccControl = pref.getBool(s, false);
    sprintf(s, "FineTune%d", i);
    waveSettings[i].fineTune = pref.getInt(s, 442);
    sprintf(s, "Transpose%d", i);
    waveSettings[i].transpose = pref.getInt(s, waveData.GetWaveTranspose(i));
    sprintf(s, "AttackSoftness%d", i);
    waveSettings[i].attackSoftness = pref.getInt(s, waveData.GetWaveAttackSoftness(i));
    sprintf(s, "PitchDrop%d", i);
    waveSettings[i].pitchDropLevel = pref.getInt(s, waveData.GetWavePitchDropLevel(i));
    sprintf(s, "PitchDropPos%d", i);
    waveSettings[i].pitchDropPos = pref.getInt(s, waveData.GetWavePitchDropPos(i));
    sprintf(s, "PortamentoRate%d", i);
    waveSettings[i].portamentoRate = pref.getInt(s, waveData.GetWavePortamento(i));
    sprintf(s, "DelayRate%d", i);
    waveSettings[i].delayRate = pref.getInt(s, 15);

    sprintf(s, "LowPassP%d", i);
    waveSettings[i].lowPassP = pref.getInt(s, waveData.GetWaveLowPass(i,0));
    sprintf(s, "LowPassR%d", i);
    waveSettings[i].lowPassR = pref.getInt(s, waveData.GetWaveLowPass(i,1));
    sprintf(s, "LowPassQ%d", i);
    waveSettings[i].lowPassQ = pref.getInt(s, waveData.GetWaveLowPass(i,2));
  }

  waveIndex = pref.getInt("WaveIndex", 0);
  ReadPlaySettings(waveIndex);
}

#ifdef HAS_RTC
//--------------------------
// RTC 時刻変更
void Menu::WriteRtc() {
  if (isRtcChanged == false) {
    return;
  }
  m5::rtc_time_t TimeStruct;
  M5.Rtc.getTime(&TimeStruct);
  TimeStruct.hours = hour;
  TimeStruct.minutes = minute;
  TimeStruct.seconds = 0;
  M5.Rtc.setTime(&TimeStruct);
}

//--------------------------
// RTC から時刻を得る
void Menu::ReadRtc() {
  m5::rtc_time_t TimeStruct;
  M5.Rtc.getTime(&TimeStruct);
  hour = TimeStruct.hours;
  minute = TimeStruct.minutes;
  isRtcChanged = false;
}
#endif

//--------------------------
// 次の音色の波形テーブルに変更
void Menu::SetNextWave() {
    waveIndex++;
    if (waveData.GetWaveTable(waveIndex) == NULL) {
      waveIndex = 0;
    }
    ReadPlaySettings(waveIndex);
}

//--------------------------
bool Menu::SetWaveIndex(int widx) {
    if (waveData.GetWaveTable(widx) != NULL) {
      waveIndex = widx;
      ReadPlaySettings(waveIndex);
      return true;
    }
    return false;
}

//--------------------------
// 次のローパスQ値に変更
bool Menu::SetNextLowPassQ() {
  bool ret = false;
  currentWaveSettings.lowPassQ += 5;
  if (currentWaveSettings.lowPassQ > 30) {
    currentWaveSettings.lowPassQ = 0;
    ret = true;
  }
  return ret;
}

//--------------------------
// メニュー更新処理 (AFUUE2)
bool Menu::Update(uint16_t key, int pressure) {
  M5.update();
#if MAINUNIT == M5STICKC_PLUS
  if (M5.BtnA.pressedFor(100)) {
#elif MAINUNIT == M5ATOM_S3
  if (M5.BtnA.pressedFor(1000)) {
#else
  if (true) {
#endif
    if (isEnabled) {
      cursorPos = 0;
#ifdef HAS_RTC
      WriteRtc();
#endif
#ifdef HAS_DISPLAY
      canvas->deleteSprite();
      M5.Lcd.setBrightness(127);
      M5.Lcd.setRotation(0);
      canvas->setColorDepth(16);
      canvas->createSprite(DISPLAY_HEIGHT, DISPLAY_WIDTH);
#endif
      isEnabled = false;
      ReadPlaySettings(waveIndex);
    }
    else {
      SetNextWave();
    }
    Display();
    while (1) {
      delay(100);
      M5.update();
      if (M5.BtnA.isReleased()) break;

      if (M5.BtnB.pressedFor(10*1000)) {
        // すべてを出荷時状態に戻す
        factoryResetRequested = true;
        return true;
      }
    }
    return true;
  }
#if MAINUNIT == M5STICKC_PLUS
  bool buttonNext = M5.BtnB.pressedFor(100);
#elif MAINUNIT == M5ATOM_S3
  bool buttonNext = M5.BtnA.pressedFor(100);
#else
  bool buttonNext = false;
#endif
  if (buttonNext) {
    if (isEnabled == false) {
#ifdef HAS_RTC
      ReadRtc();
#endif
#ifdef HAS_DISPLAY
      canvas->deleteSprite();
      M5.Lcd.setBrightness(255);
      M5.Lcd.setRotation(3);
      canvas->setColorDepth(16);
      canvas->createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);
#endif
      isEnabled = true;
    }
    else {
      cursorPos = (cursorPos + 1)%((int)m_menus.size());
      if (!m_menus[cursorPos].valueFunc) {
        cursorPos = (cursorPos + 1)%((int)m_menus.size());
      }
    }
    Display();
    while (1) {
      delay(100);
      M5.update();

#if MAINUNIT == M5STICKC_PLUS
      if (M5.BtnB.isReleased()) break;
      if (isEnabled && M5.BtnB.pressedFor(5*1000)) {
#elif MAINUNIT == M5ATOM_S3
      if (M5.BtnA.isReleased()) break;
      if (isEnabled && M5.BtnA.pressedFor(5*1000)) {
#else
      break;
      if (false) {
#endif
        // この波形のパラメータを出荷時状態に戻す
        ResetPlaySettings(waveIndex);
        cursorPos = 0;
        forcePlayTime = FORCEPLAYTIME_LENGTH * 3;
        forcePlayNote = 84;
        Display();
      }
    }
  }
  if (isEnabled) {
    bool octDown = ((key & (1 << 10)) != 0);
    bool octUp = ((key & (1 << 11)) != 0);
    if (octUp) {
      if (m_menus[cursorPos].plusFunc) {
        // 値を増やす
        m_menus[cursorPos].plusFunc();
      }
      Display();
      WritePlaySettings(waveIndex);
    }
    if (octDown) {
      if (m_menus[cursorPos].minusFunc) {
        // 値を減らす
        m_menus[cursorPos].minusFunc();
      }
      Display();
      WritePlaySettings(waveIndex);
    }
    const char* keyName[] = {
      "LowC", "Eb", "D", "E", "F", "LowC#", "G#", "G", "A", "B", "Down", "Up"
    };
    for (int i = 0; i < 12; i++) {
      if ((key & (1 << i))== (1 << i)) {
        currentKey = keyName[i];
        Display();
        break;
      }
    }
    currentPressure = pressure;
  }
  else {
#ifdef HAS_RTC
    m5::rtc_time_t TimeStruct;
    M5.Rtc.getTime(&TimeStruct);
    if ((hour != TimeStruct.hours) || (minute != TimeStruct.minutes)) {
      hour = TimeStruct.hours;
      minute = TimeStruct.minutes;
      DisplayPerform(true);
    }
#endif
  }
  return false;
}

//--------------------------
// メニュー更新処理（AFUUE2R)
bool Menu::Update2R(const KeySystem* pKey) {
  M5.update();

  if (pKey->IsKeyLowCs_Down() && pKey->IsKeyGs_Down()) {
    // Config
#if !defined(HAS_DISPLAY)
    if ((pKey->IsKeyLowC_Down() && pKey->IsKeyEb_Push()) || (pKey->IsKeyEb_Down() && pKey->IsKeyLowC_Push())) {
      // この波形のパラメータを出荷時状態に戻す
      ResetPlaySettings(waveIndex);
      forcePlayTime = FORCEPLAYTIME_LENGTH * 3;
      forcePlayNote = 84;
      return true;
    }
    else
#endif
    if (pKey->IsKeyF_Push()) {
      int pgNum = pgNumHigh * 10 + pgNumLow;
      // Change wave index or MIDI:Send Program Change
#if ENABLE_MIDI
      if (isMidiEnabled) 
      {
        if (pgNum > 0) {
          afuueMidi->NoteOff();
          delay(500);
          afuueMidi->ProgramChange(pgNum-1);
          pgNumHigh = 0;
          pgNumLow = 0;
        }
      }
      else
#endif
      {
        if (pgNum > 0) {
          bool ret = SetWaveIndex(pgNum - 1);
          if (!ret) {
            return false;
          }
        } else {
          SetNextWave();
        }
        forcePlayTime = FORCEPLAYTIME_LENGTH;
        forcePlayNote = currentWaveSettings.transpose + 61 - 1;
        return true;
      }
    }
    else if (pKey->IsKeyD_Push()) {
      // Transpose--
      if (pKey->IsKeyE_Down()) {
        currentWaveSettings.transpose = 0;
      } else {
        currentWaveSettings.transpose--;
        if (currentWaveSettings.transpose < -12) currentWaveSettings.transpose = -12;
      }
#if ENABLE_MIDI
      if (isMidiEnabled) {
        afuueMidi->NoteOn(currentWaveSettings.transpose+61-1, 10);
        delay(static_cast<int>(currentWaveSettings.transpose == 0 ? FORCEPLAYTIME_LENGTH*2 : FORCEPLAYTIME_LENGTH));
        afuueMidi->NoteOff();
      }
      else
#endif
      {
        forcePlayTime = currentWaveSettings.transpose == 0 ? FORCEPLAYTIME_LENGTH*2 : FORCEPLAYTIME_LENGTH;
        forcePlayNote = currentWaveSettings.transpose + 61 - 1;
        return true;
      }
    }
    else if (pKey->IsKeyE_Push()) {
      // Transpose++
      if (pKey->IsKeyD_Down()) {
        currentWaveSettings.transpose = 0;
      } else {
        currentWaveSettings.transpose++;
        if (currentWaveSettings.transpose > 12) currentWaveSettings.transpose = 12;
      }
#if ENABLE_MIDI
      if (isMidiEnabled) {
        afuueMidi->NoteOn(currentWaveSettings.transpose+61-1, 10);
        delay(static_cast<int>(currentWaveSettings.transpose == 0 ? FORCEPLAYTIME_LENGTH*2 : FORCEPLAYTIME_LENGTH));
        afuueMidi->NoteOff();
      }
      else
#endif
      {
        forcePlayTime = currentWaveSettings.transpose == 0 ? FORCEPLAYTIME_LENGTH*2 : FORCEPLAYTIME_LENGTH;
        forcePlayNote = currentWaveSettings.transpose + 61 - 1;
        return true;
      }
    }
#if !defined(HAS_DISPLAY)
    else if (pKey->IsKeyG_Push()) {
      // LowPassQ (Resonance)
      if (!isMidiEnabled) {
        bool ret = SetNextLowPassQ();
        forcePlayTime = ret ? FORCEPLAYTIME_LENGTH * 2 : FORCEPLAYTIME_LENGTH;
        forcePlayNote = currentWaveSettings.transpose + 61 - 1;
        return true;
      }
    }
    else if (pKey->IsKeyA_Push()) {
      // Breath sensitivity
      breathSense -= 50;
      if (breathSense < 100) {
        breathSense = 350;
      }
      forcePlayTime = (breathSense == 200) ? FORCEPLAYTIME_LENGTH * 2 : FORCEPLAYTIME_LENGTH;
      forcePlayNote = currentWaveSettings.transpose + 61 - 1;
      return true;
    }
    else if (pKey->IsKeyDown_Push()) {
      // Fine Tune
      switch (currentWaveSettings.fineTune) {
        default:
          currentWaveSettings.fineTune = 440;
          break;
        case 440:
          currentWaveSettings.fineTune = 442;
          break;
        case 442:
          currentWaveSettings.fineTune = 438;
          break;
      }
      forcePlayTime = (currentWaveSettings.fineTune == 440) ? FORCEPLAYTIME_LENGTH*2 : FORCEPLAYTIME_LENGTH;
      forcePlayNote = currentWaveSettings.transpose + 61 - 1;
      return true;
    }
    else if (pKey->IsKeyUp_Push()) {
      // Delay Rate
      switch (currentWaveSettings.delayRate) {
        default:
          currentWaveSettings.delayRate = 15;
          break;
        case 15:
          currentWaveSettings.delayRate = 30;
          break;
        case 30:
          currentWaveSettings.delayRate = 0;
          break;
      }
      forcePlayTime = (currentWaveSettings.delayRate == 15) ? FORCEPLAYTIME_LENGTH*2 : FORCEPLAYTIME_LENGTH;
      forcePlayNote = currentWaveSettings.transpose + 61 - 1;
      return true;
    }
#if ENABLE_MIDI
    else if (pKey->IsKeyB_Push()) {
      if (isMidiEnabled) {
        // Change MIDI Breath Control Mode
        afuueMidi->ChangeBreathControlMode();
      }
    }
#endif
#endif //!defined(HAS_DISPLAY)
    else if (pKey->IsKeyLowC_Push()) {
      pgNumLow  = (pgNumLow + 1) % 10;
    }
    else if (pKey->IsKeyEb_Push()) {
      pgNumHigh  = (pgNumHigh + 1) % 10;
    }
  }
  else {
    pgNumHigh = 0;
    pgNumLow = 0;
  }
#if !defined(HAS_DISPLAY)
  if (pKey->IsFuncBtn_Push()) {
    ctrlMode = (ctrlMode + 1) % 4; // 0:normal, 1:lip-bend, 2:MIDI-normal, 3:MIDI-lip-bend
    isLipSensorEnabled = (ctrlMode % 2);
#if ENABLE_MIDI
    if (isMidiEnabled) {
      afuueMidi->NoteOff();
    }
    isMidiEnabled = (ctrlMode >= 2) || isUSBMidiMounted;
#endif
    forcePlayNote = currentWaveSettings.transpose + 61 - 1;
    forcePlayTime = FORCEPLAYTIME_LENGTH;
    return true;
  }
  if (pKey->IsFuncBtn_Down()) {
    funcDownCount++;
    if (funcDownCount >= 20*5) {
      factoryResetRequested = true;
      return true;
    }
  }
  else {
    funcDownCount = 0;
  }
#endif //!defined(HAS_DISPLAY)
  return false;
}

//--------------------------
// x, y にバッテリー情報を描画
void Menu::DrawBattery(int x, int y) {
#if defined(HAS_DISPLAY) && defined(HAS_INTERNALBATTERY)
  bat_per = M5.Power.getBatteryLevel();
  if(bat_per > 100.0f){
      bat_per = 100.0f;
  }
  uint16_t color = WHITE;
  if (bat_per < 25.0f) {
    color = RED;
  }
  canvas->fillRect(x - 4, y + 1, 3, 5, color);
  canvas->fillRect(x - 2, y - 4, 24, 15, color);
  canvas->fillRect(x, y - 2, 20, 11, BLACK);

  canvas->fillRect(x, y - 2, 20, 11, BLACK);
  if (bat_per >= 75.0f) {
    canvas->fillRect(x + 2, y, 4, 7, WHITE);
  }
  if (bat_per >= 50.0f) {
    canvas->fillRect(x + 8, y, 4, 7, WHITE);
  }
  if (bat_per >= 25.0f) {
    canvas->fillRect(x + 14, y, 4, 7, WHITE);
  }
#endif
}

//--------------------------
// sx, sy の位置に str を描画（英数字のみです）
void Menu::DrawString(const char* str, int sx, int sy) {
#ifdef HAS_DISPLAY
  const char* ps = str;
  int x = sx;
  int y = sy;
  while (*ps != 0x00) {
    int c = (int)*ps;
    if (c == 0x20) {
      x += 6*2;
      ps++;
      continue;
    }
    else if (c == 0x0a) {
      x = sx;
      y += 8*2;
      ps++;
      continue;
    }
    const unsigned short* p = FontTable[c - (int)'?'];
    if ((c >= 0x20) && (c <= 0x7F)) {
      p = FontTable[c - 0x20];
    }
    canvas->pushImage(x, y, 6*2, 8*2, p);
    x += 6*2;
    ps++;
  }
#endif
}

//--------------------------
// 線をひく
void Menu::DisplayLine(int line, bool selected, const std::string& title, const std::string& value) {
#ifdef HAS_DISPLAY
  if ((line < 0)||(line > 3)) return;
  
  std::string s = "";
  if (DISPLAY_WIDTH == 128) {
    // M5AtomS3
    if (!selected) {
      return;
    }
    s += "\n";
    s += title;
    s += "\n";
    s += "\n";
    if (value != "") {
      s += " : ";
      s += value;
    }
    DrawString(s.c_str(), 10, 10 + 20);
  }
  else {
    // M5StickC Plus
    s += "  ";
    if (selected) {
      s = "> ";
    }
    s += title;
    while (s.length() < 11) {
      s += " ";
    }
    if (value != "") {
      s += " : ";
      s += value;
    }
    DrawString(s.c_str(), 10, 10 + 20*(line+2));
  }
#endif
}

//--------------------------
// 時刻から文字へ
std::string Menu::TimeToStr() const {
  char s[16];
  sprintf(s, "%02d:%02d", hour, minute);
  return std::string(s);
}

//--------------------------
// トランスポーズの文字列を取得
std::string Menu::TransposeToStr() const {
  const char* TransposeTable[25] = {
    "-C", "-Db", "-D", "-Eb", "-E", "-F", "-Gb", "-G", "-Ab", "-A", "-Bb", "-B",
    "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B", "+C"
  };
  if ((currentWaveSettings.transpose < -12) && (currentWaveSettings.transpose > 12)) {
    return "??";
  }
  return std::string(TransposeTable[currentWaveSettings.transpose+12]);
}

//--------------------------
// ノート番号の文字列を取得
std::string Menu::NoteNumberToStr() const {
  const char* NoteName[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B",
  };
  int n = testNote%12;
  int o = (testNote/12 - 1); //国際式 (ヤマハ式はさらに-1)
  return NoteName[n] + std::to_string(o);
}

//--------------------------
// メニュー用の描画
void Menu::DisplayMenu() {
#ifdef HAS_DISPLAY
  if (DISPLAY_WIDTH == 128) {
    // M5AtomS3
    DrawString("[Settings]", 10, 10);
  } else {
    // M5StickC Plus
    DrawString("[AFUUE Settings]", 10, 10);
  }
  int viewPos = 0;
  int i = 0;
  if (cursorPos >= 4) {
    viewPos = 3 - cursorPos;
  }

  for (const auto& m : m_menus) {
    if (m.valueFunc) {
      DisplayLine(viewPos, (cursorPos == i), m.name, m.valueFunc());
    }
    else {
      DisplayLine(viewPos, false, "------", "");
    }
    viewPos++;
    i++;
  }
#endif
}

//--------------------------
// 演奏時用の描画
void Menu::DisplayPerform(bool onlyRefreshTime) {
#ifdef HAS_DISPLAY
  if (onlyRefreshTime == false) {
    if (DISPLAY_HEIGHT > 128) {
      canvas->pushImage(0, 240-115, 120, 115, bitmap_logo);
    }
    DrawString(waveData.GetWaveName(waveIndex), 10, 70);
    DrawString(TransposeToStr().c_str(), 10, 100);
  }
  {
#ifdef HAS_RTC
    m5::rtc_time_t TimeStruct;
    M5.Rtc.getTime(&TimeStruct);
    if (hour == TimeStruct.hours) {
      hour = TimeStruct.hours;
    }
    if (minute == TimeStruct.minutes) {
      minute = TimeStruct.minutes;
    }
    char s[32];
    sprintf(s, "%02d:%02d", hour, minute);
    DrawString(s, 10, 10);
#endif
    DrawBattery(103, 12);
  }
#endif
}

//--------------------------
// 描画窓口
void Menu::Display() {
#ifdef HAS_DISPLAY
  canvas->fillScreen(TFT_BLACK);

  if (isEnabled) {
    DisplayMenu();
  }
  else {
#ifdef HAS_RTC
    ReadRtc();
#endif
    DisplayPerform();
  }

  canvas->pushSprite(0, 0);
#endif
}
