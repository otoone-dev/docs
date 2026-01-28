#pragma once
#include <Arduino.h>
#include "OutputDeviceBase.h"

#define MIDI_CC_BREATH_CONTROL_MSB (2)
#define MIDI_CC_BREATH_CONTROL_LSB (34)

class MIDIBase : public OutputDeviceBase {
public:
    MIDIBase() {}
    const char* GetName() const override { return "MIDI"; }

    OutputResult Update(Parameters& parameters, Message& msg) override {
        if (m_progNum != parameters.GetWaveTableIndex()) {
            m_progNum = parameters.GetWaveTableIndex();
            ProgramChange(m_progNum);
        }
        if (!m_playing) {
            if (msg.volume > 0.001f) {
                m_playing = true;
                int32_t note = static_cast<int32_t>(msg.note);
                m_prevNote = note;
                NoteOn(note, msg.volume);
            }
        }
        else {
            if (msg.volume < 0.001f) {
                m_playing = false;
                NoteOff(m_prevNote);
            }
            else {
                BreathControl(msg.volume);
                PicthBendControl(msg.bend);
            }
        }
        return OutputResult{ false, 0.0f };
    }

protected:
    int32_t m_progNum = -1;
    int32_t m_prevNote = 0;
    uint8_t m_channelNo = 1;
    bool m_playing = false;

    uint8_t GetMSB(float value, float min = 0.0f, float max = 1.0f) {
        float r = GetRate(value, min, max);
        uint16_t u16 = static_cast<uint16_t>(r * 0xFFFF) >> 2; // 14bit
        return (u16 >> 7) & 0x7F;
    }
    uint8_t GetLSB(float value, float min = 0.0f, float max = 1.0f) {
        float r = GetRate(value, min, max);
        uint16_t u16 = static_cast<uint16_t>(r * 0xFFFF) >> 2; // 14bit
        return u16 & 0x7F;
    }

    //--------
    virtual void NoteOn(int32_t note, float vol) = 0;
    virtual void NoteOff(int32_t note) = 0;
    virtual void PicthBendControl(float bend) = 0;
    virtual void BreathControl(float vol) = 0;
    virtual void ProgramChange(int32_t pgNum) = 0;
    virtual void ChangeBreathControlMode() = 0;
    virtual void ActiveNotify() = 0;

private:
    float GetRate(float value, float min, float max) {
        if (max <= min) {
            return 0;
        }
        float v = Clamp<float>(value, min, max) - min;
        return v / (max - min);
    }

};
