#pragma once
#include <Arduino.h>
#include "MIDIBase.h"
#include <vector>

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
        m_sendData[0].reserve(32);
        m_sendData[1].reserve(32);
        USB.begin();
        MIDI.begin();
        delay(1000);
        m_mounted = tud_mounted();
        if (m_mounted) {
            //result.skipAfter = true;
        }
        return result;
    }

    OutputResult Update(Parameters& params, Message& msg) override {
        m_mounted = tud_midi_mounted();
        if (m_mounted) {
            if (tud_midi_available()) {
                midiEventPacket_t recv = {0, 0, 0, 0};
                if (MIDI.readPacket(&recv)) {
                    // 受信処理
                    ReceiveMessage(recv.byte1, recv.byte2, recv.byte3);
                    //if (params.dispMessage.empty()) {
                    //    char s[32];
                    //    sprintf(s, "%02X %02X\n%02X %02X", recv.header, recv.byte1, recv.byte2, recv.byte3);
                    //    params.SetDispMessage(s, 200);
                    //}
                }
            }

            MidiUpdate(params, msg);

            int32_t i = m_sendCount % 2;
            tud_midi_stream_write(0, m_sendData[i].data(), m_sendData[i].size());
            msg.volume = 0.0f; // 後続のスピーカーに渡さない
            if (m_sendData[i].size() > 0) {
                m_sendCount++;
                m_sendData[m_sendCount%2].clear();
            }
        }
        else {
            params.SetDispMessage("UNMOUNTED", 100);
        }
        return OutputResult{ false, 0.0f };
    }

protected:
    //--------
    void NoteOn(uint8_t note, uint8_t velocity) override {
        m_noteOn = true;
        PushData(0x90 + m_channelNo, note, velocity);
    }
    //--------
    void NoteOff(uint8_t note) override {
        m_noteOn = false;
        PushData(0x80 + m_channelNo, note, 0);
    }
    //--------
    void PicthBendControl(uint16_t bend) override {
        uint8_t d0 = bend & 0x7F;
        uint8_t d1 = (bend >> 7) & 0x7F;
        PushData(0xE0 + m_channelNo, d0, d1);
    }
    //--------
    void BreathControl(uint16_t vol) override {
        PushData(0xB0 + m_channelNo, 0x02, static_cast<uint8_t>((vol >> 7) & 0x7F));
    }
    //--------
    void ProgramChange(uint8_t pgNum) override {
        PushData(0xC0 + m_channelNo, pgNum);
    }
    //--------
    void ActiveNotify() override {
        PushData(0xFE);
    }

private:
    void PushData(int32_t status, int32_t data1 = -1, int32_t data2 = -1, int32_t data3 = -1) {
        m_sendData[m_sendCount%2].push_back(static_cast<uint8_t>(status));
        if (data1 >= 0) {
            m_sendData[m_sendCount%2].push_back(static_cast<uint8_t>(data1));
        }
        if (data2 >= 0) {
            m_sendData[m_sendCount%2].push_back(static_cast<uint8_t>(data2));
        }
        if (data3 >= 0) {
            m_sendData[m_sendCount%2].push_back(static_cast<uint8_t>(data3));
        }
    }
    std::vector<uint8_t> m_sendData[2];
    int32_t m_sendCount = 0;
    bool m_mounted = false;
    bool m_noteOn = false;
};
