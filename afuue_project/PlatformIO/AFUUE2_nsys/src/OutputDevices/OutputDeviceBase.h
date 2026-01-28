#pragma once
#include <DeviceBase.h>
#include <Parameters.h>

struct OutputResult {
    bool hasCpuLoad = false;
    float cpuLoad = 0.0f;
};

class OutputDeviceBase : public DeviceBase {
public:
    virtual OutputResult Update(Parameters& parameters, Message& msg) = 0;
    virtual ~OutputDeviceBase() {}
};
