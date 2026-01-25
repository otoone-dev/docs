#pragma once
#include <DeviceBase.h>
#include <Parameters.h>
#include <functional>

struct Keys {
private:
    bool GetFlag(bool getClicked, uint16_t bit) const {
        return (getClicked ? (clicked & bit) : (pressed & bit));
    }
public:
    uint16_t pressed;
    uint16_t clicked;
    Keys(uint16_t d = 0)
        : pressed(d)
        , clicked(d) {
    }
    void Update(uint16_t d) {
        clicked = (d ^ pressed) & d;
        pressed = d;
    }

    bool KeyLowC(bool getClicked=false) const { return GetFlag(getClicked, 0x0001); }
    bool KeyEb(bool getClicked=false) const { return GetFlag(getClicked, 0x0002); }
    bool KeyD(bool getClicked=false) const { return GetFlag(getClicked, 0x0004); }
    bool KeyE(bool getClicked=false) const { return GetFlag(getClicked, 0x0008); }
    bool KeyF(bool getClicked=false) const { return GetFlag(getClicked, 0x0010); }
    bool KeyLowCs(bool getClicked=false) const { return GetFlag(getClicked, 0x0020); }
    bool KeyGs(bool getClicked=false) const { return GetFlag(getClicked, 0x0040); }
    bool KeyG(bool getClicked=false) const { return GetFlag(getClicked, 0x0080); }
    bool KeyA(bool getClicked=false) const { return GetFlag(getClicked, 0x0100); }
    bool KeyB(bool getClicked=false) const { return GetFlag(getClicked, 0x0200); }
    bool KeyUp(bool getClicked=false) const { return GetFlag(getClicked, 0x0400); }
    bool KeyDown(bool getClicked=false) const { return GetFlag(getClicked, 0x0800); }
    float GetNote(float baseNote) const {
        int b = 0;
        if (KeyLowC()) b |= (1 << 7);
        if (KeyD()) b |= (1 << 6);
        if (KeyE()) b |= (1 << 5);
        if (KeyF()) b |= (1 << 4);
        if (KeyG()) b |= (1 << 3);
        if (KeyA()) b |= (1 << 2);
        if (KeyB()) b |= (1 << 1);

        float n = 0.0f;

        // 0:C 2:D 4:E 5:F 7:G 9:A 11:B 12:HiC
        int g = (b & 0b00001110);  // keyG, keyA, keyB の組み合わせ
        
        if ((g == 0b00001110) || (g == 0b00001100)) {      // [*][*][*] or [*][*][ ]
            if ((b & 0b11110000) == 0b11110000) {
                n = 0.0f; // C
                if (KeyEb()) n = 2.0f; // D
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

        if ((KeyEb())&&(!KeyLowC())) n += 1.0f; // #
        if (KeyGs()) n += 1.0f; // #
        if (KeyLowCs()) n -= 1.0f; // b

        if (KeyUp()) n += 12.0f;
        else if (KeyDown()) n -= 12.0f;

        return baseNote + n;
    }
};

/*
struct InputResult {
    bool success = true;
    bool hasVolume = false;
    bool hasNote = false;
    bool hasBend = false;
    bool hasKey = false;

    Message message;

    std::string errorMessage = "";
    //--------------
    void SetVolume(float p) {
        hasVolume = true;
        message.volume = p;
    }
    //--------------
    void SetNote(float n) {
        hasNote = true;
        message.note = n;
    }
    //--------------
    void SetBend(float b) {
        hasBend = true;
        message.bend = b;
    }
    //--------------
    void SetKeyData(uint16_t data) {
        hasKey = true;
        message.keyData = data;
    }
};
*/

class InputDeviceBase : public DeviceBase {
public:
    InputDeviceBase() = default;
    virtual ~InputDeviceBase() = default;
    virtual bool Update(Parameters& parameters, Message& message) = 0;
};
