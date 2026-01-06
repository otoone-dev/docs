#pragma once
#include "InputDeviceBase.h"
#include "InputDevices/InputDeviceMCP23017.h"

class InputDeviceKey : public InputDeviceBase {
public:
    InputDeviceKey(TwoWire &wire)
        : InputDeviceBase(), m_mcp23017(wire) {}

    //--------------
    const char* GetName() const override {
        return "Key";
    }

    //--------------
    InitializeResult Initialize() override {
        m_lastChangeTime = micros();
        return m_mcp23017.Initialize();
    }

    //--------------
    InputResult Update(const Parameters& parameters) override {
        auto result = m_mcp23017.Update(parameters);
        if (!result.hasKey) {
            result.success = false;
            result.errorMessage = "KEY ERR";
            return result;
        }
        uint64_t t = micros();
        const uint16_t keyData = result.keyData;
        if (keyData != m_lastKeyData) {
            m_lastKeyData = keyData;
            m_lastChangeTime = t;
        }
        const bool keyLowC = ((keyData & 0x0001) != 0);
        const bool keyEb = ((keyData & 0x0002) != 0);
        const bool keyD = ((keyData & 0x0004) != 0);
        const bool keyE = ((keyData & 0x0008) != 0);
        const bool keyF = ((keyData & 0x0010) != 0);
        const bool keyLowCs = ((keyData & 0x0020) != 0);
        const bool keyGs = ((keyData & 0x0040) != 0);
        const bool keyG = ((keyData & 0x0080) != 0);
        const bool keyA = ((keyData & 0x0100) != 0);
        const bool keyB = ((keyData & 0x0200) != 0);
        const bool octUp = ((keyData & 0x0400) != 0);
        const bool octDown = ((keyData & 0x0800) != 0);

        int b = 0;
        if (keyLowC == LOW) b |= (1 << 7);
        if (keyD == LOW) b |= (1 << 6);
        if (keyE == LOW) b |= (1 << 5);
        if (keyF == LOW) b |= (1 << 4);
        if (keyG == LOW) b |= (1 << 3);
        if (keyA == LOW) b |= (1 << 2);
        if (keyB == LOW) b |= (1 << 1);

        float n = 0.0f;

        // 0:C 2:D 4:E 5:F 7:G 9:A 11:B 12:HiC
        int g = (b & 0b00001110);  // keyG, keyA, keyB の組み合わせ
        
        if ((g == 0b00001110) || (g == 0b00001100)) {      // [*][*][*] or [*][*][ ]
            if ((b & 0b11110000) == 0b11110000) {
                n = 0.0f; // C
                if (keyEb == LOW) n = 2.0f; // D
            }
            else if ((b & 0b11110000) == 0b01110000) {
                n = 2.0f; // D
            }
            else if ((b & 0b01110000) == 0b00110000) {
                n = 4.0f; // E
                //if (b & 0b10000000) n -= 1.0f;
            }
            else if ((b & 0b00110000) == 0b00010000) {
                n = 5.0f; // F
                if (b & 0b10000000) n -= 2.0f;
                //if (b & 0b01000000) n -= 1.0f;
            }
            else if ((b & 0b00010000) == 0) {
                n = 7.0f; // G
            if (b & 0b10000000) n -= 2.0f;
                //if (b & 0b01000000) n -= 1.0f;
                if (b & 0b00100000) n -= 1.0f;
            }
            if ((b & 0b00000010) == 0) {
                n += 12.0f;
            }
        }
        else if (g == 0b00000110) { // [ ][*][*]
            n = 9.0f; // A
            if (b & 0b10000000) n -= 2.0f;
            //if (b & 0b01000000) n -= 1.0f;
            if (b & 0b00100000) n -= 1.0f;
            if (b & 0b00010000) n += 1.0f; // SideBb 相当
        }
        else if (g == 0b00000010) { // [ ][ ][*]
            n = 11.0f; // B
            if (b & 0b10000000) n -= 2.0f;
            //if (b & 0b01000000) n -= 1.0f;
            if (b & 0b00100000) n -= 1.0f; // Flute Bb
            if (b & 0b00010000) n -= 1.0f;
        }
        else if (g == 0b00000100) { // [ ][*][ ]
            n = 12.0f; // HiC
            if (b & 0b10000000) n -= 2.0f;
            if (b & 0b01000000) n += 2.0f; // HiD トリル用
            if (b & 0b00100000) n -= 1.0f;
            if (b & 0b00010000) n -= 1.0f;
        }
        else if (g == 0b00001000) { // [*][ ][ ]
            n = 20.0f; // HiA
            if (b & 0b10000000) n -= 2.0f;
            if (b & 0b01000000) n += 2.0f;
            if (b & 0b00100000) n -= 1.0f;
            if (b & 0b00010000) n -= 1.0f;
        }
        else if (g == 0b00001010) { // [*][ ][*]
            n = 10.0f; // A#
            if (b & 0b10000000) n -= 2.0f;
            if (b & 0b01000000) n += 2.0f;
            if (b & 0b00100000) n -= 1.0f;
            if (b & 0b00010000) n -= 1.0f;
        }
        else if (g == 0b00000000) { // [ ][ ][ ]
            n = 13.0f; // HiC#
            //if (b & 0b10000000) n -= 0.5f;
            if (b & 0b01000000) n += 2.0f; // HiEb トリル用
            if (b & 0b00100000) n -= 1.0f;
            if (b & 0b00010000) n -= 1.0f;
        }

        if ((keyEb == LOW)&&(keyLowC != LOW)) n += 1.0f; // #
        if (keyGs == LOW) n += 1.0f; // #
        if (keyLowCs == LOW) n -= 1.0f; // b

        if (octUp == LOW) n += 12.0f;
        else if (octDown == LOW) n -= 12.0f;

        if (t - m_lastChangeTime > 20*1000) { // ピロ音防止
            m_targetNote = m_baseNote + n;
        }
        m_currentNote += (m_targetNote - m_currentNote) * m_rate;
        result.SetNote(m_currentNote);
        return result;
    }

private:
    InputDeviceMCP23017 m_mcp23017;

    uint64_t m_lastChangeTime = 0;
    float m_targetNote = 0.0f;
    float m_currentNote = 0.0f;
    float m_rate = 0.9f;
    int m_baseNote = 48; // C
    uint16_t m_lastKeyData = 0;
};
