#include <M5Unified.h>

//#define AFUUE_VER (110) // 16Bit-PWM ADC-MCP3425  (AFUUE2 Second)
#define AFUUE_VER (111) // 16Bit-PWM ADCx2        (AFUUE2R First)

#if (AFUUE_VER == 110)
// AFUUE2 改良版
#define _M5STICKC_H_
#define PWMPIN_LOW (0)
#define PWMPIN_HIGH (26)

#elif (AFUUE_VER == 111)
// AFUUE2R 初代
#define _STAMPS3_H_
#define PWMPIN_LOW (39)
#define PWMPIN_HIGH (40)

#endif

#define CORE0 (0)
#define CORE1 (1)

//-------------------------------------
static hw_timer_t * timer = NULL;
static portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
#define QUEUE_LENGTH 1
static QueueHandle_t xQueue;
static TaskHandle_t taskHandle;
static float currentNote = 60.0f;
static float currentWavelength = 0.0f;
volatile float currentWavelengthTickCount = 0.0f;
volatile float currentVolume = 0.0f;
static float requestedVolume = 0.0f;
volatile float phase = 0.0f;
volatile float lowPassValue = 0.0f;
volatile uint8_t outH = 0;
volatile uint8_t outL = 0;

const float lowPassP = 0.5f;
const float lowPassR = 5.0f;
const float lowPassQ = 1.5f; // これを大きくするとよりポワッとした音になる

#define CLOCK_DIVIDER (80)
#define TIMER_ALARM (40)
#define SAMPLING_RATE (25000.0f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*40) = 25kHz
#define LOWPASS_CUTOFF_LIMIT (12000.0f)

//-------------------------------------
// 波形データの時点で -1.0f～1.0f にした方がいいのですが、昔のデータをそのまま使ってるので int16 の範囲になってます
const float waveAfuueCla[256] = {
    -2725, -4620, -6515, -8205, -9429, -10653, -11167, -11228,
    -11289, -10400, -9424, -8404, -7236, -6067, -5213, -4643,
    -4072, -3849, -3695, -3522, -3224, -2926, -2523, -1988,
    -1453, -1125, -868, -640, -929, -1218, -1632, -2270,
    -2908, -3621, -4373, -5124, -5539, -5942, -6163, -5900,
    -5638, -5276, -4842, -4408, -4151, -3917, -3694, -3516,
    -3337, -3208, -3130, -3052, -2849, -2616, -2351, -1813,
    -1275, -780, -345, 89, 378, 609, 830, 612,
    393, -163, -1420, -2677, -4501, -6654, -8807, -10446,
    -12050, -13185, -12849, -12514, -11494, -9907, -8321, -7035,
    -5800, -4678, -4153, -3628, -3319, -3263, -3206, -3520,
    -3945, -4414, -5441, -6467, -7738, -9411, -11083, -12853,
    -14667, -16482, -18033, -19581, -20991, -22070, -23149, -23635,
    -23727, -23818, -23236, -22582, -21959, -21444, -20930, -20619,
    -20496, -20374, -20523, -20731, -20941, -21163, -21386, -21378,
    -21066, -20754, -19956, -18984, -17975, -16123, -14271, -12090,
    -9293, -6496, -3639, -750, 2139, 4240, 6307, 8028,
    8781, 9533, 9689, 9393, 9097, 8661, 8206, 7753,
    7317, 6881, 6334, 5668, 5002, 4735, 4573, 4565,
    6025, 7486, 9250, 11467, 13685, 14758, 15353, 15921,
    13932, 11944, 9593, 6458, 3322, 906, -1071, -3050,
    -3454, -3733, -3830, -3324, -2819, -2345, -1897, -1450,
    -1309, -1225, -1106, -780, -454, 56, 791, 1527,
    2217, 2893, 3541, 3776, 4011, 4010, 3604, 3198,
    2798, 2399, 2001, 2597, 3214, 4136, 5827, 7517,
    9113, 10644, 12175, 12856, 13437, 13838, 13517, 13197,
    12463, 11326, 10189, 8887, 7546, 6241, 5207, 4173,
    3476, 3243, 3010, 3653, 4627, 5629, 7526, 9422,
    11412, 13586, 15759, 17718, 19556, 21394, 23017, 24628,
    26156, 27437, 28718, 29698, 30439, 31179, 31538, 31836,
    31923, 30970, 30017, 28319, 25791, 23264, 20193, 16967,
    13773, 10931, 8088, 5531, 3420, 1309, -829, -2979,
};

//-------------------------------------
// ローパスフィルタ
// value: 現在の音のサンプリング１つぶんの値
// freq: カットオフ周波数
// q: Q値
float LowPass(float value, float freq, float q) {
  float omega = 2.0f * 3.14159265f * freq / SAMPLING_RATE;
  float alpha = sin(omega) / (2.0f * q);

  float cosv = cos(omega);
  float one_minus_cosv = 1.0f - cosv;
  float a0 =  1.0f + alpha;
  float a1 = -2.0f * cosv;
  float a2 =  1.0f - alpha;
  float b0 = one_minus_cosv / 2.0f;
  float b1 = one_minus_cosv;
  float b2 = b0;
 
  static float in1  = 0.0f;
  static float in2  = 0.0f;
  static float out1 = 0.0f;
  static float out2 = 0.0f;

  float lp = (b0 * value + b1 * in1  + b2 * in2 - a1 * out1 - a2 * out2) / a0;

  in2  = in1;
  in1  = value;
  out2 = out1;
  out1 = lp;
  return lp;
}

