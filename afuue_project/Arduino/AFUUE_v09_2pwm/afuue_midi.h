#ifndef AFUUEMIDI_H
#define AFUUEMIDI_H

enum MIDIMODE {
  BREATHCONTROL,
  EXPRESSION,
  AFTERTOUCH,
  MAINVOLUME,
  CUTOFF,
  MAX,
};

extern MIDIMODE midiMode; // 0:BreathControl 1:Expression 2:AfterTouch 3:MainVolume 4:CUTOFF(for KORG NTS-1)
extern byte midiPgNo;
extern bool deviceConnected;
extern bool playing;
extern int channelNo;
extern int prevNoteNumber;
extern unsigned long activeNotifyTime;

extern bool AFUUEMIDI_Initialize();
extern void AFUUEMIDI_NoteOn(int note, int vol);
extern void AFUUEMIDI_ChangeNote(int note, int vol);
extern void AFUUEMIDI_NoteOff();
extern void AFUUEMIDI_PicthBendControl(int value);
extern void AFUUEMIDI_BreathControl(int vol);
extern void AFUUEMIDI_ProgramChange(int no);
extern void AFUUEMIDI_ChangeBreathControlMode();
extern void AFUUEMIDI_ActiveNotify();

#endif //AFUUEMIDI_H
