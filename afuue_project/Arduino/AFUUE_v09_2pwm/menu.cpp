#include "afuue_common.h"
#include "logo.h"
#include "ascii_fonts.h"
#include "wavedata.h"
#include "menu.h"

static float bat_per = 0.0f;

enum {
  MENUINDEX_TRANSPOSE,
  MENUINDEX_LOWPASSP,
  MENUINDEX_LOWPASSR,
  MENUINDEX_LOWPASSQ,
  MENUINDEX_ATTACK,
  MENUINDEX_FINETUNE,
  MENUINDEX_PORTAMENTO,
  MENUINDEX_DELAY,
  MENUINDEX_KEYSENSE,
  MENUINDEX_BREATHSENSE,
  MENUINDEX_BREATHZERO,
  MENUINDEX_CLOCK,
  MENUINDEX_PUSHEDKEY,
  MENUINDEX_PRESSURE,
  MENUINDEX_SPEAKER,
  MENUINDEX_MAX,
};

//--------------------------
Menu::Menu(M5Canvas* _canvas) {
  canvas = _canvas;
}

//--------------------------
void Menu::Initialize(Preferences pref) {
  //pref.clear(); // CLEAR ALL FLASH MEMORY
  int ver = pref.getInt("AfuueVer", -1);
  if (ver != AFUUE_VER) {
    pref.clear(); // CLEAR ALL FLASH MEMORY
    pref.putInt("AfuueVer", AFUUE_VER);
  }
  LoadPreferences(pref);

  canvas->setColorDepth(16);
  canvas->createSprite(135, 240);
}

//--------------------------
void Menu::SavePreferences(Preferences pref) {
#ifdef _M5STICKC_H_
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
#ifdef ENABLE_MCP3425
  pref.putInt("BreathSense", breathSense);
#else
  pref.putInt("BreathSense2", breathSense);
#endif
  pref.putInt("BreathZero", breathZero);
  pref.putInt("WaveIndex", waveIndex);
}

//--------------------------
void Menu::ReadPlaySettings(int widx) {
    //isAccControl = waveSettings[widx].isAccControl;
    fineTune = waveSettings[widx].fineTune;
    transpose = waveSettings[widx].transpose;
    attackSoftness = waveSettings[widx].attackSoftness;
    portamentoRate = waveSettings[widx].portamentoRate;
    delayRate = waveSettings[widx].delayRate;

    lowPassP = waveSettings[widx].lowPassP;
    lowPassR = waveSettings[widx].lowPassR;
    lowPassQ = waveSettings[widx].lowPassQ;
}

//--------------------------
void Menu::WritePlaySettings(int widx) {
    //waveSettings[widx].isAccControl = isAccControl;
    waveSettings[widx].fineTune = fineTune;
    waveSettings[widx].transpose = transpose;
    waveSettings[widx].attackSoftness = attackSoftness;
    waveSettings[widx].portamentoRate = portamentoRate;
    waveSettings[widx].delayRate = delayRate;

    waveSettings[widx].lowPassP = lowPassP;
    waveSettings[widx].lowPassR = lowPassR;
    waveSettings[widx].lowPassQ = lowPassQ;
}

//--------------------------
void Menu::ResetPlaySettings(int widx) {
  int idx = widx;
  if (idx < 0) {
    idx = waveIndex;
  }
  // isAccControl = false;
  fineTune = 442;
  transpose = waveData.GetWaveTranspose(idx);
  portamentoRate = 15;
  delayRate = 15;

  lowPassP = 5;
  lowPassR = 5;
  lowPassQ = waveData.GetWaveLowPassQ(idx);
  WritePlaySettings(idx);
}

