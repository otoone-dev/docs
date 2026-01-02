#pragma once
#include <string>

#define CORE0 (0)
#define CORE1 (1)

struct InitializeResult {
    bool success = true;
    std::string errorMessage = "";
};

class DeviceBase {
public:
    virtual const char* GetName() const = 0;
    virtual InitializeResult Initialize() = 0;
};
