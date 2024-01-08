
#define CORE0 (0)
#define CORE1 (1)
#define DAC2 (26)

#define CLOCK_DIVIDER (80)
#define TIMER_ALARM (50)
#define SAMPLING_RATE (20000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*50) = 20kHz
#define SAMPLING_TIME_LENGTH (1.0 / SAMPLING_RATE)
#define SAMPLING_RATE_DOUBLE (20000.0)
volatile double mainVolume = 1.0;

const int VOICE_MAX = 4;
volatile double fst[VOICE_MAX] = {0.0, 0.0, 0.0, 0.0};
volatile double phase[VOICE_MAX] = {0.0, 0.0, 0.0, 0.0};
volatile double volReq[VOICE_MAX] = {0.0, 0.0, 0.0, 0.0};
volatile double tm[VOICE_MAX] = {0.0, 0.0, 0.0, 0.0};
volatile double toff[VOICE_MAX] = {-1.0, -1.0, -1.0, -1.0};
const double adsr_attack = 0.01;
const double adsr_decay = 1;
const double adsr_sustain = 0.0;
const double adsr_release = 0.4;

//---------------------------------
double CalcAdsr(double t, double toff) {
  if (t < 0) {
    return 0.0;
  }
  if (t < adsr_attack) {
    return (t / adsr_attack);
  }
  else if ((toff >= 0) && (toff < t)) {
    double t0 = toff - adsr_attack;
    double v = 1 - (t0 / adsr_decay);
    if (v < adsr_sustain)
    {
        v = adsr_sustain;
    }
    if (v > 0) {
      double t1 = (1 - v) * adsr_release;
      double r = 1 - ((t - (toff - t1)) / adsr_release);
      if (r < 0)
      {
          r = 0;
      }
      return r * r;
    }
    return 0;
  }
  else {
    double t0 = t - adsr_attack;
    double v = 1 - (t0 / adsr_decay);
    if (v < adsr_sustain)
    {
        v = adsr_sustain;
    }
    return v * v;
  }
}

//--------------------------
double CalcInvFrequency(double note) {
  return 440.0 * pow(2, (note - (69-12))/12);
}

//---------------------------------
#if 0
double Voice(double p) {
  double v = 255*p;
  int t = (int)v;
  double f = (v - t);
  
  double w0 = wavEGuitar[t];
  t = (t + 1) % 256;
  double w1 = wavEGuitar[t];
  double w = (w0 * (1-f)) + (w1 * f);
  return w / 32767.0;
}
#endif

const double VOICE2_MAXVALUE = 100;
//---------------------------------
double Voice2(double p) {
  if (p < 0.5) {
    return -1;
  }
  else {
    return 1;
  }
}

//---------------------------------
uint8_t CreateWave() {
        double e = 0;
        for (int j = 0; j < VOICE_MAX; j++) {
          phase[j] += fst[j];
          if (phase[j] >= 1.0) phase[j] -= 1.0;
          double g = Voice2(phase[j]);
          e += 130 * g * volReq[j];
        }

    if (e < -127.0) e = -256 - e;
    else if (e > 127.0) e = 256 - e;
    if (e < -127.0) e = -127.0;
    else if (e > 127.0) e = 127.0;

    uint8_t dac = (uint8_t)(e + 127);
    return dac;
}

hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
volatile int interruptCounter = 0;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

//---------------------------------
void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  {
    uint8_t dac = CreateWave();
    dacWrite(DAC2, dac);
    interruptCounter++;
  }
  portEXIT_CRITICAL_ISR(&timerMux);
  //xSemaphoreGiveFromISR(timerSemaphore, NULL);
}

//---------------------------------
void StartMusic() {
    timerSemaphore = xSemaphoreCreateBinary();
    timer = timerBegin(0, CLOCK_DIVIDER, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, TIMER_ALARM, true);
    timerAlarmEnable(timer);
}

static int play_idx = 0;
//---------------------------------
void Play(int note) {
    tm[play_idx] = 0; // minus value to delay
    toff[play_idx] = -1.0;
    double ft = CalcInvFrequency(note);
    fst[play_idx] = (ft / SAMPLING_RATE_DOUBLE);
    play_idx = (play_idx + 1) % VOICE_MAX;
}

static unsigned long sw0 = 0;
//---------------------------------
void Tick() {
  double et = 0.0;
  if (sw0 == 0) {
    sw0 = millis();
  }
  else {
    unsigned long sw = millis();
    et = ((int)(millis() - sw0)) / 1000.0;
    sw0 = sw;
  }
  for (int i = 0; i < VOICE_MAX; i++) {
    if ((fst[i] > 0)&&(tm[i] < 10000)) {
      tm[i] += et;
      volReq[i] = 0.7 * CalcAdsr(tm[i], toff[i]);
    }
  }
}
