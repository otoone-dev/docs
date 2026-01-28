#pragma once
#include <Wire.h>
#include "Key.h"

#define MCP23017_ADDR (0x20)
#define MCP23017_IODIRA (0x00)
#define MCP23017_IODIRB (0x01)
#define MCP23017_GPPUA (0x0C)
#define MCP23017_GPPUB (0x0D)
#define MCP23017_GPIOA (0x12)
#define MCP23017_GPIOB (0x13)

class KeyMCP23017 : public KeyInputBase {
public:
    //--------------
    KeyMCP23017(TwoWire &wire)
     : m_wire(wire) {}

    //--------------
    const char* GetName() const override {
        return "MCP23017";
    }

    //--------------
    InitializeResult Initialize() override {
        InitializeResult result;
        m_wire.beginTransmission(MCP23017_ADDR);
        if (m_wire.endTransmission() != 0) {
            result.success = false;
            result.errorMessage = "NO IOEXP";
            return result;
        }

        m_wire.beginTransmission(MCP23017_ADDR);
        m_wire.write(MCP23017_IODIRA);
        m_wire.write(0xFF); // input
        m_wire.endTransmission();

        m_wire.beginTransmission(MCP23017_ADDR);
        m_wire.write(MCP23017_IODIRB);
        m_wire.write(0xFF); // input
        m_wire.endTransmission();

        m_wire.beginTransmission(MCP23017_ADDR);
        m_wire.write(MCP23017_GPPUA);
        m_wire.write(0xFF); // pull-up
        m_wire.endTransmission();

        m_wire.beginTransmission(MCP23017_ADDR);
        m_wire.write(MCP23017_GPPUB);
        m_wire.write(0xFF); // pull-up
        m_wire.endTransmission();
        return result;
    }

protected:
    //--------------
    uint16_t GetKeyData() {
        m_wire.beginTransmission(MCP23017_ADDR);
        m_wire.write(MCP23017_GPIOA);
        m_wire.endTransmission();
        m_wire.requestFrom(MCP23017_ADDR, 1);
        uint8_t gpioa = m_wire.read();

        m_wire.beginTransmission(MCP23017_ADDR);
        m_wire.write(MCP23017_GPIOB);
        m_wire.endTransmission();
        m_wire.requestFrom(MCP23017_ADDR, 1);
        uint8_t gpiob = m_wire.read();

        uint16_t d = (gpiob << 8) | gpioa;
        return ~d; // push で LOW(0) なので反転
    }

private:
    TwoWire &m_wire;
};
