#pragma once
#include <string>

const uint8_t STATUS_NOTE_OFF   = 0x80;
const uint8_t STATUS_NOTE_ON    = 0x90;
const uint8_t STATUS_CONTROL    = 0xB0;
const uint8_t STATUS_PITCH_BEND = 0xE0;
const uint8_t CC_BREATH         = 0x02;

static uint8_t runningStatus = 0;  // ランニングステータス保持
static uint8_t data1 = 0;
static uint8_t byteCount = 0;
static bool playing = false;
static uint8_t playingNote = 61;
static uint8_t noteOnVelocity = 0;
static uint8_t playingBC = 0;
static uint16_t pitchBend = 0;
static std::string midiMsg = "";
static int32_t recvCnt[10];

void MidiReceive(uint8_t b) {
  // ステータスバイトの場合
  if (b & 0x80) {
    runningStatus = b;
    byteCount = 0;
    return;
  }

  // データバイトの場合
  switch (runningStatus & 0xF0) {
    case STATUS_NOTE_ON:
    case STATUS_NOTE_OFF:
      if (byteCount == 0) {
        data1 = b;  // note number
        byteCount = 1;
      } else {
        uint8_t note = data1;
        uint8_t vel  = b;
        if ((runningStatus & 0xF0) == STATUS_NOTE_ON && vel > 0) {
          //handleNoteOn(runningStatus & 0x0F, note, vel);
          playing = true;
          playingNote = note;
          noteOnVelocity = vel;
          recvCnt[0]++;
        } else {
          //handleNoteOff(runningStatus & 0x0F, note, vel);
          playing = false;
          recvCnt[1]++;
        }
        byteCount = 0;
      }
      break;

    case STATUS_CONTROL:
      if (byteCount == 0) {
        data1 = b;
        byteCount = 1;
      }
      else {
        uint8_t cc = data1;
        uint8_t val = b;
        if (cc == CC_BREATH) {
          playingBC = val;
          //handleBreath(runningStatus & 0x0F, val);
          recvCnt[2]++;
        }
        byteCount = 0;
      }
    break;

    case STATUS_PITCH_BEND:
      if (byteCount == 0) {
        data1 = b; // LSB
        byteCount = 1;
      } else {
        uint16_t value = (b << 7) | data1; // 14bit値
        pitchBend = value;
        recvCnt[3]++;
        //handlePitchBend(runningStatus & 0x0F, value);
        byteCount = 0;
      }
      break;

    default:
      // 未対応メッセージは破棄し、バイトカウンタをリセット
      byteCount = 0;
      break;
  }
  char s[32];
  sprintf(s, "%d,%d,%d,%d", recvCnt[0], recvCnt[1], recvCnt[2], recvCnt[3]);
  midiMsg = s;
}
