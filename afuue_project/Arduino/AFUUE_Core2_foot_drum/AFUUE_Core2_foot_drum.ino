#pragma mark - Depend ESP8266Audio and ESP8266_Spiram libraries
/* 
cd ~/Arduino/libraries
git clone https://github.com/earlephilhower/ESP8266Audio
git clone https://github.com/Gianbacchio/ESP8266_Spiram

7yq99a5nx@a
*/

#include <vector>
#include <string>
#include <M5Core2.h>
//#include <WiFi.h>

#include "hihat_closed.h"
#include "kick.h"
#include "snare.h"
#include "crash.h"
#include "ride.h"

#define BT_A2DP (0)
#define BT_DEVICENAME "BT earphone"
#if BT_A2DP
//------ A2DP ------
#include <BluetoothA2DPCommon.h>
//#include <BluetoothA2DPSink.h>
#include <BluetoothA2DPSource.h>
#include <SoundData.h>
BluetoothA2DPSource a2dp_source;
#else
//------ I2S ------
#include <driver/i2s.h>
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include "driver/i2s.h"
AudioGeneratorMP3 *mp3;
AudioOutputI2S *out;
AudioFileSourceSD *file;
AudioFileSourceID3 *id3;
AudioFileSourceSD *fileB;
AudioFileSourceID3 *id3B;
#define CONFIG_I2S_BCK_PIN 12
#define CONFIG_I2S_LRCK_PIN 0
#define CONFIG_I2S_DATA_PIN 2
#define CONFIG_I2S_DATA_IN_PIN 34
#define Speak_I2S_NUMBER I2S_NUM_0
#define MODE_MIC 0
#define MODE_SPK 1
#endif

const double VOICE2_MAX = 1000;


static int buttons = 0;
static int fileNo = 0;
static int mode = 0;
static int counter = 0;

#define CORE0 (0)
#define CORE1 (1)
#define DATA_SIZE 256

#define SAMPLING_RATE (44100) //44100
#define SAMPLING_RATE_DOUBLE (44100.0)
volatile double mainVolume = 1.0;
volatile bool isPressed = false;
static bool isVoiceStarted = false;

#define REVERB_BUFFER_SIZE (9801)
volatile double reverbBuffer[REVERB_BUFFER_SIZE];
volatile int reverbPos = 0;
static double reverbRate = 0.08;

volatile int16_t soundBuffer[DATA_SIZE];    // DMA転送バッファ
const int VOICE_MAX = 5;
volatile double fst[VOICE_MAX] = {0.0, 0.0, 0.0, 0.0, 0.0};
volatile int hcnt[VOICE_MAX] = {0, 0, 0, 0, 0};
volatile int pcnt[VOICE_MAX] = {0, 0, 0, 0, 0};
volatile double phase[VOICE_MAX] = {0.0, 0.0, 0.0, 0.0, 0.0};
volatile double volReq[VOICE_MAX] = {0.0, 0.0, 0.0, 0.0, 0.0};
const double adsr_attack = 0.01;
const double adsr_decay = 3;
const double adsr_sustain = 0.0;
const double adsr_release = 0.4;
volatile double currentPressure = 0.0;
volatile float accX = 0;
volatile float accY = 0;
volatile float accZ = 0;
volatile double dist = 0;

//---------------------------------
#define TX1 (80)
#define TX2 (160)
#define TY1 (80)
#define TY2 (240)
#define TW (240)
#define TH (320)

void PrintInfo() {
#if 0
  M5.Lcd.fillRect(0, TY1+1, 240, (TY2-TY1)-1, BLACK);

  if (current_code != "") {
    M5.Lcd.setTextSize(2);
    SetTextColor();
    M5.Lcd.drawCentreString(current_code.c_str(), 120, 100, 4);
  }
  M5.Lcd.setTextColor(WHITE);
  if ((currentMusic != 0) && (currentMusic->title.c_str() != 0)) {  
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawCentreString(currentMusic->title.c_str(), 120, 195, 4);
  }
  int x = ((TW - 10)*codes_point) / codes_max;
  M5.Lcd.fillRect(x, TH/2-3, 10, 6, WHITE);
#endif
}

