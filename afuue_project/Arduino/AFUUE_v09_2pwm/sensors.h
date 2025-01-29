#pragma once

#include "afuue_common.h"

class Sensors {
public:
  Sensors();
  bool Initialize();
  void Update();
  void UpdateAcc();
  void BendExec(float td, float vol, bool bendKeysDown);
  float GetBlowPower() const;

  float accx, accy, accz;
  float blowPower;
  float bendNoteShift = 0.0f;
  float breathSenseRate = 300.0f;
  float breathZero = 0.0f;
  bool isLipSensorEnabled = false;

private:
  int defaultPressureValue = 0;
  int defaultPressureValue2 = 0;
  int pressureValue = 0;
  int pressureValue2 = 0;
  int bendCounter = 0;
  const float BENDRATE = -1.0f;
  const float BENDDOWNTIME_LENGTH = 100.0f; //ms
  float bendDownTime = 0.0f;
  float bendVolume = 0.0f;

#ifdef ENABLE_MCP3425
  bool InitPressureMCP3425();
#endif
#ifdef ENABLE_LPS33
  bool InitPressureLPS33(int side);
  int32_t GetPressureValueLPS33(int side);
#endif

#ifdef ENABLE_ADC
  int GetPressureValueADC(int index);
#endif
  int GetPressureValue(int index);
};

