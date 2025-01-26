#include "afuue_common.h"
#include "sensors.h"

//---------------------------------
Sensors::Sensors() {}

//---------------------------------
// 初期化
bool Sensors::Initialize() {
#ifdef ENABLE_MCP3425
  bool mcp3425OK = InitPressureMCP3425();
  if (!mcp3425OK) {
    return false;
  }
#endif

#ifdef ENABLE_LPS33
  bool lps33OK = InitPressureLPS33(0);
#ifdef ENABLE_LIP
  lps33OK &= InitPressureLPS33(1);
#endif
  if (!lps33OK) {
    return false;
  }
#endif

#ifdef ENABLE_ADC
  pinMode(ADCPIN, INPUT);
#ifdef ENABLE_LIP
  pinMode(ADCPIN2, INPUT);
#endif
#endif

#ifdef _M5STICKC_H_
  gpio_pulldown_dis(GPIO_NUM_25);
  gpio_pullup_dis(GPIO_NUM_25);
#endif
  analogSetAttenuation(ADC_0db);
  analogReadResolution(12); // 4096
#ifdef _STAMPS3_H_
  adc2_config_channel_atten(ADC2_CHANNEL_0, ADC_ATTEN_DB_0);
  adc2_config_channel_atten(ADC2_CHANNEL_1, ADC_ATTEN_DB_0);
#endif

  int pressure = 0;
  int lowestPressure = 0;
#ifdef ENABLE_LPS33
  lowestPressure = 2000;
  delay(1000);
  defaultPressureValue = GetPressureValue(0) + lowestPressure;
#ifdef ENABLE_LIP
  defaultPressureValue2 = GetPressureValue(1) + lowestPressure;
#endif
#else
  for (int i = 0; i < 10; i++) {
    pressure += GetPressureValue(0);
    delay(30);
  }
  defaultPressureValue = (pressure / 10) + lowestPressure;
#ifdef ENABLE_LIP
  pressure = 0;
  for (int i = 0; i < 10; i++) {
    pressure += GetPressureValue(1);
    delay(30);
  }
  defaultPressureValue2 = (pressure / 10) + lowestPressure;
#endif
#endif //LPS33
  return true;
}

//---------------------------------
// センサー更新
void Sensors::Update() {
    pressureValue = GetPressureValue(0);
#ifdef ENABLE_LIP
    if (isLipSensorEnabled) {
      //気圧センサー (ピッチベンド用)
      pressureValue2 = GetPressureValue(1);
    }
#endif
    int defPressure = defaultPressureValue + breathZero;
#ifdef ENABLE_MCP3425
    float vol = (pressureValue - defPressure) / ((2047.0f-breathSenseRate)-defPressure); // 0 - 1
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    vol = pow(vol,2.0f) * (1.0f-bendVolume);
#endif
#ifdef ENABLE_LPS33
    float vol = (pressureValue - defPressure) / 70000.0f; // 0 - 1
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    vol = pow(vol,2.0f);
#endif
#ifdef ENABLE_ADC
    float vol = (pressureValue - defPressure) / breathSenseRate; // 0 - 1
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    vol = pow(vol,2.0f);
#endif
}

//---------------------------------
// ベンド処理
void Sensors::BendExec(float td, float vol, bool bendKeysDown) {
  #ifdef ENABLE_LIP
      if (isLipSensorEnabled) {
        float b2 = (pressureValue2 - (defaultPressureValue2 + breathZero)) / breathSenseRate;
        if (b2 < 0.0f) b2 = 0.0f;
        if (b2 > vol) b2 = vol;
        float bendNoteShiftTarget = 0.0f;
        if (vol > 0.0001f) {
          bendNoteShiftTarget = -1.0f + ((vol - b2) / vol)*1.2f;
          if (bendNoteShiftTarget > 0.0f) {
            bendNoteShiftTarget = 0.0f;
          }
        }
        else {
          bendNoteShiftTarget = 0.0f;
        }
        bendNoteShift += (bendNoteShiftTarget - bendNoteShift) * 0.5f;
      }
  #else
    // LowC + Eb down -------
    if (bendKeysDown) {
    //if ((keyLowC == LOW) && (keyEb == LOW)) {
      bendDownTime += td;
      if (bendDownTime > BENDDOWNTIME_LENGTH) {
        bendDownTime = BENDDOWNTIME_LENGTH;
      }
    }
    else {
      if (bendDownTime > 0.0f) {
        bendDownTime = -bendDownTime;
      }
      else {
        bendDownTime += td;
        if (bendDownTime > 0.0f) {
          bendDownTime = 0.0f;
        }
      }
    }
    float bend = 0.0f;
    if (bendDownTime > 0.0f) {
      bend = bendDownTime / BENDDOWNTIME_LENGTH;
      bendVolume = bend;
    }
    else if (bendDownTime < 0.0f) {
      bend = -(bendDownTime / BENDDOWNTIME_LENGTH);
      bendVolume = bend;
    }
    bendNoteShift = bend * BENDRATE;
  #endif
}

