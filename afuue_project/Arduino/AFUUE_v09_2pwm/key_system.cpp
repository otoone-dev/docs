#include "afuue_common.h"
#include "key_system.h"
#include "sensors.h"

//---------------------------------
KeySystem::KeySystem() {}

//---------------------------------
// 初期化
int KeySystem::Initialize() {
#ifdef HAS_IOEXPANDER
  return 0;//setupIOExpander();
#endif

#if (MAINUNIT == M5STAMP_S3)
  const int keyPortList[14] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 ,9, 10, 44, 46 };
  for (int i = 0; i < 14; i++) {
    pinMode(keyPortList[i], INPUT_PULLUP); // INPUT FOR SWITCHES
  }
#endif
  return 0;
}

//---------------------------------
// 演奏用のキー処理
void KeySystem::UpdateKeys(uint16_t exKeys) {
#ifdef HAS_IOEXPANDER
  uint16_t mcpKeys = exKeys;// readFromIOExpander();
#if (MAINUNIT == M5STICKC_PLUS)
  keyLowC = ((mcpKeys & 0x0001) != 0);
  keyEb = ((mcpKeys & 0x0002) != 0);
  keyD = ((mcpKeys & 0x0004) != 0);
  keyE = ((mcpKeys & 0x0008) != 0);
  keyF = ((mcpKeys & 0x0010) != 0);
  keyLowCs = ((mcpKeys & 0x0200) != 0);
  keyGs = ((mcpKeys & 0x0400) != 0);
  keyG = ((mcpKeys & 0x0800) != 0);
  keyA = ((mcpKeys & 0x1000) != 0);
  keyB = ((mcpKeys & 0x2000) != 0);
  octDown = ((mcpKeys & 0x4000) != 0);
  octUp = ((mcpKeys & 0x8000) != 0);
#elif (MAINUNIT == M5ATOM_S3)
  keyLowC = ((mcpKeys & 0x0001) != 0);
  keyEb = ((mcpKeys & 0x0002) != 0);
  keyD = ((mcpKeys & 0x0004) != 0);
  keyE = ((mcpKeys & 0x0008) != 0);
  keyF = ((mcpKeys & 0x0010) != 0);
  keyLowCs = ((mcpKeys & 0x0020) != 0);
  keyGs = ((mcpKeys & 0x0040) != 0);
  keyG = ((mcpKeys & 0x0080) != 0);
  keyA = ((mcpKeys & 0x0100) != 0);
  keyB = ((mcpKeys & 0x0200) != 0);
  octUp = ((mcpKeys & 0x0400) != 0);
  octDown = ((mcpKeys & 0x0800) != 0);
#endif
#endif
#if (MAINUNIT == M5STAMP_S3)
  keyLowC = digitalRead(1);
  keyEb = digitalRead(2);
  keyD = digitalRead(3);
  keyE = digitalRead(4);
  keyF = digitalRead(5);
  keyLowCs = digitalRead(6);
  keyGs =digitalRead(7);
  keyG = digitalRead(8);
  keyA = digitalRead(9);
  keyB = digitalRead(10);
  octDown = digitalRead(44);
  octUp = digitalRead(46);
#endif
}

//---------------------------------
// 機能操作向けキー処理
void KeySystem::UpdateMenuKeys(bool isKeyRepeatEnabled) {
  uint16_t k = 0;
  if (keyLowC == LOW) k |= (1 << 0);
  if (keyEb == LOW) k |= (1 << 1);
  if (keyD == LOW) k |= (1 << 2);
  if (keyE == LOW) k |= (1 << 3);
  if (keyF == LOW) k |= (1 << 4);
  if (keyLowCs == LOW) k |= (1 << 5);
  if (keyGs == LOW) k |= (1 << 6);
  if (keyG == LOW) k |= (1 << 7);
  if (keyA == LOW) k |= (1 << 8);
  if (keyB == LOW) k |= (1 << 9);
  if (octDown == LOW) k |= (1 << 10);
  if (octUp == LOW) k |= (1 << 11);
#if (MAINUNIT == M5STAMP_S3)
  if (digitalRead(0) == LOW) k |= (1 << 12);
#endif
#if (MAINUNIT == M5ATOM_S3)
  if (digitalRead(41) == LOW) k |= (1 << 12);
#endif

  keyData = k;
  keyPush = (keyData ^ keyCurrent) & keyData;
  if (isKeyRepeatEnabled && (keyData != 0) && (keyData == keyCurrent)) {
    keyRepeatCount++;
    if (keyRepeatCount > 15) {
      if (keyRepeatCount % 2 == 0) {
        keyPush = keyData;
      }
    }
  }
  else {
    keyRepeatCount = 0;
  }
  keyCurrent = keyData;
}

