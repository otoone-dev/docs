#include <M5Unified.h>
#include <WiFi.h>
#include <AsyncUDP.h>
#include <string>
#include "esp_camera.h"

#define TIMER_CAM (1)
#define UNIT_CAM (0)
#define UNIT_CAM_S3 (0)

//-----------------
#if TIMER_CAM
#include "camera_pins.h"
#include "led.h"
#include "bmm8563.h"
#include "battery.h"
#define LEDPIN (2) // TimerCam
#endif

//-----------------
#if UNIT_CAM
#include "camera_pins.h"
#define LEDPIN (4) // UnitCam
#endif

//-----------------
#if UNIT_CAM_S3
// UnitCamS3
#define LED_GPIO_NUM      14

#define PWDN_GPIO_NUM      0
#define RESET_GPIO_NUM    21
#define XCLK_GPIO_NUM     11
#define SIOD_GPIO_NUM     17
#define SIOC_GPIO_NUM     41

#define Y9_GPIO_NUM       13
#define Y8_GPIO_NUM        4
#define Y7_GPIO_NUM       10
#define Y6_GPIO_NUM        5
#define Y5_GPIO_NUM        7
#define Y4_GPIO_NUM       16
#define Y3_GPIO_NUM       15
#define Y2_GPIO_NUM        6
#define VSYNC_GPIO_NUM    42
#define HREF_GPIO_NUM     18
#define PCLK_GPIO_NUM     12

#define LEDPIN (LED_GPIO_NUM)
#endif // UnitCamS3
//-----------------

#define PORTNUM (11765)

#define ENABLE_INTERVAL_CAM (0)
const int interval_ms = 60*1000; // 60s
#define FRAMERATE (12)
#define TICKTIME (1000/FRAMERATE) 
#define ENABLE_FRAMEDEBUG (1)

volatile int systemStatus = 0;

const double bat_percent_max_vol = 4.1;     // バッテリー残量の最大電圧
const double bat_percent_min_vol = 3.3;     // バッテリー残量の最小電圧
static double bat_per_inclination = 0;        // バッテリー残量のパーセンテージ式の傾き
static double bat_per_intercept   = 0;        // バッテリー残量のパーセンテージ式の切片
static double bat_per             = 0;        // バッテリー残量のパーセンテージ
static double bat_vol             = 0;        // バッテリー電圧

#if 1
// Camera Viewer の AP に直接接続
const char* ssid = "MTCF110"; // SSID, Password を Camera 側と合わせる
const char* password = "6F99z1qm_8ypm"; // 数字の１
IPAddress ipaddr = IPAddress(192,168,4,1);
#else
// 自宅等の WiFi router 経由で接続 (非推奨)
const char* ssid = "ssid"; // 自宅等の SSID, Password を入れる
const char* password = "pass";
IPAddress ipaddr = IPAddress(192,168,1,XX); // Camera Viewer に表示されるアドレスを入れる
#endif


//----------------------------------------
void camera_setup() {
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  //config.frame_size = FRAMESIZE_UXGA;
  config.frame_size   = FRAMESIZE_QVGA; // 320x240
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 20; //jpeg品質 0(高品質)～63(低品質)
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      Serial.printf("PSRAM found\n");
      //config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      Serial.printf("PSRAM not found\n");
      // Limit the frame size when PSRAM is not available
      //config.frame_size   = FRAMESIZE_QVGA; // 320x240
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    Serial.printf("Direct\n");
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
      Serial.printf("Camera init failed with error 0x%x", err);
      return;
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_vflip(s, 1); //上下反転
#if !TIMER_CAM
  s->set_hmirror(s, 1); //左右反転
#endif
  s->set_saturation(s, 2);
/*
s->set_brightness(s, 0);     // -2 to 2
s->set_contrast(s, 0);       // -2 to 2
s->set_saturation(s, 0);     // -2 to 2
s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
s->set_aec2(s, 0);           // 0 = disable , 1 = enable
s->set_ae_level(s, 0);       // -2 to 2
s->set_aec_value(s, 300);    // 0 to 1200
s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
s->set_agc_gain(s, 0);       // 0 to 30
s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
s->set_bpc(s, 0);            // 0 = disable , 1 = enable
s->set_wpc(s, 1);            // 0 = disable , 1 = enable
s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
s->set_lenc(s, 1);           // 0 = disable , 1 = enable
s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
s->set_vflip(s, 0);          // 0 = disable , 1 = enable
s->set_dcw(s, 1);            // 0 = disable , 1 = enable
s->set_colorbar(s, 0);       // 0 = disable , 1 = enable*/
}

