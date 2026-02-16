#pragma once
#include <Wire.h>
#include "Key.h"

class KeyDigitalAFUUE2R : public KeyInputBase {
public:
    //--------------
    const char* GetName() const override {
        return "KeySwitches";
    }

    //--------------
    InitializeResult Initialize(Parameters& params) override {
        InitializeResult result;
        const int keyPortList[14] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 ,9, 10, 44, 46 };
        for (int i = 0; i < 14; i++) {
            pinMode(keyPortList[i], INPUT_PULLUP); // INPUT FOR SWITCHES
        }
        return result;
    }

protected:
    //--------------
    uint16_t GetKeyData() override {
        bool keyLowC = digitalRead(1);
        bool keyEb = digitalRead(2);
        bool keyD = digitalRead(3);
        bool keyE = digitalRead(4);
        bool keyF = digitalRead(5);
        bool keyLowCs = digitalRead(6);
        bool keyGs =digitalRead(7);
        bool keyG = digitalRead(8);
        bool keyA = digitalRead(9);
        bool keyB = digitalRead(10);
        bool octUp = digitalRead(44);
        bool octDown = digitalRead(46);
        uint16_t d = 0;
        d |= ((keyLowC == LOW) ? 0x0001 : 0x0000);
        d |= ((keyEb == LOW) ? 0x0002 : 0x0000);
        d |= ((keyD == LOW) ? 0x0004 : 0x0000);
        d |= ((keyE == LOW) ? 0x0008 : 0x0000);
        d |= ((keyF == LOW) ? 0x0010 : 0x0000);
        d |= ((keyLowCs == LOW) ? 0x0020 : 0x0000);
        d |= ((keyGs == LOW) ? 0x0040 : 0x0000);
        d |= ((keyG == LOW) ? 0x0080 : 0x0000);
        d |= ((keyA == LOW) ? 0x0100 : 0x0000);
        d |= ((keyB == LOW) ? 0x0200 : 0x0000);
        d |= ((octDown == LOW) ? 0x0400 : 0x0000);
        d |= ((octUp == LOW) ? 0x0800 : 0x0000);
        return d;
    }
};
