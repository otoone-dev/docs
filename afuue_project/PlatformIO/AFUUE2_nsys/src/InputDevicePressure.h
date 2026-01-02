#pragma once
#include <Wire.h>
#include "InputDeviceBase.h"
#include "InputDevices/InputDeviceLPS33.h"

class InputDevicePressure : public InputDeviceBase {
public:
    InputDevicePressure(TwoWire &wire)
        : InputDeviceBase(), m_wire(wire), m_lps33(wire, 0) {}

    //--------------
    const char* GetName() const override {
        return "Pressure";
    }
    //--------------
    InitializeResult Initialize() override {
        return m_lps33.Initialize();
    }

    //--------------
    InputResult Update() override {
        auto result = m_lps33.Update();
        if (!result.success) {
            result.errorMessage = "PRESSURE ERR";
            return result;
        }
        float v = result.pressure;
        result.pressure = v * v;
        return result;
    }

private:
    TwoWire &m_wire;
    InputDeviceLPS33 m_lps33;
};
