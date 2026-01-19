#pragma once
#include <DeviceBase.h>
#include <Parameters.h>
#include <InputDevices/InputDeviceBase.h>

class MenuBase : public DeviceBase {
public:
    MenuBase() = default;
    virtual ~MenuBase() = default;

    virtual void Update(Parameters& parameters, Keys& keys) = 0;
};
