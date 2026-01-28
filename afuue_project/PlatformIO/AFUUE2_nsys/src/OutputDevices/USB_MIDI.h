#pragma once
#include <Arduino.h>
#include "MIDIBase.h"

#include <USB.h>
#include <USBMIDI.h>
USBMIDI MIDI("AFUUE2R");

//----------------
class USB_MIDI : public MIDIBase {
public:
    USB_MIDI() {}

    const char* GetName() const override { return "USB-MIDI"; }

    InitializeResult Initialize() override {
        InitializeResult result;
        MIDI.begin();
        USB.begin();
        delay(1000);
        m_mounted = tud_mounted();
        if (m_mounted) {
            result.skipAfter = true;
        }
        return result;
    }

protected:
    //--------
    void NoteOn(int32_t note, float vol) override {
        if (!m_mounted) {
            return;
        }
        MIDI.noteOn(note, static_cast<uint8_t>(Clamp<int32_t>(static_cast<int32_t>(127*vol), 0, 127)), m_channelNo);
        BreathControl(vol);
    }
    //--------
    void NoteOff(int32_t note) override {
        if (!m_mounted) {
            return;
        }
        MIDI.noteOff(note, 0x00, m_channelNo);
    }
    //--------
    void PicthBendControl(float bend) override {
        if (!m_mounted) {
            return;
        }
        MIDI.pitchBend(Clamp<float>(bend, -1.0f, 1.0f), m_channelNo);
    }
    //--------
    void BreathControl(float vol) override {
        if (!m_mounted) {
            return;
        }
        MIDI.controlChange(MIDI_CC_BREATH_CONTROL_MSB, GetMSB(vol, 0.0f, 1.0f), m_channelNo);
        MIDI.controlChange(MIDI_CC_BREATH_CONTROL_LSB, GetLSB(vol, 0.0f, 1.0f), m_channelNo);
    }
    //--------
    void ProgramChange(int32_t pgNum) override {
        if (!m_mounted) {
            return;
        }
        MIDI.programChange(static_cast<uint8_t>(Clamp<int32_t>(pgNum, 0, 127)), m_channelNo);
    }
    //--------
    void ChangeBreathControlMode() override {
        if (!m_mounted) {
            return;
        }
        
    }
    //--------
    void ActiveNotify() override {
        if (!m_mounted) {
            return;
        }
        
    }

private:
    bool m_mounted = false;
};