//----------------------------------------
void TickThread(void *pvParameters) {
  while (1) {
    switch (systemStatus) {
      default:
        delay(1000);
        break;
      case 0:
      case 1:
        analogWrite(LEDPIN, 1);
        delay(400);
        analogWrite(LEDPIN, 0);
        delay(400);
        break;
      case 2: // get camera data
        analogWrite(LEDPIN, 255);
        delay(100);
        analogWrite(LEDPIN, 1);
        delay(400);
        break;
      case 3: // send data
      case 4:
      case 5:
        analogWrite(LEDPIN, 1);
        delay(100);
        analogWrite(LEDPIN, 0);
        delay(100);
        break;
      case 6: // after send
      case 7:
      case 8:
        analogWrite(LEDPIN, 1);
        delay(200);
        analogWrite(LEDPIN, 255);
        delay(200);
        analogWrite(LEDPIN, 1);
        delay(200);
        analogWrite(LEDPIN, 0);
        delay(200);
        break;
      case 9: // failure
        analogWrite(LEDPIN, 255);
        delay(200);
        analogWrite(LEDPIN, 0);
        delay(200);
        analogWrite(LEDPIN, 1);
        delay(200);
        analogWrite(LEDPIN, 0);
        delay(200);
        break;
      case 10: // disconnected
        analogWrite(LEDPIN, 1);
        delay(100);
        analogWrite(LEDPIN, 0);
        delay(100);
        analogWrite(LEDPIN, 1);
        delay(100);
        analogWrite(LEDPIN, 0);
        delay(400);
        break;
    }
  }
}

//----------------------------------------
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  // Init
  Serial.begin(115200);
  Serial.println("serial start");

#if TIMER_CAM
  Serial.println("bat_init");
  bat_init();
  Serial.println("bmm8563_init");
  bmm8563_init();
  Serial.println("led_init");
  led_init(LEDPIN);
#endif
  // Test LED
        analogWrite(LEDPIN, 255);
  delay(200);

  Serial.println("camera_setup");
  camera_setup();

#if TIMER_CAM
  pinMode(13, INPUT_PULLUP); // Grove端子での電源OFF検出用
#endif
#if UNIT_CAM
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
#endif
  analogWrite(LEDPIN, 0);

  bat_per_inclination = 100.0F/(bat_percent_max_vol-bat_percent_min_vol);
  bat_per_intercept = -1 * bat_percent_min_vol * bat_per_inclination;

  xTaskCreatePinnedToCore(TickThread, "TickThread", 4096, NULL, 1, NULL, 1);

  Serial.println("init done");
}

//----------------------------------------
void checkKeepAlivePin() {
#if TIMER_CAM
    if (digitalRead(13)) { // TimerCamera の SCL(GPIO13) を Low にしておくとバッテリ駆動する。Grove 端子から抜くと電源OFF
      // Halt
      bat_disable_output();
    }
  bat_per = bat_per_inclination * bat_vol + bat_per_intercept;    // %
#endif
}

#define BUFF_MAX (2048)

