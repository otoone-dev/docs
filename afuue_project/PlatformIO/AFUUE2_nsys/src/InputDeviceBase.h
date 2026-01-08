#pragma once
#include "DeviceBase.h"
#include "Parameters.h"
#include <functional>

struct InputResult {
    bool success = true;
    bool hasVolume = false;
    bool hasNote = false;
    bool hasBend = false;
    bool hasKey = false;

    Message message;

    std::string errorMessage = "";
#ifdef DEBUG
    std::string debugMessage = "";
#endif
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

class InputDeviceBase : public DeviceBase {
public:
  virtual ~InputDeviceBase() = default;
  virtual InputResult Update(const Parameters& parameters) = 0;
};
