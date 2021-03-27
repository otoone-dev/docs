#ifndef MIDI_H
#define MIDI_H

extern bool midiEnabled;
extern byte midiMode; // 1:BreathControl 2:Expression 3:AfterTouch 4:MainVolume 5:CUTOFF(for KORG NTS-1)
extern byte midiPgNo;
extern bool deviceConnected;
extern bool playing;
extern int channelNo;
extern int prevNoteNumber;
extern unsigned long activeNotifyTime;

extern void MIDI_Initialize();
extern void MIDI_NoteOn(int note, int vol);
extern void MIDI_ChangeNote(int note, int vol);
extern void MIDI_NoteOff();
extern void MIDI_BreathControl(int vol);
extern void MIDI_ProgramChange(int no);
extern void MIDI_ActiveNotify();

#endif
