#include "afuue_common.h"
#include "logo.h"
#include "ascii_fonts.h"
#include "wavedata.h"
#include "menu.h"

const float bat_percent_max_vol = 4.0f;     // バッテリー残量の最大電圧
const float bat_percent_min_vol = 3.3f;     // バッテリー残量の最小電圧
static float bat_per_inclination = 0.0f;        // バッテリー残量のパーセンテージ式の傾き
static float bat_per_intercept   = 0.0f;        // バッテリー残量のパーセンテージ式の切片
static float bat_per             = 0.0f;        // バッテリー残量のパーセンテージ
static float bat_vol             = 0.0f;        // バッテリー電圧

enum {
  MENUINDEX_TRANSPOSE,
  MENUINDEX_ACCCONTROL,
  MENUINDEX_FINETUNE,
  MENUINDEX_PORTAMENTO,
  MENUINDEX_DELAY,
  MENUINDEX_KEYSENSE,
  MENUINDEX_BREATHSENSE,
  MENUINDEX_CLOCK,
  MENUINDEX_PUSHEDKEY,
  MENUINDEX_PRESSURE,
  MENUINDEX_SPEAKER,
  MENUINDEX_MAX,
};

//--------------------------
Menu::Menu() {
  bat_per_inclination = 100.0f/(bat_percent_max_vol-bat_percent_min_vol);
  bat_per_intercept = -bat_percent_min_vol * bat_per_inclination;
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
}

//--------------------------
void Menu::SavePreferences(Preferences pref) {
  pref.putInt("KeySense", keySense);
  pref.putInt("BreathSense", breathSense);
  
  for (int i = 0; i < WAVE_MAX; i++) {
    if (waveData.GetWaveTable(i) == NULL) {
      break;
    }

    char s[32];
    sprintf(s, "AccControl%d", i);
    pref.putBool(s, waveSettings[i].isAccControl);
    sprintf(s, "FineTune%d", i);
    pref.putInt(s, waveSettings[i].fineTune);
    sprintf(s, "Transpose%d", i);
    pref.putInt(s, waveSettings[i].transpose);
    sprintf(s, "PortamentoRate%d", i);
    pref.putInt(s, waveSettings[i].portamentoRate);
    sprintf(s, "DelayRate%d", i);
    pref.putInt(s, waveSettings[i].delayRate);
  }

  pref.putInt("WaveIndex", waveIndex);
}

//--------------------------
void Menu::ReadPlaySettings(int widx) {
    isAccControl = waveSettings[widx].isAccControl;
    fineTune = waveSettings[widx].fineTune;
    transpose = waveSettings[widx].transpose;
    portamentoRate = waveSettings[widx].portamentoRate;
    delayRate = waveSettings[widx].delayRate;
}

//--------------------------
void Menu::WritePlaySettings(int widx) {
    waveSettings[widx].isAccControl = isAccControl;
    waveSettings[widx].fineTune = fineTune;
    waveSettings[widx].transpose = transpose;
    waveSettings[widx].portamentoRate = portamentoRate;
    waveSettings[widx].delayRate = delayRate;
}

//--------------------------
void Menu::LoadPreferences(Preferences pref) {
  keySense = pref.getInt("KeySense", 45);
  breathSense = pref.getInt("BreathSense", 300);
  
  for (int i = 0; i < WAVE_MAX; i++) {
    if (waveData.GetWaveTable(i) == NULL) {
      break;
    }

    char s[32];
    sprintf(s, "AccControl%d", i);
    waveSettings[i].isAccControl = pref.getBool(s, false);
    sprintf(s, "FineTune%d", i);
    waveSettings[i].fineTune = pref.getInt(s, 442);
    sprintf(s, "Transpose%d", i);
    waveSettings[i].transpose = pref.getInt(s, 0);
    sprintf(s, "PortamentoRate%d", i);
    waveSettings[i].portamentoRate = pref.getInt(s, 15);
    sprintf(s, "DelayRate%d", i);
    waveSettings[i].delayRate = pref.getInt(s, 15);
  }

  waveIndex = pref.getInt("WaveIndex", 0);
  ReadPlaySettings(waveIndex);
}

//--------------------------
void Menu::WriteRtc() {
  if (isRtcChanged == false) {
    return;
  }
  RTC_TimeTypeDef TimeStruct;
  M5.Rtc.GetTime(&TimeStruct);
  TimeStruct.Hours = hour;
  TimeStruct.Minutes = minute;
  TimeStruct.Seconds = 0;
  M5.Rtc.SetTime(&TimeStruct);
}

//--------------------------
void Menu::ReadRtc() {
  RTC_TimeTypeDef TimeStruct;
  M5.Rtc.GetTime(&TimeStruct);
  hour = TimeStruct.Hours;
  minute = TimeStruct.Minutes;
  isRtcChanged = false;
}

