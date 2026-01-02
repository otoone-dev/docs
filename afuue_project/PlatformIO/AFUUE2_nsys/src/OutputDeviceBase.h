#pragma once
#include "DeviceBase.h"

class OutputDeviceBase : public DeviceBase {
public:
    virtual void Update(float note, float volume) = 0;
    virtual ~OutputDeviceBase() {}
};