//--------------------------
void Menu::LoadPreferences(Preferences pref) {
  keySense = pref.getInt("KeySense", 50);
#ifdef ENABLE_MCP3425
  breathSense = pref.getInt("BreathSense", 150);
  breathZero = pref.getInt("BreathZero", 110);
#else
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
    sprintf(s, "PortamentoRate%d", i);
    waveSettings[i].portamentoRate = pref.getInt(s, waveData.GetWavePortamento(i));
    sprintf(s, "DelayRate%d", i);
    waveSettings[i].delayRate = pref.getInt(s, 15);

    sprintf(s, "LowPassP%d", i);
    waveSettings[i].lowPassP = pref.getInt(s, 5);
    sprintf(s, "LowPassR%d", i);
    waveSettings[i].lowPassR = pref.getInt(s, 5);
    sprintf(s, "LowPassQ%d", i);
    waveSettings[i].lowPassQ = pref.getInt(s, waveData.GetWaveLowPassQ(i));
  }

  waveIndex = pref.getInt("WaveIndex", 0);
  ReadPlaySettings(waveIndex);
}

#ifdef ENABLE_RTC
//--------------------------
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
void Menu::ReadRtc() {
  m5::rtc_time_t TimeStruct;
  M5.Rtc.getTime(&TimeStruct);
  hour = TimeStruct.hours;
  minute = TimeStruct.minutes;
  isRtcChanged = false;
}
#endif

//--------------------------
void Menu::SetNextWave() {
    waveIndex++;
    if (waveData.GetWaveTable(waveIndex) == NULL) {
      waveIndex = 0;
    }
    ReadPlaySettings(waveIndex);
}

//--------------------------
bool Menu::SetNextLowPassQ() {
  bool ret = false;
  lowPassQ += 5;
  if (lowPassQ > 30) {
    lowPassQ = 0;
    ret = true;
  }
  WritePlaySettings(waveIndex);
  return ret;
}