//---------------------------------
// 現在のキーで再生されるノート番号
float KeySystem::GetNoteNumber(int baseNote) const {
  int b = 0;
  if (keyLowC == LOW) b |= (1 << 7);
  if (keyD == LOW) b |= (1 << 6);
  if (keyE == LOW) b |= (1 << 5);
  if (keyF == LOW) b |= (1 << 4);
  if (keyG == LOW) b |= (1 << 3);
  if (keyA == LOW) b |= (1 << 2);
  if (keyB == LOW) b |= (1 << 1);

  float n = 0.0f;

  // 0:C 2:D 4:E 5:F 7:G 9:A 11:B 12:HiC
  int g = (b & 0b00001110);  // keyG, keyA, keyB の組み合わせ
  
  if ((g == 0b00001110) || (g == 0b00001100)) {      // [*][*][*] or [*][*][ ]
    if ((b & 0b11110000) == 0b11110000) {
      n = 0.0f; // C
      if (keyEb == LOW) n = 2.0f; // D
    }
    else if ((b & 0b11110000) == 0b01110000) {
      n = 2.0f; // D
    }
    else if ((b & 0b01110000) == 0b00110000) {
      n = 4.0f; // E
      //if (b & 0b10000000) n -= 1.0f;
    }
    else if ((b & 0b00110000) == 0b00010000) {
      n = 5.0f; // F
      if (b & 0b10000000) n -= 2.0f;
      //if (b & 0b01000000) n -= 1.0f;
    }
    else if ((b & 0b00010000) == 0) {
      n = 7.0f; // G
    if (b & 0b10000000) n -= 2.0f;
      //if (b & 0b01000000) n -= 1.0f;
      if (b & 0b00100000) n -= 1.0f;
    }
    if ((b & 0b00000010) == 0) {
      n += 12.0f;
    }
  }
  else if (g == 0b00000110) { // [ ][*][*]
    n = 9.0f; // A
    if (b & 0b10000000) n -= 2.0f;
    //if (b & 0b01000000) n -= 1.0f;
    if (b & 0b00100000) n -= 1.0f;
    if (b & 0b00010000) n += 1.0f; // SideBb 相当
  }
  else if (g == 0b00000010) { // [ ][ ][*]
    n = 11.0f; // B
    if (b & 0b10000000) n -= 2.0f;
    //if (b & 0b01000000) n -= 1.0f;
    if (b & 0b00100000) n -= 1.0f; // Flute Bb
    if (b & 0b00010000) n -= 1.0f;
  }
  else if (g == 0b00000100) { // [ ][*][ ]
    n = 12.0f; // HiC
    if (b & 0b10000000) n -= 2.0f;
    if (b & 0b01000000) n += 2.0f; // HiD トリル用
    if (b & 0b00100000) n -= 1.0f;
    if (b & 0b00010000) n -= 1.0f;
  }
  else if (g == 0b00001000) { // [*][ ][ ]
    n = 20.0f; // HiA
    if (b & 0b10000000) n -= 2.0f;
    if (b & 0b01000000) n += 2.0f;
    if (b & 0b00100000) n -= 1.0f;
    if (b & 0b00010000) n -= 1.0f;
  }
  else if (g == 0b00001010) { // [*][ ][*]
    n = 10.0f; // A#
    if (b & 0b10000000) n -= 2.0f;
    if (b & 0b01000000) n += 2.0f;
    if (b & 0b00100000) n -= 1.0f;
    if (b & 0b00010000) n -= 1.0f;
  }
  else if (g == 0b00000000) { // [ ][ ][ ]
    n = 13.0f; // HiC#
    //if (b & 0b10000000) n -= 0.5f;
    if (b & 0b01000000) n += 2.0f; // HiEb トリル用
    if (b & 0b00100000) n -= 1.0f;
    if (b & 0b00010000) n -= 1.0f;
  }

  if ((keyEb == LOW)&&(keyLowC != LOW)) n += 1.0f; // #
  if (keyGs == LOW) n += 1.0f; // #
  if (keyLowCs == LOW) n -= 1.0f; // b

  if (octUp == LOW) n += 12.0f;
  else if (octDown == LOW) n -= 12.0f;

  float bnote = baseNote - 13; // C
  return bnote + n;
}

//---------------------------------
bool KeySystem::IsBendKeysDown() const {
  return (keyLowC == LOW) && (keyEb == LOW);
}

//---------------------------------
// キーが押されているかどうか
uint16_t KeySystem::GetKeyData() const {
  return keyData;
}

//---------------------------------
// キーを押した直後かどうか
uint16_t KeySystem::GetKeyPush() const {
  return keyPush;
}
