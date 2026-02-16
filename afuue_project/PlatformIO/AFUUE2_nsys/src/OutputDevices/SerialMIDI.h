#pragma once
#include "MIDIBase.h"

class SerialMIDI : public MIDIBase {
public:
    SerialMIDI(gpio_num_t outPin, gpio_num_t inPin)
     : m_outPin(outPin)
     , m_inPin(inPin)
    {}

    const char* GetName() const override { return "MIDIout"; }

    InitializeResult Initialize(Parameters& params) override {
        InitializeResult result;
        Serial2.begin(31250, SERIAL_8N1, m_inPin, m_outPin);
        return result;
    }

    OutputResult Update(Parameters& params, Message& msg) override {
        if (params.IsMidiEnabled()) {
            MidiUpdate(params, msg);
            msg.volume = 0.0f; // 後続のスピーカーに渡さない
        }
        return OutputResult{ false, 0.0f };
    }

protected:
    void SendData(const uint8_t* buff, size_t size) override {
        Serial2.write(buff, size);
    }

private:
    gpio_num_t m_outPin;
    gpio_num_t m_inPin;
};
