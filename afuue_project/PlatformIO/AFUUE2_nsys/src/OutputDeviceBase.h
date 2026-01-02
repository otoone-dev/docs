#pragma once
#include "DeviceBase.h"

struct OutputResult {
    bool hasCpuLoad = false;
    float cpuLoad = 0.0f;
};

class OutputDeviceBase : public DeviceBase {
public:
    virtual OutputResult Update(float note, float volume) = 0;
    virtual ~OutputDeviceBase() {}
};