//---------------------------------
float Sensors::GetBlowPower() const {
    return vol;
}

//-------------------------------------
// 加速度センサー更新
void Sensors:: UpdateAcc() {
    float ax = 0.0f;
    float ay = 0.0f;
    float az = 1.0f;
  #ifdef _M5STICKC_H_
    //M5.IMU.getAccelData(&ax,&ay,&az);
  #endif
    accx += (ax - accx) * 0.3f;
    accy += (ay - accy) * 0.3f;
    accz += (az - accz) * 0.3f;
}

//-------------------------------------
// MCP3425 初期化
#ifdef ENABLE_MCP3425
bool Sensors::InitPressureMCP3425() {
  Wire.beginTransmission(MCP3425_ADDR);
  int error = Wire.endTransmission();
  if (error != 0) {
    return false;
  }
  Wire.beginTransmission(MCP3425_ADDR);
  //Wire.write(0b10010111); // 0,1:Gain(1x,2x,4x,8x) 2,3:SampleRate(0:12bit,1:14bit,2:16bit) 4:Continuous 5,6:NotDefined 7:Ready
  Wire.write(0b10010011); // 0,1:Gain(1x,2x,4x,8x) 2,3:SampleRate(0:12bit,1:14bit,2:16bit) 4:Continuous 5,6:NotDefined 7:Ready
  Wire.endTransmission();
  return true;
}
#endif
#ifdef ENABLE_LPS33

//-------------------------------------
// LPS33 初期化    todo:アドレス違いに対応すべし
bool Sensors::InitPressureLPS33(int side) {
  return BARO.begin();
}

//-------------------------------------
// LPS33 から値取得    todo:アドレス違いに対応すべし
int32_t Sensors::GetPressureValueLPS33(int side) {
  int d = static_cast<int32_t>(BARO.readPressure() * 40960.0f);
  return d >> 1;
}
#endif

//-------------------------------------
// ADC から値取得
#define PRESSURE_AVERAGE_COUNT (10)
#ifdef ENABLE_ADC
int Sensors::GetPressureValueADC(int index) {
  int averaged = 0;
  int average_count = 0;
  while (average_count == 0) {
    for (int i = 0; i < PRESSURE_AVERAGE_COUNT; i++) {
#ifdef ENABLE_LIP
      if (index == 1) {
        //averaged += analogRead(ADCPIN2);
        int value;
        if (adc2_get_raw(ADC2_CHANNEL_1, ADC_WIDTH_BIT_12, &value) == ESP_OK) {
          averaged += value;
          average_count++;
        }
      }
      else
#endif
      {
        //averaged += analogRead(ADCPIN);
        int value;
        if (adc2_get_raw(ADC2_CHANNEL_0, ADC_WIDTH_BIT_12, &value) == ESP_OK) {
          averaged += value;
          average_count++;
        }
      }
      delayMicroseconds(100);
    }
  }
  return averaged / average_count;//PRESSURE_AVERAGE_COUNT;
}
#endif

//-------------------------------------
// 気圧センサーの値取得窓口
int Sensors::GetPressureValue(int index) {
#ifdef ENABLE_LIP
  // ADC x 2
#ifdef ENABLE_LPS33
    return GetPressureValueLPS33(index);
#else
    return GetPressureValueADC(index);
#endif
#else
  // ADC x 1
#ifdef ENABLE_MCP3425
    Wire.requestFrom(MCP3425_ADDR, 2);
    return (Wire.read() << 8 ) | Wire.read();
#endif
#ifdef ENABLE_LPS33
    return GetPressureValueLPS33(0);
#endif
#ifdef ENABLE_ADC
    return GetPressureValueADC(0);
#endif
  // ----
#endif // LIP
}
