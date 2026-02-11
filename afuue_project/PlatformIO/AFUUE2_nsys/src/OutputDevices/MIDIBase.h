#pragma once
#include <Arduino.h>
#include "OutputDeviceBase.h"

#define MIDI_CC_BREATH_CONTROL_MSB (2)
#define MIDI_CC_BREATH_CONTROL_LSB (34)

class MIDIBase : public OutputDeviceBase {
public:
    MIDIBase() {}
    const char* GetName() const override { return "MIDI"; }

protected:
    uint64_t m_time = 0;
    int32_t m_progNum = -1;
    float m_inputVolume = 0.0f;
    float m_inputNote = 0.0f;
    uint8_t m_prevNote = 0;
    uint8_t m_channelNo = 1;
    bool m_playing = false;
    bool m_inputPlaying = false;

    //--------
    void MidiUpdate(Parameters& params, Message& msg) {
        static int count = 0;
        uint64_t t = micros();
        if (t - m_time > 250*1000) {
            m_time = t;
            ActiveNotify();
        }

        if (m_progNum != params.GetWaveTableIndex()) {
            m_progNum = params.GetWaveTableIndex();
            ProgramChange(m_progNum);
        }
        if (!m_playing) {
            if (msg.volume > 0.01f) {
                m_playing = true;
                uint8_t note = static_cast<uint8_t>(Clamp(static_cast<int32_t>(msg.note), 0, 127));
                m_prevNote = note;
                NoteOn(note, 120);
                BreathControl(msg.volume);
                count = 0;
            }
        }
        else {
            if (msg.volume < 0.01f) {
                m_playing = false;
                NoteOff(m_prevNote);
            }
            else {
                int32_t note = static_cast<int32_t>(msg.note);
                if (note != m_prevNote) {
                    NoteOff(m_prevNote);
                    m_prevNote = note;
                    NoteOn(note, 120);
                    count = 0;
                }
                uint16_t breath = static_cast<uint16_t>(Clamp(msg.volume, 0.0f, 1.0f) * 16383.0f);
                BreathControl(breath);
                uint16_t bend = static_cast<uint16_t>(8192.0f + Clamp(msg.bend, -1.0f, 1.0f) * 8191.0f);
                PicthBendControl(bend);
            }
        }
        count++;
    }

    //--------
    void ReceiveMessage(uint8_t byte1, uint8_t byte2, uint8_t byte3) {
        switch (byte1 & 0xF0) {
            case 0x90:
                m_inputPlaying = true;
                m_inputNote = byte2;
                m_inputVolume = byte3 / 127.0f;
                break;
            case 0x80:
                m_inputPlaying = false;
                break;
        }       
    }

    //--------
    virtual void NoteOn(uint8_t note, uint8_t velocity) = 0;
    virtual void NoteOff(uint8_t note) = 0;
    virtual void PicthBendControl(uint16_t bend) = 0;
    virtual void BreathControl(uint16_t vol) = 0;
    virtual void ProgramChange(uint8_t pgNum) = 0;
    virtual void ActiveNotify() = 0;

private:
    float GetRate(float value, float min, float max) {
        if (max <= min) {
            return 0;
        }
        float v = Clamp(value, min, max) - min;
        return v / (max - min);
    }

};
