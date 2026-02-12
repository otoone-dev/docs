#pragma once
#include <Arduino.h>
#include "OutputDeviceBase.h"
#include <vector>
#include <queue>

#define MIDI_CC_BREATH_CONTROL_MSB (2)
#define MIDI_CC_BREATH_CONTROL_LSB (34)

class MIDIBase : public OutputDeviceBase {
public:
    MIDIBase() {
        m_sendData[0].reserve(32);
        m_sendData[1].reserve(32);
    }
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
                uint8_t note = static_cast<uint8_t>(Clamp(static_cast<int32_t>(msg.note), 0, 127));
                m_prevNote = note;
                NoteOn(note, 120);
                BreathControl(msg.volume);
            }
        }
        else {
            if (msg.volume < 0.01f) {
                NoteOff(m_prevNote);
            }
            else {
                int32_t note = static_cast<int32_t>(msg.note);
                if (note != m_prevNote) {
                    NoteOff(m_prevNote);
                    m_prevNote = note;
                    NoteOn(note, 120);
                }
                uint16_t breath = static_cast<uint16_t>(Clamp(msg.volume, 0.0f, 1.0f) * 16383.0f);
                BreathControl(breath);
                uint16_t bend = static_cast<uint16_t>(8192.0f + Clamp(msg.bend, -1.0f, 1.0f) * 8191.0f);
                PicthBendControl(bend);
            }
        }
        int32_t i = m_sendCount % 2;

        if (m_sendData[i].size() > 0) {
            SendData(m_sendData[i].data(), m_sendData[i].size());
            m_sendCount++;
            m_sendData[m_sendCount%2].clear();
        }
    }

    //--------
    void ReceiveData(const uint8_t* buff, size_t size) {
        // TBW
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
    void NoteOn(uint8_t note, uint8_t velocity) {
        m_playing = true;
        PushData(0x90 + m_channelNo, note, velocity);
    }
    //--------
    void NoteOff(uint8_t note) {
        m_playing = false;
        PushData(0x80 + m_channelNo, note, 0);
    }
    //--------
    void PicthBendControl(uint16_t bend) {
        uint8_t d0 = bend & 0x7F;
        uint8_t d1 = (bend >> 7) & 0x7F;
        PushData(0xE0 + m_channelNo, d0, d1);
    }
    //--------
    void BreathControl(uint16_t vol) {
        PushData(0xB0 + m_channelNo, 0x02, static_cast<uint8_t>((vol >> 7) & 0x7F));
    }
    //--------
    void ProgramChange(uint8_t pgNum) {
        PushData(0xC0 + m_channelNo, pgNum);
    }
    //--------
    void ActiveNotify() {
        PushData(0xFE);
    }

    //--------
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

    //--------
    virtual void SendData(const uint8_t* buff, size_t size) = 0;

private:
    //--------
    float GetRate(float value, float min, float max) {
        if (max <= min) {
            return 0;
        }
        float v = Clamp(value, min, max) - min;
        return v / (max - min);
    }

    std::vector<uint8_t> m_sendData[2];
    std::queue<uint8_t> m_recvData;
    int32_t m_sendCount = 0;
};
