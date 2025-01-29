#include "afuue_common.h"
#include "wave_generator.h"
#include <cfloat>

//----------------------------
WaveGenerator::WaveGenerator(volatile WaveInfo* pInfo)
  : m_pInfo(pInfo)
{
  for (int i = 0; i < DELAY_BUFFER_SIZE; i++) {
    delayBuffer[i] = 0.0f;
  }
}

//----------------------------
// 初期化
void WaveGenerator::Initialize(Menu& menu) {
#ifdef SOUND_TWOPWM
  pinMode(PWMPIN_LOW, OUTPUT);
  pinMode(PWMPIN_HIGH, OUTPUT);
  ledcSetup(0, 156250, 8); // PWM 156,250Hz, 8Bit(256段階)
  ledcAttachPin(PWMPIN_LOW, 0);
  ledcWrite(0, 0);
  ledcSetup(1, 156250, 8); // PWM 156,250Hz, 8Bit(256段階)
  ledcAttachPin(PWMPIN_HIGH, 1);
  ledcWrite(1, 0);
#else
  pinMode(DACPIN, OUTPUT);
#endif
  sinTable = menu.waveData.GetSinTable();
  tanhTable = menu.waveData.GetTanhTable();
  currentWaveTable = menu.waveData.GetWaveTable(menu.waveIndex);
}

//----------------------------
// 音程の変更など比較的ゆっくりな処理
void WaveGenerator::Tick(float note) {
  // 現在の再生周波数から１サンプルあたりのフェーズの進みを計算
  float wavelength = CalcFrequency(note);
  currentWavelength += (wavelength - currentWavelength) * m_pInfo->portamentoRate;
  currentWavelengthTickCount = (currentWavelength / SAMPLING_RATE);

  // ローパスパラメータの更新
  if (m_pInfo->lowPassQ > 0.0f) {
    float a = (tanh(m_pInfo->lowPassR*(requestedVolume-m_pInfo->lowPassP)) + 1.0f) * 0.5f;
    float lp = 100.0f + 20000.0f * a;
    if (lp > 12000.0f) {
      lp = 12000.0f;
    }
    lowPassValue += (lp - lowPassValue) * 0.8f;

    float omega = lowPassValue / SAMPLING_RATE;
    float alpha = InteropL(sinTable, 1024, omega) * m_pInfo->lowPassIDQ;
    float cosv = InteropL(sinTable, 1024, omega + 0.25f);
    float one_minus_cosv = 1.0f - cosv;
    lp_a0 =  1.0f + alpha;
    lp_a1 = -2.0f * cosv;
    lp_a2 =  1.0f - alpha;
    lp_b0 = one_minus_cosv * 0.5f;
    lp_b1 = one_minus_cosv;
    lp_b2 = lp_b0;
  }
}

//-------------------------------------
// 波形生成のため高速に呼ばれる処理
void WaveGenerator:: CreateWave(bool enabled) {
  if (!enabled) {
    waveOutH = 0;
    waveOutL = 0;
    return;
  }

  // 波形を生成する
  phase += currentWavelengthTickCount;
  if (phase >= 1.0f) phase -= 1.0f;

  float g = InteropL(currentWaveTable, 256, phase) / 32767.0f;

  //ノイズ
  if (noiseVolume > 0.0f) {
    float n = (static_cast<float>(rand()) / RAND_MAX);
    g = (g * (1.0f-noiseVolume) + n * noiseVolume);
  }

  if (m_pInfo->lowPassQ > 0.0f) {
    g = LowPass(g);
  }

  float e = (g * requestedVolume) + delayBuffer[delayPos];

  if ( (-0.00002f < e) && (e < 0.00002f) ) {
    e = 0.0f;
  }
  delayBuffer[delayPos] = e * m_pInfo->delayRate;
  delayPos = (delayPos + 1) % DELAY_BUFFER_SIZE;

  e *= 32000.0f;
  if (e < -32700.0f) e = -32700.0f;
  else if (e > 32700.0f) e = 32700.0f;

  uint16_t dac = static_cast<uint16_t>(e + 32768.0f);
  waveOutH = (dac >> 8) & 0xFF; // HHHH HHHH LLLL LLLL
  waveOutL = dac & 0xFF;
}

//--------------------------
float WaveGenerator::CalcFrequency(float note) {
  return m_pInfo->fineTune * pow(2, (note - (69.0f-12.0f))/12.0f);
}

//---------------------------------
// テーブルの補間計算 (波形などのループ用)
float WaveGenerator::InteropL(volatile const float* table, int tableCount, float p) {
  float v = (tableCount - FLT_MIN) *p;
  int t = (int)v;
  v -= t;
  float w0 = table[t];
  t = (t + 1) % tableCount;
  float w1 = table[t];
  return (w0 * (1.0f - v)) + (w1 * v);
}

//---------------------------------
// テーブルの補間計算 (Tan などのループしないもの用)
float WaveGenerator::InteropC(volatile const float* table, int tableCount, float p) {
  float v = (tableCount - FLT_MIN)*p;
  int t = (int)v;
  v -= t;
  float w0 = table[t];
  t = (t + 1) % (tableCount + 1);
  float w1 = table[t];
  return (w0 * (1.0f - v)) + (w1 * v);
}

//-------------------------------------
// ローパス
float WaveGenerator::LowPass(float value) { 
  static float in1  = 0.0f;
  static float in2  = 0.0f;
  static float out1 = 0.0f;
  static float out2 = 0.0f;

  float lp = (lp_b0 * value + lp_b1 * in1  + lp_b2 * in2 - lp_a1 * out1 - lp_a2 * out2) / lp_a0;

  in2  = in1;
  in1  = value;
  out2 = out1;
  out1 = lp;
  return lp;
}

