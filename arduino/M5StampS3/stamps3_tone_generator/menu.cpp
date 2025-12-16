#include "wavedata.h"
#include "menu.h"
#include "ssd1306.h"
#include "ascii_fonts.h"

//--------------------------
Menu::Menu() {
}

//--------------------------
// 初期化
void Menu::Initialize() {
  ssd1306_init();
  BeginPreferences();
  {
    int ver = pref.getInt("AfuueVer", -1);
    if (ver != 800) {
      pref.clear(); // CLEAR ALL FLASH MEMORY
      pref.putInt("AfuueVer", 800);
    }
    LoadPreferences();
  } EndPreferences();

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
}

//-------------------------------------
// Flash書き込み開始
void Menu::BeginPreferences() {
  usePreferencesDepth++;
  if (usePreferencesDepth > 1) return;
  pref.begin("Afuue/Settings", false);
}

//--------------------------
// Flash書き込み終了
void Menu::EndPreferences() {
  usePreferencesDepth--;
  if (usePreferencesDepth == 0) {
    pref.end();  
  }
  if (usePreferencesDepth < 0) usePreferencesDepth = 0;
}

//--------------------------
// Flashに保存
void Menu::SavePreferences() {
  for (int i = 0; i < WAVE_MAX; i++) {
    if (waveData.GetWaveTable(i) == NULL) {
      break;
    }

    char s[32];
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
  for (int i = 0; i < WAVE_MAX; i++) {
    if (waveData.GetWaveTable(i) == NULL) {
      break;
    }

    char s[32];
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
bool Menu::Update(bool shortPress, bool longPress, bool buttonNext, bool buttonPrev) {
  if (longPress) {
    isEnabled = !isEnabled;
    return false;
    
    // すべてを出荷時状態に戻す
    //factoryResetRequested = true;
    //return true;
  }
  else if (shortPress) {
    if (isEnabled) {
      cursorPos = (cursorPos + 1)%((int)m_menus.size());
      if (!m_menus[cursorPos].valueFunc) {
        cursorPos = (cursorPos + 1)%((int)m_menus.size());
      }
    }
    else {
      SetNextWave();
    }
    return true;
  }
  if (isEnabled) {
    if (buttonNext) {
      if (m_menus[cursorPos].plusFunc) {
        // 値を増やす
        m_menus[cursorPos].plusFunc();
      }
      WritePlaySettings(waveIndex);
    }
    if (buttonPrev) {
      if (m_menus[cursorPos].minusFunc) {
        // 値を減らす
        m_menus[cursorPos].minusFunc();
      }
      WritePlaySettings(waveIndex);
    }
  }
  return false;
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
void Menu::DisplayLine(const std::string& title, const std::string& value) {
  std::string s = "";
    s += title;
    s += " ";
    if (value != "") {
      s += "\n : ";
      s += value;
    }
    DrawString(s.c_str(), 0, 0);
}

//--------------------------
// メニュー用の描画
void Menu::DisplayMenu() {
  int viewPos = 0;
  int i = 0;
  for (const auto& m : m_menus) {
    if (m.valueFunc) {
      if (cursorPos == i) {
        DisplayLine(m.name, m.valueFunc());
      }
    }
    i++;
  }
}

//--------------------------
void Menu::DisplayMessage(const char* s) {
    clearBuffer();
    DrawString(s, 2, 0);
    ssd1306_update();
}

//--------------------------
// 描画窓口
void Menu::Display(float currentVol, float currentBend) {
    clearBuffer();
    if (isEnabled) {
      DisplayMenu();
    }
    else 
    {
      fillRect(2, 32+2, (int)(120.0f*currentVol), 14, true);
      fillRect(2, 32+25, 120, 2, true);
      int x = 64 + (int)(60.0f * currentBend);
      fillRect(x-5, 32+20, 5, 14, true);
      if (msg != "") {
        DrawString(msg.c_str(), 2, 0);
      }
      else {
        char s[32];
        sprintf(s, "%1.1f, %1.1f", currentVol, currentBend);
        DrawString(s, 2, 0);
      }
    }
    ssd1306_update();

#if 0
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