#if !BT_A2DP
//---------------------------------
bool InitI2SSpeakOrMic(int mode)
{
    esp_err_t err = ESP_OK;

    i2s_driver_uninstall(Speak_I2S_NUMBER);
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER),
        .sample_rate = SAMPLING_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = 128,
    };
    if (mode == MODE_MIC)
    {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    }
    else
    {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
        i2s_config.use_apll = false;
        i2s_config.tx_desc_auto_clear = true;
    }
    err += i2s_driver_install(Speak_I2S_NUMBER, &i2s_config, 0, NULL);
    i2s_pin_config_t tx_pin_config;

    tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;
    tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
    tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
    tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;
    err += i2s_set_pin(Speak_I2S_NUMBER, &tx_pin_config);
    err += i2s_set_clk(Speak_I2S_NUMBER, SAMPLING_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
    i2s_zero_dma_buffer(Speak_I2S_NUMBER);

    delay(1000);
    return true;
}
#endif

#if BT_A2DP
//---------------------------------
int32_t bt_callback(Channels *data, int32_t len) {
      for (int i = 0; i < len; i++) {
        double e = 0;
        for (int j = 0; j < VOICE_MAX; j++) {
          phase[j] += fst[j];
          if (phase[j] >= 1.0) phase[j] -= 1.0;
          double g = Voice2(phase[j]);
          e += g * volReq[j];
        }
        e += reverbBuffer[reverbPos];
        if ( (-0.005 < e) && (e < 0.005) ) {
          e = 0.0;
        }
        if (e < -32000.0) e = -32000;
        else if (e > 32000.0) e = 32000;
        reverbBuffer[reverbPos] = e * reverbRate;
        reverbPos = (reverbPos + 1) % REVERB_BUFFER_SIZE;
        int16_t d = (int16_t)(e);

        data[i].channel1 = d;
        data[i].channel2 = d;
      }
  return len;
}

#else

//---------------------------------
void voiceThread(void *pvParameters)
{
    for(;;) {
      for (int i = 0; i < DATA_SIZE; i++) {
        double e = 0;
        for (int j = 0; j < VOICE_MAX; j++) {
          if (hcnt[j] != pcnt[j]) {
            pcnt[j] = hcnt[j];
            phase[j] = 0.0;
          } else if (phase[j] < 44100*3) {
            phase[j] += fst[j];
          }
          int p = (int)phase[j];
          switch (j) {
            case 0:
              if (p < hihat_closed_raw_len/2) {
                double g = hihat_closed_raw[p];
                e += g * volReq[j];
              }
              break;            
            case 1:
              if (p < kick_raw_len/2) {
                double g = kick_raw[p];
                e += g * volReq[j];
              }
              break;            
            case 2:
              if (p < snare_raw_len/2) {
                double g = snare_raw[p];
                e += g * volReq[j];
              }
              break;            
            case 3:
              if (p < crash_raw_len/2) {
                double g = crash_raw[p];
                e += g * volReq[j];
              }
              break;            
            case 4:
              if (p < ride_raw_len/2) {
                double g = ride_raw[p];
                e += g * volReq[j];
              }
              break;            
          }
        }
        e += reverbBuffer[reverbPos];
        if ( (-0.005 < e) && (e < 0.005) ) {
          e = 0.0;
        }
        if (e < -32000.0) e = -32000;
        else if (e > 32000.0) e = 32000;
        reverbBuffer[reverbPos] = e * reverbRate;
        reverbPos = (reverbPos + 1) % REVERB_BUFFER_SIZE;
        int16_t d = (int16_t)(e);
        
        soundBuffer[i] = d;
      }
      size_t transBytes;
      i2s_write(Speak_I2S_NUMBER, (char*)soundBuffer, sizeof(soundBuffer), &transBytes, portMAX_DELAY);
    }
}
#endif

static int touchThreshold14 = 0;
static int touchThreshold35 = 0;
static int touchThreshold25 = 0;
static int touchThreshold27 = 0;