//-------------------------------------
// 波形データから p (0.0f - 1.0f) の位置の波形を取り出す（256段階のデータの間の補間もする）
float Voice(float p) {
  float v = 255.99f*p;
  int t = (int)v;
  v -= t;

  float w0 = waveAfuueCla[t];
  t = (t + 1) % 256;
  float w1 = waveAfuueCla[t];
  float w = (w0 * (1.0f-v)) + (w1 * v);
  return w;
}

//-------------------------------------
// 波形生成処理メイン
uint16_t CreateWave() {
  // 現在の波形の参照位置を進める
  phase += currentWavelengthTickCount;
  if (phase >= 1.0f) phase -= 1.0f;

  // 波形データ取り出し
  float g = Voice(phase) / 32767.0f;

  // ローパスフィルタで音をいじる
  g = LowPass(g, lowPassValue, lowPassQ);

  // 音量
  float e = g * currentVolume;

  // 16bit の範囲に成形して出力
  e *= 32700.0f;
  if (e < -32700.0f) e = -32700.0f;
  else if (e > 32700.0f) e = 32700.0f;

  return static_cast<uint16_t>(e + 32767.0f);
}
//---------------------------------
// サンプリング周波数ごとに正確に呼ばれるタイマー
void IRAM_ATTR onTimer(){
  portENTER_CRITICAL_ISR(&timerMux);
  // 現在の値を書き込むだけ（タイマー割込みで重い処理は書かない方がいいので）
  ledcWrite(0, outL);
  ledcWrite(1, outH);

  int8_t data;
  xQueueSendFromISR(xQueue, &data, 0); // キューを送信
  portEXIT_CRITICAL_ISR(&timerMux);
}

//---------------------------------
// 波形生成タスク
void createWaveTask(void *pvParameters) {
  // ここが常にまわっている
  while (1) {
    int8_t data;
    xQueueReceive(xQueue, &data, portMAX_DELAY); // タイマー割込みで送信されるキューを受信するまで待つ＝サンプリング周波数ごとに次へ進む

    uint16_t dac = CreateWave();
    outH = (dac >> 8) & 0xFF;
    outL = dac & 0xFF;
  }
}


//-------------------------------------
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  // 2本の PWM を AFUUE2 基板の抵抗の差で 16bit 相当にしている。1本の PWM の場合は HIGH 側だけ使えばよい。
  pinMode(PWMPIN_LOW, OUTPUT);
  pinMode(PWMPIN_HIGH, OUTPUT);
  ledcSetup(0, 156250, 8); // 156250Hz, 8Bit(256段階)
  ledcAttachPin(PWMPIN_LOW, 0);
  ledcWrite(0, 0);
  ledcSetup(1, 156250, 8); // 156250Hz, 8Bit(256段階)
  ledcAttachPin(PWMPIN_HIGH, 1);
  ledcWrite(1, 0);

  // CORE0 で動く波形生成タスクを作成
  xQueue = xQueueCreate(QUEUE_LENGTH, sizeof(int8_t));
  xTaskCreatePinnedToCore(createWaveTask, "createWaveTask", 16384, NULL, 5, &taskHandle, CORE0);

  // タイマー割込み（CORE1 での動作）
  timer = timerBegin(0, CLOCK_DIVIDER, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, TIMER_ALARM, true);
  timerAlarmEnable(timer);
}

//--------------------------
// MIDI のノート番号から周波数に変換
float CalcFrequency(float note) {
  return 440.0f * pow(2.0f, (note - (69.0f-12.0f))/12.0f);
}

//--------------------------
// Arduino Runs On がデフォルトなら CORE1 でループが回る
void loop() {
  // MIDI ノート番号 (currentNote) からサンプリング周波数あたりの波形データ参照位置の移動量 (currentWavelengthTickCount) を計算
  float wavelength = CalcFrequency(currentNote);
  currentWavelength += (wavelength - currentWavelength) * 0.85f;
  currentWavelengthTickCount = (currentWavelength / SAMPLING_RATE);

  // ボタンを押したら鳴らす
#ifdef _STAMPS3_H_
  if (digitalRead(0) == LOW) {
    requestedVolume = 0.7f;
  }
  else {
    requestedVolume = 0.01f;
  }
#endif
#ifdef _M5STICKC_H_
  M5.update();
  if (M5.BtnA.isPressed()) {
    requestedVolume = 0.7f;
  }
  else {
    requestedVolume = 0.01f;
  }
#endif
  currentVolume += (requestedVolume - currentVolume) * 0.05f; // 音量をフワッっと上げるため

  // ローパスパラメータの更新
  float a = (tanh(lowPassR*(currentVolume-lowPassP)) + 1.0f) * 0.5f; // 音量でカットオフ周波数を動かす（tanh を使ってるのはリニアにしないため）
  float lp = 100.0f + 20000.0f * a; // このへんの値は適当
  if (lp > LOWPASS_CUTOFF_LIMIT) { // サンプリング周波数の 1/2 より下にしないと計算が破綻する
    lp = LOWPASS_CUTOFF_LIMIT;
  }
  lowPassValue += (lp - lowPassValue) * 0.8f;

  delay(5);
}
