#include "Arduino.h"
#include <WiFi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "FontTable.h"
#include "SharpLogo.h"
#include "MCP23017.h"
#include "eval.h"

#include "Music.h"

#define PIN_MOSI (26)
#define PIN_MISO (36)
#define PIN_SCK (18)
#define PIN_DAC (25)
#define PIN_SDA (21)
#define PIN_SCL (22)
#define PIN_CS (19)
#define PIN_LED (27)

#define PIN_SPI_SCL (PIN_SCL)
#define PIN_SPI_SDA (PIN_SDA)
#define PIN_SPI_RST (PIN_DAC)
#define PIN_SPI_CS (-1) //19
#define PIN_SPI_DC (PIN_CS)

#define SCK (PIN_SPI_SCL)
#define MISO (PIN_SPI_SDA)
#define MOSI (PIN_MISO)
#include <U8g2lib.h>
#define USE_FASTLED (0)
#if USE_FASTLED
#include <FastLED.h>
#define NUM_LEDS 1
CRGB leds[NUM_LEDS];
#endif

//---------------------------------
//https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#sh1122-256x64
#if 0
U8G2_SH1122_256X64_F_4W_SW_SPI u8g2(U8G2_R0, PIN_SPI_SCL, PIN_SPI_SDA, U8X8_PIN_NONE, PIN_SPI_DC, PIN_SPI_RST); //(rotation, clock, data, cs, dc [, reset])
#else
#if 0
U8G2_SH1122_256X64_F_2ND_4W_HW_SPI u8g2(U8G2_R0, U8X8_PIN_NONE, PIN_SPI_DC); // (rotation, cs, dc [, reset])
#else
class U8G2_SH1122_256X64_SPI : public U8G2 {
  public: U8G2_SH1122_256X64_SPI(const u8g2_cb_t *rotation, uint8_t clock, uint8_t data, uint8_t cs, uint8_t dc, uint8_t reset = U8X8_PIN_NONE) : U8G2() {
    u8g2_Setup_sh1122_256x64_f(&u8g2, rotation, u8x8_byte_arduino_hw_spi, u8x8_gpio_and_delay_arduino);
    u8x8_SetPin(getU8x8(), U8X8_PIN_I2C_CLOCK, clock); // なぜか PIN_I2C を SPI 初期化で使ってある
    u8x8_SetPin(getU8x8(), U8X8_PIN_I2C_DATA, data);
    u8x8_SetPin_4Wire_HW_SPI(getU8x8(), cs, dc, reset);
  }
};
U8G2_SH1122_256X64_SPI u8g2(
  U8G2_R0,       // rotation
  PIN_SPI_SCL,   // clock
  PIN_SPI_SDA,   // data
  U8X8_PIN_NONE, // cs
  PIN_SPI_DC,    // dc
  PIN_SPI_RST); //[, reset])
#endif
#endif

//---------------------------------
void setLed(uint8_t r, uint8_t g, uint8_t b) {
#if USE_FASTLED
  CRGB color = (((int32_t)r) << 8) | (((int32_t)g) << 16) | b;
  leds[0] = color;
  FastLED.show();
#endif
}

#define KEY_BRK (0x0100) // bank:1
#define BANK_BRK (1)
#define KEY_RIGHT (0x0200) // bank:1
#define BANK_RIGHT (1)
#define KEY_LEFT (0x0400) // bank:1
#define BANK_LEFT (1)
#define KEY_UP (0x0800) // bank:1
#define BANK_UP (1)
#define KEY_DOWN (0x1000) // bank:1
#define BANK_DOWN (1)
#define KEY_SHIFT (0x2000) // bank:1
#define BANK_SHIFT (1)
#define KEY_DEF (0x4000) // bank:1
#define BANK_DEF (1)
#define KEY_PRO (0x0040) // bank:2
#define BANK_PRO (2)
#define KEY_POWEROFF (0x0080) // bank:2
#define BANK_POWEROFF (2)
#define KEY_ENTER (0x0100) // bank:2
#define BANK_ENTER (2)
#define KEY_CL (0x0800) // bank:3
#define BANK_CL (3)

