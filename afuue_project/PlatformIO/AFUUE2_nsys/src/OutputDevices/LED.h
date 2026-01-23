#pragma once
#include <Arduino.h>
#include "OutputDeviceBase.h"

class LED : public OutputDeviceBase {
public:
    LED(gpio_num_t ledPin) : m_ledPin(ledPin) {}

    const char* GetName() const override { return "LED"; }

    InitializeResult Initialize() override {
        InitializeResult result;
        pinMode(m_ledPin, OUTPUT);
        ledcAttach(m_ledPin, 156250, 8); // PWM 156,250Hz, 8Bit(256段階)
        ledcWrite(m_ledPin, 0);
        return result;
    }

    OutputResult Update(Parameters& parameters, Message& msg) override {
        ledcWrite(m_ledPin, (int)(1 + msg.volume * 250.0f));
        return OutputResult{ false, 0.0f };
    }
private:
    gpio_num_t m_ledPin;
};