//--------------------------
bool Menu::Update(uint16_t key, int pressure) {
#ifdef _M5STICKC_H_
  M5.update();
  if (M5.BtnA.pressedFor(100)) {
    if (isEnabled) {
      cursorPos = 0;
      WriteRtc();
      M5.Axp.ScreenBreath(8);
      M5.Lcd.setRotation(0);
      isEnabled = false;
    }
    else {
      waveIndex++;
    }
    if (waveData.GetWaveTable(waveIndex) == NULL) {
      waveIndex = 0;
    }
    ReadPlaySettings(waveIndex);
    Display();
    while (1) {
      delay(100);
      M5.update();
      if (M5.BtnA.isReleased()) break;
    }
    return true;
  }
  if (M5.BtnB.pressedFor(100)) {
    if (isEnabled == false) {
      ReadRtc();
      M5.Axp.ScreenBreath(15);
      M5.Lcd.setRotation(3);
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
    }
  }
#endif
  if (isEnabled) {
    bool octDown = ((key & (1 << 10)) != 0);
    bool octUp = ((key & (1 << 11)) != 0);
    if (octDown) {
      switch (cursorPos) {
        case MENUINDEX_TRANSPOSE:
          transpose--;
          if (transpose < -12) transpose = -12;
          break;
        case MENUINDEX_ACCCONTROL:
          isAccControl = false;
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
          breathSense -= 10;
          if (breathSense < 100) breathSense = 100;
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
        case MENUINDEX_ACCCONTROL:
          isAccControl = true;
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
          breathSense += 10;
          if (breathSense > 500) breathSense = 500;
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
    RTC_TimeTypeDef TimeStruct;
    M5.Rtc.GetTime(&TimeStruct);
    if ((hour != TimeStruct.Hours) || (minute != TimeStruct.Minutes)) {
      hour = TimeStruct.Hours;
      minute = TimeStruct.Minutes;
      DisplayPerform(true);
    }
  }
  return false;
}

//--------------------------
void Menu::DrawBattery(int x, int y) {
#ifdef _M5STICKC_H_
  bat_vol = M5.Axp.GetVbatData() * 1.1 / 1000;   // V
  bat_per = bat_per_inclination * bat_vol + bat_per_intercept;    // %
  if(bat_per > 100.0){
      bat_per = 100.0;
  }
  uint16_t color = WHITE;
  if (bat_per < 25) {
    color = RED;
  }
  M5.Lcd.fillRect(x - 4, y + 1, 3, 5, color);
  M5.Lcd.fillRect(x - 2, y - 4, 24, 15, color);
  M5.Lcd.fillRect(x, y - 2, 20, 11, BLACK);

  M5.Lcd.fillRect(x, y - 2, 20, 11, BLACK);
  if (bat_per >= 75) {
    M5.Lcd.fillRect(x + 2, y, 4, 7, WHITE);
  }
  if (bat_per >= 50) {
    M5.Lcd.fillRect(x + 8, y, 4, 7, WHITE);
  }
  if (bat_per >= 25) {
    M5.Lcd.fillRect(x + 14, y, 4, 7, WHITE);
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
    M5.Lcd.drawBitmap(x, y, 6*2, 8*2, p);
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
  int o = (testNote/12 - 1);
  return NoteName[n] + std::to_string(o);
}

//--------------------------
void Menu::DisplayMenu() {
#ifdef _M5STICKC_H_
  M5.Lcd.fillScreen(BLACK);
  DrawString("[AFUUE Settings]", 10, 10);
  int viewPos = 0;
  int i = 0;
  if (cursorPos >= 4) {
    viewPos = 3 - cursorPos;
  }
  DisplayLine(viewPos, (cursorPos == i), "Transpose", TransposeToStr()); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "AccCtrl", std::to_string(isAccControl)); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "FineTune", std::to_string(fineTune)); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "Portamnto", std::to_string(portamentoRate)); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "Delay", std::to_string(delayRate)); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "KeySense", std::to_string(keySense)); viewPos++; i++;
  DisplayLine(viewPos, (cursorPos == i), "BrthSense", std::to_string(breathSense)); viewPos++; i++;
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
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.drawBitmap(0, 240-115, 120, 115, bitmap_logo);
    DrawString(waveData.GetWaveName(waveIndex), 10, 70);
    DrawString(TransposeToStr().c_str(), 10, 100);
  }
  {
    RTC_TimeTypeDef TimeStruct;
    M5.Rtc.GetTime(&TimeStruct);
    if (hour == TimeStruct.Hours) {
      hour = TimeStruct.Hours;
    }
    if (minute == TimeStruct.Minutes) {
      minute = TimeStruct.Minutes;
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
  if (isEnabled) {
    DisplayMenu();
  }
  else {
    DisplayPerform();
  }
}