const char* KeyChars[] = {
  "E", "X", "S", "W", "Z", "A", "Q", 0x00,
  "B", "G", "T", "V", "F", "R", "C", "D",

  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // BRK, Right, Left, Up, Down, Shift, DEF, x

  "M", "J", "U", "N", "H", "Y", 0x00, 0x00, // Pro, Off
  0x00, "=", "P", "L", "O", " ", "K", "I", // Enter

  ".", "2", "5", "8", "0", "1", "4", "7", 
  "-", "*", "/", 0x00, "+", "3", "6", "9", // CL 
};

const char* KeyShiftChars[] = {
  "#", "USING", "IF", "\"", "PRINT", "INPUT", "!", 0x00,
  "DIM", "FOR", "%", "RETURN", "GOTO", "$", "GOSUB", "THEN",

  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // BRK, Right, Left, Up, Down, Shift, DEF, x

  "CSAVE", "STEP", "?", "END", "TO", "&", 0x00, 0x00, // Pro, Off
  0x00, "RUN", ";", "LIST", ",", "CLOAD", "NEXT", ":", // Enter

  "SQRT(", ")", "]", "~", "PI", "(", "[", "_", 
  ">", "<", "^", 0x00, "EXP(", "@", "\\", "'", // CL 
};

const char* KeySmallChars[] = {
  "e", "x", "s", "w", "z", "a", "q", 0x00,
  "b", "g", "t", "v", "f", "r", "c", "d",

  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // BRK, Right, Left, Up, Down, Shift, DEF, x

  "m", "j", "u", "n", "h", "y", 0x00, 0x00, // Pro, Off
  0x00, "=", "p", "l", "o", " ", "k", "i", // Enter

  ".", "2", "5", "8", "0", "1", "4", "7", 
  "-", "*", "/", 0x00, "+", "3", "6", "9", // CL 
};

int PianoNote[] = {
  0, 74, 75, 0, 72, 73, 0, 0,
  79, 80, 0, 77, 78, 0, 76, 0,

  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0,

  83, 0, 0, 81, 82, 0, 0, 0,
  0, 0, 0, 0, 0, 84, 0, 0,

  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0,
};


String textBuffer = "";
String inputBuffer = "";
volatile uint16_t keys[4] = {0, 0, 0, 0};
volatile int syscnt = 0;
volatile int fontMode = 1;
volatile bool isPowerOn = true;
volatile bool isShowingResult = false;
volatile bool isBusy = false;
volatile bool isCaps = true;
volatile bool isShift = false;

//---------------------------------
void DisplayThread(void *pvParameters) {
  for (;;) {
  u8g2.clearBuffer();
  if (!isPowerOn) {
    u8g2.updateDisplay();
    delay(8);
    continue;
  }
#if 0
  char s[64];
  sprintf(s, "KEYS (%04x,%04x,%04x,%04x)%d", keys[0], keys[1], keys[2], keys[3], syscnt);
  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.drawStr(0, 21, s);
#endif
  
  if (isBusy) {
    u8g2.drawXBM(5, 14, 15, 5, bitmap_busy);
  }

  if (isCaps) {
    u8g2.drawXBM(37, 14, 15, 5, bitmap_caps);
  }

  u8g2.drawXBM(120, 14, 11, 5, bitmap_deg);

  if (isShift) {
    u8g2.drawXBM(235, 14, 17, 5, bitmap_shift);
  }

  int x = 0;
  int y = 32;
  if (fontMode == 0) {
    x = 0;
    y = 21;
  }

  if (textBuffer.length() == 0) {
    if (fontMode == 0) {
      u8g2.drawXBM( x, y, 5, 7, Font1_0x3E);
    }
    else {
      u8g2.drawXBM( x, y, 10, 14, Font2_0x3E);
    }
  }
  else {
    int maxChar = 20;
    if (fontMode == 0) {
      maxChar = 41;
    }
    if (fontMode == 0) {
      // multi line mode
      if (isShowingResult == false) {
        int p = 0;
        while (textBuffer.length() -p >= (maxChar+1) * 4) {
          p += (maxChar+1);
        }
        for (int i = 0; i < textBuffer.length(); i++) {
          if (p+i >= textBuffer.length()) break;
          unsigned char c = (unsigned char)textBuffer[p+i];
          if (c != 0x20) {
            const unsigned char* tex = SharpFonts1[c];
            if (tex) {
              u8g2.drawXBM( x, y, 5, 7, tex);
            }
          }
          x += 6;
          if (x > maxChar * 6) {
            y += 10;
            x = 0;
          }
        }
        if (isBusy == false) {
          u8g2.drawXBM( x, y, 5, 7, Font1_0x5F);
        }
      }
      else {
        int p = textBuffer.length()-1;
        x = 6*40;
        y = 21+30;
        for (int i = 0; i < textBuffer.length(); i++) {
          unsigned char c = (unsigned char)textBuffer[p-i];
          if (c != 0x20) {
            const unsigned char* tex = SharpFonts1[c];
            if (tex) {
              u8g2.drawXBM( x, y, 5, 7, tex);
            }
          }
           x -= 6;
          if (x < 0) break;
        }
      }
    }
    else {
      // single line mode
      if (isShowingResult == false) {
        int p = 0;
        if (textBuffer.length() >= maxChar+1) {
          p = textBuffer.length() - maxChar;
        }
        for (int i = 0; i < textBuffer.length(); i++) {
          if (p+i >= textBuffer.length()) break;
          unsigned char c = (unsigned char)textBuffer[p + i];
          if (c != 0x20) {
            const unsigned char* tex = SharpFonts2[c];
            if (tex) {
              u8g2.drawXBM( x, y, 10, 14, tex);
            }
          }
           x += 12;
        }
        if (isBusy == false) {
          u8g2.drawXBM( x, y, 10, 14, Font2_0x5F);
        }
      }
      else {
        int p = textBuffer.length()-1;
        x = 12*20;
        for (int i = 0; i < textBuffer.length(); i++) {
          unsigned char c = (unsigned char)textBuffer[p-i];
          if (c != 0x20) {
            const unsigned char* tex = SharpFonts2[c];
            if (tex) {
              u8g2.drawXBM( x, y, 10, 14, tex);
            }
          }
          x -= 12;
          if (x < 0) break;
        }
      }
    }
  }
  u8g2.updateDisplay();
  delay(8);
  syscnt++;
  }
}