//--------------------------
bool Menu::Update(Preferences pref, uint16_t key, int pressure) {
#ifdef _M5STICKC_H_
  M5.update();
  if (M5.BtnA.pressedFor(100)) {
    if (isEnabled) {
      cursorPos = 0;
#ifdef ENABLE_RTC
      WriteRtc();
#endif
      canvas->deleteSprite();
      M5.Lcd.setBrightness(127);
      M5.Lcd.setRotation(0);
      canvas->setColorDepth(16);
      canvas->createSprite(135, 240);
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

      if (!isEnabled && M5.BtnB.pressedFor(10*1000)) {
        // すべてを出荷時状態に戻す (!isEnabled なのは↑で false にしているから)
        factoryResetRequested = true;
        return true;
      }
    }
    return true;
  }
  if (M5.BtnB.pressedFor(100)) {
    if (isEnabled == false) {
#ifdef ENABLE_RTC
      ReadRtc();
#endif
      canvas->deleteSprite();
      M5.Lcd.setBrightness(255);
      M5.Lcd.setRotation(3);
      canvas->setColorDepth(16);
      canvas->createSprite(240, 135);
      isEnabled = true;
    }
    else {
      cursorPos = (cursorPos + 1)%((int)MENUINDEX_MAX);
    }
    Display();
    while (1) {
      delay(100);
      M5.update();
      if (M5.BtnB.isReleased()) break;

      if (isEnabled && M5.BtnB.pressedFor(5*1000)) {
        // この波形のパラメータを出荷時状態に戻す
        ResetPlaySettings(waveIndex);
        cursorPos = 0;
        forcePlayNote = 84;
        Display();
      }
    }
  }
  if (isEnabled) {
    bool octDown = ((key & (1 << 10)) != 0);
    bool octUp = ((key & (1 << 11)) != 0);
    if (octDown) {
      switch (cursorPos) {
        case MENUINDEX_TRANSPOSE:
          transpose--;
          if (transpose < -12) transpose = -12;
          break;
        case MENUINDEX_LOWPASSP:
          lowPassP--;
          if (lowPassP < 1) lowPassP = 1;
          break;
        case MENUINDEX_LOWPASSR:
          lowPassR--;
          if (lowPassR < 1) lowPassR = 1;
          break;
        case MENUINDEX_LOWPASSQ:
          lowPassQ--;
          if (lowPassQ < 0) lowPassQ = 0;
          break;
        case MENUINDEX_ATTACK:
          attackSoftness--;
          if (attackSoftness < 0) attackSoftness = 0;
          break;
        case MENUINDEX_FINETUNE:
          fineTune--;
          if (fineTune < 400) fineTune = 400;
          break;
        case MENUINDEX_PORTAMENTO:
          portamentoRate--;
          if (portamentoRate < 0) portamentoRate = 0;
          break;
        case MENUINDEX_DELAY:
          delayRate--;
          if (delayRate < 0) delayRate = 0;
          break;
        case MENUINDEX_KEYSENSE:
          keySense -= 5;
          if (keySense < 0) keySense = 0;
          break;
        case MENUINDEX_BREATHSENSE:
#ifdef ENABLE_MCP3425
          breathSense -= 50;
          if (breathSense < 0) breathSense = 0;
#else
          breathSense -= 10;
          if (breathSense < 100) breathSense = 100;
#endif
          break;
        case MENUINDEX_BREATHZERO:
          breathZero -= 10;
          if (breathZero < 0) breathZero = 0;
          break;
        case MENUINDEX_CLOCK:
          minute = (minute + 59) % 60;
          if (minute == 59) {
            hour = (hour + 23)%24;
          }
          isRtcChanged = true;
          break;
        case MENUINDEX_SPEAKER:
          testNote--;
          if (testNote < 12) testNote = 12;
          forcePlayNote = testNote;
          break;
      }
      if (cursorPos != MENUINDEX_PUSHEDKEY) {
        Display();
        WritePlaySettings(waveIndex);
      }
    }
    if (octUp) {
      switch (cursorPos) {
        case MENUINDEX_TRANSPOSE:
          transpose++;
          if (transpose > 12) transpose = 12;
          break;
        case MENUINDEX_LOWPASSP:
          lowPassP++;
          if (lowPassP > 10) lowPassP = 10;
          break;
        case MENUINDEX_LOWPASSR:
          lowPassR++;
          if (lowPassR > 30) lowPassR = 30;
          break;
        case MENUINDEX_LOWPASSQ:
          lowPassQ++;
          if (lowPassQ > 50) lowPassQ = 50;
          break;
        case MENUINDEX_ATTACK:
          attackSoftness++;
          if (attackSoftness > 99) attackSoftness = 99;
          break;
        case MENUINDEX_FINETUNE:
          fineTune++;
          if (fineTune > 480) fineTune = 480;
          break;
        case MENUINDEX_PORTAMENTO:
          portamentoRate++;
          if (portamentoRate > 99) portamentoRate = 99;
          break;
        case MENUINDEX_DELAY:
          delayRate++;
          if (delayRate > 99) delayRate = 99;
          break;
        case MENUINDEX_KEYSENSE:
          keySense += 5;
          if (keySense > 100) keySense = 100;
          break;
        case MENUINDEX_BREATHSENSE:
#ifdef ENABLE_MCP3425
          breathSense += 50;
          if (breathSense > 500) breathSense = 500;
#else
          breathSense += 10;
          if (breathSense > 500) breathSense = 500;
#endif
          break;
        case MENUINDEX_BREATHZERO:
          breathZero += 10;
          if (breathZero > 300) breathZero = 300;
          break;
        case MENUINDEX_CLOCK:
          minute = (minute + 1)%60;
          if (minute == 0) {
            hour = (hour + 1)%24;
          }
          isRtcChanged = true;
          break;
        case MENUINDEX_SPEAKER:
          testNote++;
          if (testNote > 96) testNote = 96;
          forcePlayNote = testNote;
          break;
      }
      if (cursorPos != MENUINDEX_PUSHEDKEY) {
        Display();
        WritePlaySettings(waveIndex);
      }
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
#ifdef ENABLE_RTC
    m5::rtc_time_t TimeStruct;
    M5.Rtc.getTime(&TimeStruct);
    if ((hour != TimeStruct.hours) || (minute != TimeStruct.minutes)) {
      hour = TimeStruct.hours;
      minute = TimeStruct.minutes;
      DisplayPerform(true);
    }
#endif
  }
#endif // _M5STICKC_H_
  return false;
}

//--------------------------
void Menu::DrawBattery(int x, int y) {
#ifdef _M5STICKC_H_
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
void Menu::DrawString(const char* str, int sx, int sy) {
#ifdef _M5STICKC_H_
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
void Menu::DisplayLine(int line, bool selected, const std::string& title, const std::string& value) {
  if ((line < 0)||(line > 3)) return;
  
  std::string s = "  ";
  if (selected) {
    s = "> ";
  }
  s += title;
  while (s.length() < 11) {
    s += " ";
  }
  s += ": ";
  s += value;
  DrawString(s.c_str(), 10, 10 + 20*(line+2));
}

//--------------------------
std::string Menu::TimeToStr() {
  char s[16];
  sprintf(s, "%02d:%02d", hour, minute);
  return std::string(s);
}

//--------------------------
std::string Menu::TransposeToStr() {
  const char* TransposeTable[25] = {
    "-C", "-Db", "-D", "-Eb", "-E", "-F", "-Gb", "-G", "-Ab", "-A", "-Bb", "-B",
    "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B", "+C"
  };
  if ((transpose < -12) && (transpose > 12)) {
    return "??";
  }
  return std::string(TransposeTable[transpose+12]);
}

//--------------------------
std::string Menu::NoteNumberToStr() {
  const char* NoteName[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B",
  };
  int n = testNote%12;
  int o = (testNote/12 - 1); //国際式 (ヤマハ式はさらに-1)
  return NoteName[n] + std::to_string(o);
}

//--------------------------
void Menu::DisplayMenu() {
#ifdef _M5STICKC_H_
  //canvas->fillScreen(BLACK);
  DrawString("[AFUUE Settings]", 10, 10);
  int viewPos = 0;
  int i = 0;
  if (cursorPos >= 4) {
    viewPos = 3 - cursorPos;
  }
  DisplayLine(viewPos, (cursorPos == i), "Transpose", TransposeToStr()); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "LP_Pos", std::to_string(lowPassP)); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "LP_Rate", std::to_string(lowPassR)); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "LP_Power", std::to_string(lowPassQ)); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "Attack", std::to_string(attackSoftness)); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "FineTune", std::to_string(fineTune)); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "Portamnto", std::to_string(portamentoRate)); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "Delay", std::to_string(delayRate)); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "KeySense", std::to_string(keySense)); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "BrthSense", std::to_string(breathSense)); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "BrthZero", std::to_string(breathZero)); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "Clock", TimeToStr()); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "KeyTest", currentKey); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "BrthTest", std::to_string(currentPressure)); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "SpkTest", NoteNumberToStr()); viewPos++; i++;
#endif
}

//--------------------------
void Menu::DisplayPerform(bool onlyRefreshTime) {
#ifdef _M5STICKC_H_
  if (onlyRefreshTime == false) {
    //canvas->fillScreen(BLACK);
    canvas->pushImage(0, 240-115, 120, 115, bitmap_logo);
    DrawString(waveData.GetWaveName(waveIndex), 10, 70);
    DrawString(TransposeToStr().c_str(), 10, 100);
  }
  {
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
    DrawBattery(103, 12);
  }
#endif
}

//--------------------------
void Menu::Display() {
#ifdef _M5STICKC_H_
  canvas->fillScreen(TFT_BLACK);

  if (isEnabled) {
    DisplayMenu();
  }
  else {
    ReadRtc();
    DisplayPerform();
  }

  canvas->pushSprite(0, 0);
#endif
}
