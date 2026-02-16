#pragma once
#include <DeviceBase.h>
#include <functional>

//-------------
class InputDeviceBase : public DeviceBase {
public:
    InputDeviceBase() = default;
    virtual ~InputDeviceBase() = default;
    virtual bool Update(Parameters& parameters, Message& message) = 0;
};
