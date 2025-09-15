#pragma once

#include "afuue_common.h"

class Sensors {
public:
  Sensors();
  int32_t Initialize();
  void Update();
  void UpdateAcc();
  void BendExec(float td, float vol, bool bendKeysDown);
  float GetBlowPower() const;
  float GetPressureValue(int index);

  float accx, accy, accz;
  float blowPower;
  float bendNoteShift = 0.0f;
  float breathSenseRate = 300.0f;
  float breathZero = 0.0f;
  bool isLipSensorEnabled = false;

  uint16_t exKeys = 0;

private:
  int defaultPressureValue = 0;
  int defaultPressureValue2 = 0;
  float pressureValue = 0;
  float pressureValue2 = 0;
  int32_t bendCounter = 0;
  const float BENDRATE = -1.0f;
  const float BENDDOWNTIME_LENGTH = 100.0f; //ms
  float bendDownTime = 0.0f;
  float bendVolume = 0.0f;

#ifdef USE_MCP3425
  bool InitPressureMCP3425();
#endif
#ifdef USE_LPS33
  int32_t InitPressureLPS33(int side);
  int32_t GetPressureValueLPS33(int side);
#endif

#ifdef USE_INTERNALADC
  int32_t GetPressureValueADC(int index);
#endif
};
