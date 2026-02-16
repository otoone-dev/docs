#pragma once
#include <Wire.h>
#include "InputDeviceBase.h"
#include <string>
#include <format>

#define LPS33_ADDR (0x5C)
#define LPS33_WHOAMI_REG (0x0F)
#define LPS33_WHOAMI_RET (0xB1)
#define LPS33_REG_CTRL (0x10)
#define LPS33_REG_OUT_XL (0x28)

class PressureLPS33 : public InputDeviceBase {
public:
    enum class ReadType : uint8_t {
        BREATH, // addr=0x5C
        BREATH_AND_BEND,   // addr=0x5D
    };

    //--------------
    PressureLPS33(TwoWire &wire, ReadType readType) 
        : m_wire(wire)
        , m_address(LPS33_ADDR)
        , m_readType(readType) {}

    //--------------
    const char* GetName() const override {
        return "LPS33";
    }

    //--------------
    InitializeResult Initialize(Parameters& params) override {
        InitializeResult result;
        delay(100);
        result = StartDevice(m_address); // BREATH
        if (result.success && m_readType == ReadType::BREATH_AND_BEND) {
            delay(10);
            result = StartDevice(m_address + 1); // BEND
        }
        delay(100);
        if (result.success) {
            m_defaultPressure = GetPressure(m_address) + 50.0f;
            m_currentPressure = m_defaultPressure;
            if (m_readType == ReadType::BREATH_AND_BEND) {
                delay(10);
                m_defaultBendPressure = GetPressure(m_address + 1) + 50.0f;
                m_currentBendPressure = m_defaultBendPressure;
            }
        }
        return result;
    }

    //--------------
    bool Update(Parameters& params, Message& message) override {
        m_currentPressure += (GetPressure(m_address) - m_currentPressure) * params.breathDelay;
        float p = (m_currentPressure - m_defaultPressure) * 0.003f;
        float v = Clamp(p, 0.0f, 1.0f);
        message.volume = v * v;
        if (m_readType == ReadType::BREATH_AND_BEND && params.IsBendEnabled()) {
            m_currentBendPressure += (GetPressure(m_address + 1) - m_currentBendPressure) * params.breathDelay; //ブレスと同じでないとズレる
            float bendNoteShiftTarget = 0.0f;
            if (v > 0.00001f) {
                float pb = (m_currentBendPressure - m_defaultBendPressure)  * 0.003f;
                float vb = Clamp(pb, 0.0f, 1.0f);            
                bendNoteShiftTarget = -1.0f + ((v - vb) / v) * 1.2f;
                if (bendNoteShiftTarget > 0.0f) {
                    bendNoteShiftTarget = 0.0f;
                }
            }
            else {
                bendNoteShiftTarget = 0.0f;
            }
            message.bend = bendNoteShiftTarget;
        }
        else {
            message.bend = 0.0f;
        }
#ifdef DEBUG
        //char s[64];
        //sprintf(s, "%1.1f\n%1.1f\n%1.1fB\n%1.1fB\n%1.1f", m_currentPressure, m_defaultPressure, m_currentBendPressure, m_defaultBendPressure, v);
        //params.debugMessage = s;
#endif
        return true;
    }
private:
    TwoWire &m_wire;
    int m_address;
    float m_defaultPressure = 0.0f;
    float m_currentPressure = 0.0f;
    float m_defaultBendPressure = 0.0f;
    float m_currentBendPressure = 0.0f;
    ReadType m_readType;

    //--------------
    InitializeResult StartDevice(int address) {
        InitializeResult result;
        m_wire.beginTransmission(address);
        if (m_wire.endTransmission() != 0) {
            result.success = false;
            result.errorMessage = std::format("NO LPS33:{0}", address);
            return result;
        }
        m_wire.beginTransmission(address);
        m_wire.write(LPS33_WHOAMI_REG);
        m_wire.endTransmission();
        m_wire.requestFrom(address, 1);
        uint8_t whoami = m_wire.read();
        if (whoami != LPS33_WHOAMI_RET) {
            result.success = false;
            result.errorMessage = std::format("LPS33 ERR:{0}", address);
            return result;
        }
        m_wire.beginTransmission(address);
        m_wire.write(LPS33_REG_CTRL);
        m_wire.write(0x50); // odr (0x50=75Hz, 0x40=50Hz, 0x30=25Hz, 0x20=10Hz, 0x10=1Hz, 0x00=OFF)
        m_wire.endTransmission();
        return result;
    }

    //--------------
    float GetPressure(int address) {
        Wire.beginTransmission(address);
        Wire.write(LPS33_REG_OUT_XL);
        Wire.endTransmission();
        Wire.requestFrom(address, 3);

        uint8_t data[3];
        for (int i = 0; i < 3; i++) {
            data[i] = Wire.read();
        }        
        uint32_t pressure = ((((uint32_t)data[2]) << 16) | (((uint32_t)data[1]) << 8) | data[0]);
        return pressure / 256.0f; // 1/4096 で hPa 単位だが、ESP32のADCに合わせるため 1/256にする。
    }
};
