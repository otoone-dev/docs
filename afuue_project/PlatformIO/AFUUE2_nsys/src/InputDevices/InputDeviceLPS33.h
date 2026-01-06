#pragma once
#include <Wire.h>
#include "InputDeviceBase.h"

#define LPS33_ADDR (0x5C)
#define LPS33_WHOAMI_REG (0x0F)
#define LPS33_WHOAMI_RET (0xB1)
#define LPS33_REG_CTRL (0x10)
#define LPS33_REG_OUT_XL (0x28)

class InputDeviceLPS33 : public InputDeviceBase {
public:
    //--------------
    InputDeviceLPS33(TwoWire &wire, int side) : m_wire(wire), m_address(LPS33_ADDR + side) {}

    //--------------
    const char* GetName() const override {
        return "LPS33";
    }

    //--------------
    InitializeResult Initialize() override {
        InitializeResult result;
        Wire.beginTransmission(LPS33_ADDR);
        if (Wire.endTransmission() != 0) {
            result.success = false;
            result.errorMessage = "NO LPS33";
            return result;
        }
        m_wire.beginTransmission(LPS33_ADDR);
        m_wire.write(LPS33_WHOAMI_REG);
        m_wire.endTransmission();
        m_wire.requestFrom(LPS33_ADDR, 1);
        uint8_t whoami = m_wire.read();
        if (whoami != LPS33_WHOAMI_RET) {
            result.success = false;
            result.errorMessage = "LPS33 ERR";
            return result;
        }
        m_wire.beginTransmission(LPS33_ADDR);
        m_wire.write(LPS33_REG_CTRL);
        m_wire.write(0x50); // odr (0x50=75Hz, 0x40=50Hz, 0x30=25Hz, 0x20=10Hz, 0x10=1Hz, 0x00=OFF)
        m_wire.endTransmission();
        delay(100);
        m_defaultPressure = GetPressure() + 50.0f;
        m_currentPressure = m_defaultPressure;

        return result;
    }

    //--------------
    InputResult Update(const Parameters& parameters) override {
        InputResult result;

        m_currentPressure += (GetPressure() - m_currentPressure) * 0.8f;
        float v = (m_currentPressure - m_defaultPressure) / 400.0f;
        if (v < 0.0f) {
            v = 0.0f;
        }
        else if (v > 1.0f) {
            v = 1.0f;
        }
        result.SetPressure(v);
        return result;
    }
private:
    TwoWire &m_wire;
    int m_address;
    float m_defaultPressure = 0.0f;
    float m_currentPressure = 0.0f;

    //--------------
    float GetPressure() {
        Wire.beginTransmission(m_address);
        Wire.write(LPS33_REG_OUT_XL);
        Wire.endTransmission();
        Wire.requestFrom(m_address, 3);

        uint8_t data[3];
        for (int i = 0; i < 3; i++) {
        data[i] = Wire.read();
        }        
        uint32_t pressure = ((((uint32_t)data[2]) << 16) | (((uint32_t)data[1]) << 8) | data[0]);
        return pressure / 256.0f;
    }
};
