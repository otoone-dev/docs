#pragma once
#include <DeviceBase.h>
#include <Parameters.h>
#include <InputDevices/InputDeviceBase.h>

class MenuBase : public DeviceBase {
public:
    MenuBase() = default;
    virtual ~MenuBase() = default;

    virtual void Update(Parameters& parameters, Keys& keys) = 0;

protected:
    //--------------
    std::string TransposeToStr(int32_t transpose) const {
        const char* TransposeTable[12] = {
            "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B",
        };
        int i = (transpose + 120) % 12;
        int j = (transpose + 120) / 12 - 10;
        std::string ret = std::string(TransposeTable[i]);
        while (j < 0) {
            ret += "-";
            j++;
        }
        while (j > 0) {
            ret += "+";
            j--;
        }
        return ret;
    }

    //-------------
    std::string Format(const std::string& str, float f, int32_t digit) {
        char s[128];
        switch (digit) {
            default:
                sprintf(s, "%s%1.0f", str.c_str(), f); break;
            case 1:
                sprintf(s, "%s%1.1f", str.c_str(), f); break;
            case 2:
                sprintf(s, "%s%1.2f", str.c_str(), f); break;
            case 3:
                sprintf(s, "%s%1.3f", str.c_str(), f); break;
        }
        return s;
    }

    //-------------
    std::string Format(const std::string& str, int i) {
        char s[128];
        sprintf(s, "%s%d", str.c_str(), i);
        return s;
    }
};
