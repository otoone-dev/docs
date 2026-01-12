#pragma once
#include <Arduino.h>
#include "OutputDeviceBase.h"

#define LEDPIN (8)

class LED : public OutputDeviceBase {
public:
    const char* GetName() const override { return "LED"; }

    InitializeResult Initialize() override {
        InitializeResult result;
        pinMode(LEDPIN, OUTPUT);
        ledcAttach(LEDPIN, 156250, 8); // PWM 156,250Hz, 8Bit(256段階)
        ledcWrite(LEDPIN, 0);
        return result;
    }

    OutputResult Update(const Parameters& parameters, Message& msg) override {
        ledcWrite(LEDPIN, (int)(1 + msg.volume * 250.0f));
        return OutputResult{ false, 0.0f };
    }
};
