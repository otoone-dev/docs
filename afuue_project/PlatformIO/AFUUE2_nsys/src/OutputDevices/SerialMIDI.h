#pragma once
#include "MIDIBase.h"

class SerialMIDI : public MIDIBase {
public:
    SerialMIDI(gpio_num_t outPin, gpio_num_t inPin = (gpio_num_t)-1)
     : m_outPin(outPin)
     , m_inPin(inPin)
    {}

    const char* GetName() const override { return "MIDIout"; }

    InitializeResult Initialize() override {
        InitializeResult result;
        Serial2.begin(31250, SERIAL_8N1, m_outPin, m_inPin);
        return result;
    }

    OutputResult Update(Parameters& params, Message& msg) override {
        MidiUpdate(params, msg);
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