#define CORE0 (0)
#define CORE1 (1)

//---------------------------------
void setup(){
  WiFi.mode(WIFI_OFF); 
#if USE_FASTLED
  // Status LED
  FastLED.addLeds<SK6812, PIN_LED, RGB>(leds, NUM_LEDS);  // GRB ordering is typical
#endif
  Serial.begin(115200);

  setLed(0x00, 0x00, 0x10);
  setupMCP23017();
  setLed(0x00, 0x00, 0x00);
  delay(100);
  
  setLed(0x10, 0x00, 0x10);
  u8g2.begin();
  u8g2.clearBuffer();
#if 0
  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.drawStr(0, 21, "PC-1245 Revived");
  u8g2.drawStr(0, 32, "OtooneDev");
  u8g2.drawStr(0, 43, "2021/12/26");
#else
  u8g2.drawXBM(73, 28, 110, 16, bitmap_sharp_logo);
#endif
  u8g2.setContrast(1); // brightness
  u8g2.sendBuffer();
  delay(1500);
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  setLed(0x00, 0x00, 0x00);
  delay(500);
  xTaskCreatePinnedToCore(DisplayThread, "DisplayThread", 4096, NULL, 1, NULL, CORE1);

  StartMusic();
}

//---------------------------------
void ReplaceHex() {
  while (1) {
    int i = textBuffer.indexOf("&");
    if (i < 0) break;
    String s = "";
    const char* ps = textBuffer.c_str() + i + 1; // & の次
    while (*ps != 0) {
      char c = *ps++;
      if ( ((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')) ) {
        s += c;
      }
      else {
        break;
      }
    }
    if (s.length() > 0) {
      int v = strtol(s.c_str(),NULL,16);
      s = "&" + s;
      String sv = "";
      sv += v;
      textBuffer.replace(s, sv);
    }
  }
}

static uint16_t currKeys[4] = {0, 0, 0, 0};
static bool isRun = false;
static String runMode = "";
static uint32_t count = 0;

//---------------------------------
void piano(uint16_t* keysDown) {
  isDef = ((keys[BANK_DEF] & KEY_DEF) == 0);
  isShift = ((keys[BANK_SHIFT] & KEY_SHIFT) == 0);
  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 16; i++) {
      if (keysDown[j] & (1 << i)) {
        int n = PianoNote[j*16+i];
        if (n > 0) {
          if (isShift) {
            n += 12;
          }
          if (isDef) {
            n -= 12;
          }
          Play(n);
        }
      }
    }
  }
  Tick();
  delay(10);
}

