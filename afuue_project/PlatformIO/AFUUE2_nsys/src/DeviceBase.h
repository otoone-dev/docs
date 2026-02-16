#pragma once
#include <Parameters.h>
#include <string>

struct InitializeResult {
    bool success = true;
    bool skipAfter = false;
    std::string errorMessage = "";
};

class DeviceBase {
public:
    virtual const char* GetName() const = 0;
    virtual InitializeResult Initialize(Parameters& params) = 0;
};