//----------------------------------------
void loop() {
  AsyncUDP udp;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  uint8_t* sendBuff = reinterpret_cast<uint8_t*>(malloc(BUFF_MAX));
  if (!sendBuff) {
    Serial.println("can't allocate memory...");
    while (1) {
      delay(1000);
    }
  }

  while (1) {
    Serial.printf("WiFi connecting (ssid=%s)...\n", ssid);
    //led_brightness(10);
    while (WiFi.waitForConnectResult(500) != WL_CONNECTED) {
      systemStatus = 1;
        Serial.printf("WiFi Failed (ssid=%s)\n", ssid);
        //led_brightness(1024);
        delay(250);
        //led_brightness(10);
        delay(250);
        checkKeepAlivePin();
        Serial.printf("WiFi Retry to connect\n");
    }

    uint8_t frameCount = 0;

    if(udp.connect(ipaddr, PORTNUM) && udp.connected()) {
        Serial.println("UDP connected");
        while (WiFi.status() == WL_CONNECTED) {
      systemStatus = 2;
          //led_brightness(256);
          camera_fb_t *fb = esp_camera_fb_get();
          unsigned long t0 = millis();
          if (fb) {
            {
      systemStatus = 3;
              const uint8_t* buf = reinterpret_cast<const uint8_t*>(fb->buf);

              // frameCount, seqNum, seqTotal, checkSum, size(2), data ...

              uint8_t seqNum = 0;
              uint8_t seqTotal = fb->len / 1024 + 1;
              int size = fb->len;
#if ENABLE_FRAMEDEBUG
              Serial.printf("--------\n");
#endif
              for (int seqNum = 0; seqNum < seqTotal; seqNum++) {
                uint8_t* p = sendBuff;
                *p++ = frameCount;
                *p++ = seqNum;
                *p++ = seqTotal;
                *p++ = 0; // sum
#if TIMER_CAM
                *p++ = (unsigned char)M5.Power.getBatteryLevel();
#else
                *p++ = 0;
#endif
                int sendSize = 0;
                if (size >= 1024) {
                  sendSize = 1024;
                }
                else {
                  sendSize = size;
                }
                *p++ = sendSize & 0xFF;
                *p++ = (sendSize >> 8) & 0xFF;
                size -= sendSize;
                uint8_t sum = 0;
                for (int i = 0; i < sendSize; i++) {
                  uint8_t d = *buf++;
                  *p++ = d;
                  sum += d;
                }
                sendBuff[3] = sum;
                //led_brightness(10);
                size_t totalSendSize = (size_t)(p - sendBuff);
      systemStatus = 4;
                if (udp.connected()) {
                  udp.writeTo(sendBuff, totalSendSize, ipaddr, PORTNUM);
                }
                delay(1);
      systemStatus = 5;
                if (udp.connected()) {
                  udp.writeTo(sendBuff, totalSendSize, ipaddr, PORTNUM);
                }
                delay(1);
      systemStatus = 6;
#if ENABLE_FRAMEDEBUG
                int t = (int)(millis() - t0);
                Serial.printf("%d send %df (%d/%d) %d, %d, sum=%02x\n", t, frameCount, seqNum, seqTotal, (int)sendSize, size, sum);
#endif
              }
            }

            esp_camera_fb_return(fb);
            frameCount = (frameCount+1)%128;
      systemStatus = 7;

#if ENABLE_INTERVAL_CAM
            M5.Power.deepSleep(SLEEP_MIN(time));
#endif
            sendBuff[0] = 0xEF; // Sended
            udp.writeTo(sendBuff, 1, ipaddr, PORTNUM);
      systemStatus = 8;
          }
          else {
      systemStatus = 9;
            // fb 取れなかった
            for (int i = 0; i < 10; i++) {
              //led_brightness(1024);
              delay(100);
              //led_brightness(10);
              delay(100);
            }
            sendBuff[0] = 0xEE; // Camera Fail
            udp.writeTo(sendBuff, 1, ipaddr, PORTNUM);
          }
          //led_brightness(1);
          int t = (int)(millis() - t0);
          if (t < TICKTIME) {
            delay(TICKTIME-t);
          }
          checkKeepAlivePin();
        }
    }
      systemStatus = 10;
    delay(1000);
    checkKeepAlivePin();
  }
}