//---------------------------------
void loop() {
  setLed(0x00, 0x10, 0x00);
  //delay(50);
  uint16_t keysUp[4] = {0, 0, 0, 0};
  uint16_t keysDown[4] = {0, 0, 0, 0};
  keys[0] = readMCP23017(0);
  keys[1] = readMCP23017(1);
  keys[2] = readMCP23017(2);
  keys[3] = readMCP23017(3);
  for (int i = 0; i < 4; i++) {
    if (currKeys[i] == 0) {
      currKeys[i] = keys[i];
    }
    else {
      keysUp[i] = (keys[i] ^ currKeys[i]) & keys[i];
      keysDown[i] = (keys[i] ^ currKeys[i]) & (~keys[i]);
      currKeys[i] = keys[i];
    }
  }

  if ((isPowerOn == false) && (keysUp[BANK_POWEROFF] & KEY_POWEROFF)) {
    ESP.restart();
  }

  if (isRun) {
    textBuffer = "";
    if (runMode == "piano") {
      textBuffer = "Pocket PIANO";
      piano(keysDown);
    }
    else {
      textBuffer += count;
      count++;
    }
    if (keysUp[BANK_BRK] & KEY_BRK) {
      isRun = false;
      isBusy = false;
      textBuffer = "";
    }
    return;
  }

  if (keysDown[BANK_CL] & KEY_CL) {
    isShowingResult = false;
    textBuffer = "";
  }
  fontMode = (int)((keys[BANK_PRO] & KEY_PRO) != 0);
  if (isPowerOn && (keysDown[BANK_POWEROFF] & KEY_POWEROFF)) {
    isPowerOn = false;
  }
  if (isShowingResult) {
    // 結果表示中
    if ((keysUp[BANK_LEFT] & KEY_LEFT) || (keysUp[BANK_RIGHT] & KEY_RIGHT) || (keysUp[BANK_BRK] & KEY_BRK)) {
      isShowingResult = false;
      textBuffer = inputBuffer;
    }
  } else {
    // 入力中
    if (keysDown[BANK_DEF] & KEY_DEF) {
      isCaps = 1 - isCaps;
    }
    isShift = ((keys[BANK_SHIFT] & KEY_SHIFT) == 0);
    if ((keysUp[BANK_BRK] & KEY_BRK) && (textBuffer.length() > 0)) {
      textBuffer = textBuffer.substring(0, textBuffer.length()-1);
    }
    if ((keysUp[BANK_ENTER] & KEY_ENTER) && (textBuffer.length() > 0)) {
      inputBuffer = textBuffer;
      textBuffer.toLowerCase();
      if (textBuffer.substring(0, 3) == "run") {
        runMode = textBuffer.substring(4);
        isRun = true;
        isBusy = true;
        count = 0;
      }
      else {
        textBuffer.replace("pi", "3.141592653");
        if (textBuffer.indexOf("&") >= 0) {
          ReplaceHex();
        }
        double v = eval(textBuffer.c_str());
        char s[32];
        sprintf(s, "%f", v);
        textBuffer = "";
        textBuffer += s;
        textBuffer = textBuffer.substring(0, 10);
        if (textBuffer.indexOf(".") >= 0) {
          // 少数以下あり
          while (textBuffer.endsWith("0")) {
            textBuffer = textBuffer.substring(0, textBuffer.length()-1);
          }
        }
        else {
          textBuffer += ".";
        }
        isShowingResult = true;
      }
    }
  }

  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 16; i++) {
      if (keysDown[j] & (1 << i)) {
        const char* p = 0;
        if (isCaps) {
          p = KeyChars[j*16+i];
          if (isShift) {
            p = KeyShiftChars[j*16+i];
          }
        }
        else {
          p = KeySmallChars[j*16+i];
          if (isShift) {
            p = KeyChars[j*16+i];
          }
        }
        if (p) {
          if (isShowingResult) {
            if (textBuffer.endsWith(".")) {
              textBuffer = textBuffer.substring(0, textBuffer.length()-1);
            }
          }
          isShowingResult = false;
          textBuffer += p;
        }
      }
    }
  }
  delay(10);
}
