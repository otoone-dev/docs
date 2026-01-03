#pragma once
#include <Arduino.h>
#include "OutputDeviceBase.h"

#define LEDPIN (8)

class OutputDeviceLED : public OutputDeviceBase {
public:
    const char* GetName() const override { return "LED"; }

    InitializeResult Initialize() override {
        InitializeResult result;
        pinMode(LEDPIN, OUTPUT);
        ledcAttachChannel(LEDPIN, 156250, 8, 5); // PWM 156,250Hz, 8Bit(256段階)
        ledcWriteChannel(5, 0);
        return result;
    }

    OutputResult Update(float note, float volume) override {
        ledcWriteChannel(5, (int)(1 + volume * 250.0f));
        return OutputResult{ false, 0.0f };
    }
};
