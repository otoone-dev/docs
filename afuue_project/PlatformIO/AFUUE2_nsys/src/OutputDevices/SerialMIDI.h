#pragma once
#include <Arduino.h>
#include "MIDIBase.h"

class SerialMIDI : public MIDIBase {
public:
    SerialMIDI(gpio_num_t outPin) : m_outPin(outPin) {}

    const char* GetName() const override { return "MIDIout"; }

    InitializeResult Initialize() override {
        InitializeResult result;
        pinMode(m_outPin, OUTPUT);
        return result;
    }

protected:
    //--------
    void NoteOn(int32_t note, int32_t velocity) override {
    }
    //--------
    void NoteOff(int32_t note) override {
    }
    //--------
    void PicthBendControl(float bend) override {
        //uint16_t value = static_cast<uint16_t>(static_cast<int32_t>(bend * 0x7FFF) + 0x8000) >> 2; // 14bit
        //uint8_t d0 = static_cast<uint8_t>(value & 0x7F);
        //uint8_t d1 = static_cast<uint8_t>((value >> 7) & 0x7F);
        //SendMessage3(0xE0, d0, d1);
    }
    //--------
    void BreathControl(float vol) override {
        //SendMessage3(0xB0, 0x02, vol); // 0x02 : Breath Control
        /*
        midiPacket[1] = 0xD0 + channelNo; // AFTER TOUCH
        midiPacket[2] = vol;

        midiPacket[1] = 0x0B; // expression
            if (midiMode == MIDIMODE::BREATHCONTROL) {
                midiPacket[1] = 0x02; // breath control
            }
            else if (midiMode == MIDIMODE::MAINVOLUME) {
                midiPacket[1] = 0x07; // main volume
            }
            else if (midiMode == MIDIMODE::CUTOFF) {
                midiPacket[1] = 43; // CUTOFF (for KORG NTS-1)
            }
        */
    }
    //--------
    void ProgramChange(int32_t pgNum) override {
    }
    //--------
    void ChangeBreathControlMode() override {
    }
    //--------
    void ActiveNotify() override {
    }

private:
    gpio_num_t m_outPin;
};