//----------------------------------
void setup()
{
  //Wire.begin(32, 33);
  M5.begin(true, true, true, false);

  M5.Axp.SetLed(1);
  delay(200);
  M5.Axp.SetLed(0);
  delay(200);
  M5.Axp.SetLed(1);
  delay(200);
  M5.Axp.SetLed(0);

  Serial.println("-----------");

  //M5.IMU.Init(); // accerelometer

  Serial.println("Hello DRUMKIT");
  //WiFi.mode(WIFI_OFF); 

  M5.Lcd.setBrightness(100);
  Serial.println("-----------");
  
  M5.Lcd.setRotation(0);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.drawFastHLine(0, TY1, TW, WHITE);
  M5.Lcd.drawFastHLine(0, TY2, TW, WHITE);
  M5.Lcd.drawFastVLine(TX1, 0, TH, WHITE);
  M5.Lcd.drawFastVLine(TX2, 0, TH, WHITE);
  M5.Lcd.setTextFont(4);

  const int keyPortList[7] = { 13, 14, 19, 27, 25, 26, 35 };
  for (int i = 0; i < 7; i++) {
    if (keyPortList[i] < 34) {
      pinMode(keyPortList[i], INPUT_PULLUP);
    }
    else {
      pinMode(keyPortList[i], INPUT); // 34-39 は外付け PULLUP が必要
    }
  }

  double t14 = 0.0;
  double t25 = 0.0;
  double t27 = 0.0;
  double t35 = 0.0;
  for (int i = 0; i < 20; i++) {
    t14 += (touchRead(14) - t14) * 0.5;
    t25 += (touchRead(25) - t25) * 0.5;
    t27 += (touchRead(27) - t27) * 0.5;
    t35 += (touchRead(35) - t35) * 0.5;
    delay(100);
  }
  touchThreshold14 = (int)(t14 * 0.7);
  touchThreshold25 = (int)(t25 * 0.7);
  touchThreshold27 = (int)(t27 * 0.7);
  touchThreshold35 = (int)(t35 * 0.7);

#if BT_A2DP
  a2dp_source.start(BT_DEVICENAME, bt_callback);
  if (a2dp_source.isConnected() == false) {
    M5.Axp.SetLed(1);
    delay(100);
    M5.Axp.SetLed(0);
    delay(100);
  }
#else
  M5.Axp.SetSpkEnable(true);
  InitI2SSpeakOrMic(MODE_SPK);
  xTaskCreatePinnedToCore(voiceThread, "voiceThread", 8192, NULL, 3, NULL, CORE1);
#endif
}

//----------------------------------
static unsigned long sw0 = 0;
static unsigned long pd0 = -1;
static int swcnt = 0;
static int pdt = 0;
static double pdVol = 0.0;
static int pdmode = 0;
static uint8_t keyCurrent = 0;
static uint8_t keyStable = 0;
const uint8_t bitKick = (1 << 0);
const uint8_t bitHiHat = (1 << 1);
const uint8_t bitSnare = (1 << 2);
const uint8_t bitCrash = (1 << 3);
const uint8_t bitTouch = (1 << 4);
const uint8_t bitPedal = (1 << 5);
const uint8_t bitRide = (1 << 6);

void PedalExec(int pedalA, int pedalB, double et) {
  switch (pdmode) {
    case 0: {
      if (pedalA == LOW) {
        pd0 = millis();
        pdmode = 1;
      }
      break;
    }
    case 1: {
      if (pedalB == LOW) {
        pdt = (int)(millis() - pd0); // 0ms - 300ms
        if (pdt > 500) pdt = 500;
        pdmode = 2;
        fst[1] = 1.0;
        volReq[1] = 1.0;
        hcnt[1]++;
      }
      else {
        if (pedalA != LOW) {
          pd0 = millis();
        }
      }
      break;
    }
    case 2: {
      if ((pedalA != LOW) && (pedalB != LOW)) {
        pdmode = 3;
        //Serial.println("3");
      }
      break;
    }
    case 3: {
      if (pedalA == LOW) {
        pd0 = millis();
        pdmode = 1;
        //Serial.println("1");
      }
      break;
    }
  }
}

