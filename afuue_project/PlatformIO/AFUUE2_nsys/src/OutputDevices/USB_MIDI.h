#pragma once
#include "MIDIBase.h"

#include <USB.h>
#include <USBMIDI.h>
USBMIDI MIDI("AFUUE2R");

//----------------
class USB_MIDI : public MIDIBase {
public:
    USB_MIDI() {}

    const char* GetName() const override { return "USB-MIDI"; }

    InitializeResult Initialize(Parameters& params) override {
        InitializeResult result;
        USB.begin();
        MIDI.begin();
        delay(1000);
        m_mounted = tud_mounted();
        if (m_mounted) {
            m_initialized = true;
            params.playMode = PlayMode::USBMIDI_Normal;
            result.skipAfter = true; // 後続の SerialMIDI と Speaker を起動しない
        }
        return result;
    }

    OutputResult Update(Parameters& params, Message& msg) override {
        if (!m_initialized) {
            return OutputResult{ false, 0.0f };
        }

        m_mounted = tud_midi_mounted();
        if (m_mounted) {
            uint32_t recvSize = tud_midi_available();
            if (recvSize > 0) {
                uint8_t data[256];
                int readSize = (recvSize < 256 ? recvSize : 256);
                tud_midi_stream_read(data, readSize);
                ReceiveData(data, readSize);
            }

            MidiUpdate(params, msg);

            msg.volume = 0.0f; // 後続のスピーカーに渡さない
        }
        else {
            params.SetDispMessage("UNMOUNTED", 100);
        }
        return OutputResult{ false, 0.0f };
    }
protected:
    void SendData(const uint8_t* buff, size_t size) override {
        tud_midi_stream_write(0, buff, size);
    }

private:
    bool m_initialized = false;
    bool m_mounted = false;
};
