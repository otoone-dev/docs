#pragma once

#include "afuue_common.h"

#ifdef USE_MIDI

enum MIDIMODE {
  BREATHCONTROL,
  EXPRESSION,
  AFTERTOUCH,
  MAINVOLUME,
  CUTOFF,
  MAX,
};

class AfuueMIDI {
public:
  AfuueMIDI() {}
  bool Initialize();
  void Update(int note, float requestedVolume, bool isLipSensorEnabled, float bendNoteShift);

  void NoteOn(int note, int vol);
  void NoteOff();
  void PicthBendControl(int bend);
  void BreathControl(int vol);
  void ProgramChange(int no);
  void ChangeBreathControlMode();
  void ActiveNotify();

private:
  MIDIMODE midiMode = MIDIMODE::BREATHCONTROL; // 0:BreathControl 1:Expression 2:AfterTouch 3:MainVolume 4:CUTOFF(for KORG NTS-1)
  byte midiPgNo = 51;
  uint8_t midiPacket[32];
  bool deviceConnected = false;
  bool playing = false;
  int channelNo = 1;
  int prevNoteNumber = -1;
  unsigned long activeNotifyTime = 0;
  bool isUSBMounted = false;

  void SerialSend(uint8_t* midiPacket, size_t size);

#if ENABLE_BLE_MIDI
  BLECharacteristic *pCharacteristic;

  class MyServerCallbacks: public BLEServerCallbacks {
  public:
    MyServerCallbacks(AfuueMIDI* _afuueMidi) {
      afuueMidi = _afuueMidi;
    }
      void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        SerialPrintLn("connected");

        afuueMidi->NoteOn(baseNote + 1, 50);
        delay(500);
        afuueMidi->NoteOff();
      };

      void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        SerialPrintLn("disconnected");
      }
  private:
    AfuueMIDI* afuueMidi;
  };
#endif

};
#else
class AfuueMIDI {
public:
  AfuueMIDI() {}
  bool Initialize() {}
  void Update(int note, float requestedVolume, bool isLipSensorEnabled, float bendNoteShift) {}

  void NoteOn(int note, int vol) {}
  void NoteOff() {}
  void PicthBendControl(int bend) {}
  void BreathControl(int vol) {}
  void ProgramChange(int no) {}
  void ChangeBreathControlMode() {}
  void ActiveNotify() {}
};
#endif // USE_MIDI
