#pragma once

#include "MenuBase.h"

class MenuForKey : public MenuBase {
public:
    MenuForKey() = default;
    virtual ~MenuForKey() = default;

    //--------------
    const char* GetName() const override {
        return "Key";
    }

    //--------------
    InitializeResult Initialize() override {
        return InitializeResult();
    }
    
    //--------------
    void Update(Parameters& parameters, Keys& keys) override {
        if (!keys.KeyGs() || !keys.KeyLowCs()) {
            return;
        }
        if (keys.KeyF_Clicked()) {
            parameters.waveTableIndex++;
        }

        //後段のメニューにキーを渡さない
        keys.clicked = 0;
    }
};