//----------------------------------
void loop()
{
  //Serial.println(touchRead(14));
  //delay(500);
  //return;
  
  double et = 0.0;
  if (sw0 == 0) {
    sw0 = millis();
  }
  else {
    unsigned long sw = millis();
    et = ((int)(millis() - sw0)) / 1000.0;
    sw0 = sw;
  }
  int b = 0;
  TouchPoint_t pos= M5.Touch.getPressPoint();
  int touch_x = pos.x;
  int touch_y = pos.y;

  bool keyCrash = (touchRead(25) > touchThreshold25);
  bool keySnare = (touchRead(14) > touchThreshold14);
  bool keyKick = digitalRead(13);
  bool keyHiHat = (touchRead(27) > touchThreshold27);
  bool keyPedal = true;//digitalRead(35);
  bool pedalA = true;//digitalRead(25);
  bool pedalB = true;//digitalRead(26);

  uint8_t keyData = 0;
  if (keyKick == LOW) keyData |= bitKick;
  if (keyHiHat == LOW) keyData |= bitHiHat;
  if (keySnare == LOW) keyData |= bitSnare;
  if (keyCrash == LOW) keyData |= bitCrash;
  //if (keyPedal == LOW) keyData |= bitKick;
/*
#define TX1 (80)
#define TX2 (160)
#define TY1 (80)
#define TY2 (240)
#define TW (240)
#define TH (320)

 */
  if (M5.Touch.ispressed() && ((touch_x >= 0) || (touch_y  >= 0))) {
    keyData |= bitTouch;
    // タッチで物理ボタンの代わり
    if (touch_y < TY1) {
      if (touch_x < TX1) {
        keyData |= bitHiHat;
      }
      else if (touch_x > TX2) {
        keyData |= bitCrash;
      }
      else {
        keyData |= bitRide;
      }
    }
    else if (touch_y <TY2) {
      keyData |= bitSnare;
    }
    else {
      keyData |= bitKick;
    }
    //char s[32];
    //sprintf(s, "%d, %d\n", touch_x, touch_y);
    //Serial.println(s);
  }

  uint8_t keyPush = 0;
  if (keyData == keyCurrent) {
    swcnt++;
    if (swcnt == 1) {
      // 0010 = (0000 ^ 0010) & 1111
      // 0000 = (0010 ^ 0000) & 1101
       
      //  0000 = (0000 ^ 0010) & 0000 : down
      //  0010 = (0010 ^ 0000) & 0010 : up
      keyPush = (keyStable ^ keyCurrent) & (~keyStable);
      //keyUp = (keyStable ^ keyCurrent) & keyStable;
      keyStable = keyCurrent;
  
      if (keyPush & bitHiHat) {
        // Hi-hat
        fst[0] = 1.0;
        volReq[0] = 0.3;
        hcnt[0]++;
      }
      if (keyPush & bitKick) {
        // Kick
        fst[1] = 1.0;
        volReq[1] = 1.0;
        hcnt[1]++;
      }
      if (keyPush & bitSnare) {
        // Snare
        fst[2] = 1.0;
        volReq[2] = 0.7;
        hcnt[2]++;
      }
      if (keyPush & bitCrash) {
        // Crash
        fst[3] = 1.0;
        volReq[3] = 0.5;
        hcnt[3]++;
      }
      if (keyPush & bitRide) {
        // Ride
        fst[4] = 1.0;
        volReq[4] = 0.8;
        hcnt[4]++;
      }
    }
  } else {
    swcnt = 0;
  }
  keyCurrent = keyData;

  //if (keyData & bitTouch) {
  //  if ((touch_y > TY1) && (touch_y < TY2)) {
  //    pedalA = LOW;
  //    pedalB = LOW;
  //  }
  //}
  //PedalExec(pedalA, pedalB, et);
  

#if 0
  //M5.Lcd.fillScreen();
  M5.Lcd.setTextSize(1);
  int y = 20;
  char s[64];
  sprintf(s, "POS(%d, %d, %d)    ", M5.Touch.ispressed(), touch_x, touch_y);
  M5.Lcd.drawString(s,0,y); y+= 28;  
  sprintf(s, "K %02d(%d, %d, %d, %d, %d, %d, %d)", swcnt%100, (int)keyHiHat, (int)keySnare, (int)keyKick, (int)keyCrash, (int)keyPedal, (int)pedalA, (int)pedalB);
  M5.Lcd.drawString(s,0,y); y+= 28;  
  sprintf(s, "K %08b", keyData);
  M5.Lcd.drawString(s,0,y); y+= 28;  
  sprintf(s, "K %08b", keyCurrent);
  M5.Lcd.drawString(s,0,y); y+= 28;  
  //sprintf(s, "ET(%d ms)    ", (int)(et*1000));
  //M5.Lcd.drawString(s,0,y); y+= 28;  
  //sprintf(s, "PDT(%d ms)    ", pdt);
  //M5.Lcd.drawString(s,0,y); y+= 28;  
  //sprintf(s, "NT(%d, %d, %d, %d)", code_notes[0], code_notes[1], code_notes[2], code_notes[3] );
  //M5.Lcd.drawString(s,0,y); y+= 28;

  //if (current_code != "") {
  //  M5.Lcd.drawString(current_code.c_str(), 0, y); y+= 28;
  //}
  
  //sprintf(s, "NOTE(%1.1f)    ", 59+(float)note);
  //M5.Lcd.drawString(s,0,y); y+= 28;  
#endif
  delay(5);
}
