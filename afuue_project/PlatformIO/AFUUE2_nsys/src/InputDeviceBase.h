#pragma once
#include <functional>
#include "DeviceBase.h"

struct InputResult {
    bool success = true;
    bool hasPressure = false;
    bool hasNote = false;
    bool hasKey = false;

    float pressure = 0.0f;
    float note = 0.0f;
    uint16_t keyData = 0;

    std::string errorMessage = "";
    std::string debugMessage = "";

    //--------------
    void SetPressure(float p) {
        hasPressure = true;
        pressure = p;
    }
    //--------------
    void SetNote(float n) {
        hasNote = true;
        note = n;
    }
    //--------------
    void SetKeyData(uint16_t data) {
        hasKey = true;
        keyData = data;
    }
};

class InputDeviceBase : public DeviceBase {
public:
  virtual ~InputDeviceBase() = default;
  virtual InputResult Update() = 0;
};
