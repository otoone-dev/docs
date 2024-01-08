#include <M5StickCPlus.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <time.h>
#include "seg7.h"

#define DEBUG_PRINT(dbgmes) { Serial.print(dbgmes); M5.Lcd.print(dbgmes); }
#define DEBUG_PRINTLN(dbgmes) { Serial.println(dbgmes); M5.Lcd.println(dbgmes); }
#define DEBUG_PRINTF(dbgmes, ...) { Serial.printf(dbgmes, __VA_ARGS__); M5.Lcd.printf(dbgmes, __VA_ARGS__); }
#define DEBUG_PRINTFLN(dbgmes, ...) { Serial.printf(dbgmes "\n", __VA_ARGS__); M5.Lcd.printf(dbgmes "\n", __VA_ARGS__); }

#define WEATHER_LATITUDE "35.6584149"
#define WEATHER_LONGITUDE "139.7455983"
#define WEATHER_APPID "******" // openweathermap から発行される APPID
#define BICYCLE_CIRCUMFERENCE (1.1937) // タイヤの１周の距離(m)
#define BODY_WEIGHT (60) // 体重(kg)

const char* ssid = "ssid" //Enter SSID
const char* password = "pass"  //Enter Password

const char* openweathermap_url = "http://api.openweathermap.org/data/2.5/onecall?lat=" WEATHER_LATITUDE "&lon=" WEATHER_LONGITUDE "&units=metric&lang=en&appid=" WEATHER_APPID;
const char* ntpServer =  "ntp.jst.mfeed.ad.jp";

const int LED_PIN = 10;
const uint16_t COLOR_ELAPSED = TFT_WHITE;
const uint16_t COLOR_TRIP = TFT_CYAN;
const uint16_t COLOR_ODO = TFT_PINK;
const uint16_t COLOR_AVESPD = TFT_GREEN;
const uint16_t COLOR_MAXSPD = TFT_RED;
const uint16_t COLOR_BURN = TFT_ORANGE;
const double bat_percent_max_vol = 4.1;     // バッテリー残量の最大電圧
const double bat_percent_min_vol = 3.3;     // バッテリー残量の最小電圧

static HTTPClient http;

static double bat_per_inclination = 0;        // バッテリー残量のパーセンテージ式の傾き
static double bat_per_intercept   = 0;        // バッテリー残量のパーセンテージ式の切片
static double bat_per             = 0;        // バッテリー残量のパーセンテージ
static double bat_vol             = 0;        // バッテリー電圧
static double odometer = 0;
static double tripmeter = 0;
static double maxSpeed = 0;
static double maxSpeed2 = 0;
static double tripKcal = 0;
static int scrBright = 100;        // 明るさ
static double speed = 0;
static double tempSpeed = 0;
static int counter = 0;
static int stopTimeCounter = 0;
static unsigned long t0 = 0;
static int mode = 0;
static bool isRun = false;
static unsigned long startTime = 0;
static double elapsedTime = 0;
static bool enableWeather = false;
static int weather_now = 0;
static int weather_12h = 0;
static double tempMin = 99;
static double tempCurrent = 0;
static double tempMax = 0;
static bool securityMode = false;

static Preferences preferences;

//----------------------------------
bool getWeather(const char *url) {
  // openweathermap から天気を取得
  // json パース方法は https://arduinojson.org/v6/assistant/ 参照
  bool result = true;

  DEBUG_PRINTLN("[HTTP] Connecting...");
  bool ret = http.begin(url);
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  if (ret == false) {
    DEBUG_PRINTLN("[HTTP] failed.");
    delay(1000);
    return false;
  }

  DEBUG_PRINTLN("GET openweathermap");
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    DEBUG_PRINTLN("[HTTP] Loading...");

    http.useHTTP10(true);
    String jsonStr = http.getString();
    http.end();
    //DEBUG_PRINTFLN("free %d", xPortGetFreeHeapSize());
    DynamicJsonDocument doc(32768);
    DeserializationError error = deserializeJson(doc, jsonStr.c_str());
    jsonStr = "";
    if (!error) {
      DEBUG_PRINTLN("[JSON] parse ok.");
      {
        JsonObject current = doc["current"];
        if (current == NULL) return false;
        DEBUG_PRINTLN("[JSON] current");
        tempCurrent = current["temp"];
        JsonObject weather = current["weather"][0];
        if (weather == NULL) return false;
        //DEBUG_PRINTLN("[JSON] weather");
        const char* icon = weather["icon"];
        if (icon && strlen(icon)>=2) {
          weather_now = (icon[0] - 0x30)*10 + (icon[1] - 0x30);
        }
      }
      JsonArray hourly = doc["hourly"];
      if (hourly == NULL) return false;
      DEBUG_PRINTLN("[JSON] hourly");
      int hour = 0;
      int weather_predict = 1;
      for (JsonObject hd : hourly) {
        if (hd == NULL) continue;
        float temp = hd["temp"];
        if (temp < tempMin) tempMin = temp;
        if (temp > tempMax) tempMax = temp;
        JsonObject weather = hd["weather"][0];
        if (weather == NULL) continue;
        //DEBUG_PRINTFLN("[JSON] weather %d", hour);
        const char* icon = weather["icon"];
        if (icon && strlen(icon)>=2) {
          int w = (icon[0] - 0x30)*10 + (icon[1] - 0x30);
          if ((weather_12h < w) && (w <= 13)) {
            weather_12h = w;
          }
          if (hour >= 6) {
            if ((weather_predict < w) && (w <= 13)) {
              weather_predict = w;
              if (hour > 12) {
                if ((weather_now >= weather_12h) && (weather_predict < weather_12h)) {
                  weather_12h = weather_predict;
                }
              }
            }
          }
        }
        hour++;
      }
      DEBUG_PRINTLN("[JSON] readed.");
      //DEBUG_PRINTFLN("temp %d<%d<%d", (int)tempMin, (int)tempCurrent, (int)tempMax);
      return true;
    }
    else {
      DEBUG_PRINTLN("[JSON] parse error.");
      DEBUG_PRINTLN(error.c_str());
       delay(1000);
    }
  }
  else {
    http.end();
    DEBUG_PRINTFLN("[HTTP] ERROR %d", httpCode);
    DEBUG_PRINTFLN(" %s", http.errorToString(httpCode).c_str());
    delay(1000);
  }
  delay(300);
  return result;
}

//----------------------------------
const uint16_t* getWeatherIcon(int v) {
  // icon / 01=晴れ 02=曇り晴れ 03,04=曇り 09,10=雨 11=雷 13=雪 50=霧
  switch (v) {
    case 1:
      return weather_sunny;
    case 2:
      return weather_sun_and_cloud;
    case 3:
    case 4:
      return weather_cloudy;
    case 9:
    case 10:
      return weather_rainy;
    case 11:
      return weather_thunder;
    case 13:
      return weather_snowy;
  }
  return NULL;
}
//----------------------------------
void setNtpTimeToRTC() {
  // NTP サーバーから時刻を取得して RTC 設定
  DEBUG_PRINTLN("[NTP] Connecting...");
  configTime(9 * 3600, 0, ntpServer);
  struct tm timeInfo;
  if (getLocalTime(&timeInfo)) {
    RTC_TimeTypeDef TimeStruct;
    TimeStruct.Hours   = timeInfo.tm_hour;
    TimeStruct.Minutes = timeInfo.tm_min;
    TimeStruct.Seconds = timeInfo.tm_sec;
    M5.Rtc.SetTime(&TimeStruct);
    RTC_DateTypeDef DateStruct;
    DateStruct.WeekDay = timeInfo.tm_wday;
    DateStruct.Month = timeInfo.tm_mon + 1;
    DateStruct.Date = timeInfo.tm_mday;
    DateStruct.Year = timeInfo.tm_year + 1900;
    M5.Rtc.SetDate(&DateStruct);
    DEBUG_PRINTF("DATE: %02d/%02d/%02d\r\n", timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday);
    DEBUG_PRINTF("TIME: %02d:%02d:%02d\r\n", timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
  }
  else 
  {
    DEBUG_PRINTLN("[NTP] failed.");
  }
  delay(300);
}

//----------------------------------
void loadData() {
  preferences.begin("spdmeter", false);
  tripmeter = preferences.getDouble("tripmeter", 0.0);
  odometer = preferences.getDouble("odometer", 0.0);
  maxSpeed = preferences.getDouble("maxspeed", 0.0);
  tripKcal = preferences.getDouble("tripKcal", 0.0);
  elapsedTime = preferences.getDouble("elapsedTime", 0.0);
  preferences.end();
}

//----------------------------------
void saveData() {
  preferences.begin("spdmeter", false);
  preferences.putDouble("tripmeter", tripmeter);
  preferences.putDouble("odometer", odometer);
  preferences.putDouble("maxspeed", maxSpeed);
  preferences.putDouble("tripKcal", tripKcal);
  preferences.putDouble("elapsedTime", elapsedTime);
  preferences.end();
}

//----------------------------------
void drawDefault() {
  M5.Lcd.setCursor(97, 80);
  M5.Lcd.println("km/h");

  M5.Lcd.setCursor(15, 170);
  M5.Lcd.println("CLOCK");
  M5.Lcd.drawBitmap( 67, 188, 26, 35, ccolon);
}

//----------------------------------
void setup() {
  M5.begin();
  M5.Lcd.setRotation(0);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(1);  

  Serial.begin(115200);
  {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    M5.Lcd.setCursor(0, 10);
    DEBUG_PRINTLN("Wifi Connecting...");

    WiFi.begin(ssid, password);
    for (int i = 0; i < 60 && WiFi.status() != WL_CONNECTED; i++)
    {
        digitalWrite(LED_PIN, HIGH);
        delay(250);
        Serial.print(".");
        digitalWrite(LED_PIN, LOW);
        delay(250);
        M5.update();
        if(M5.BtnA.isPressed()){
          DEBUG_PRINTLN("\nAborted.");
          break;
        }
    }
    if (WiFi.status() == WL_CONNECTED) {
      DEBUG_PRINTLN("-");
      setNtpTimeToRTC();
      DEBUG_PRINTLN("-");
      getWeather(openweathermap_url);
      WiFi.disconnect(true);
    }
  }
  while (1) {
    delay(100);
    M5.update();
    if (M5.BtnA.isPressed() == false) {
      break;
    }
  }

  digitalWrite(LED_PIN, HIGH);
  
  WiFi.mode(WIFI_OFF);

  M5.Lcd.fillScreen(BLACK);

  drawDefault();
  
  if ((weather_now > 0) && (weather_12h > 0)) {
    M5.Lcd.drawBitmap( 10, 130, 23, 26, getWeatherIcon(weather_now));
    M5.Lcd.drawBitmap( 35, 130, 23, 26, weather_arrow);
    M5.Lcd.drawBitmap( 52, 130, 23, 26, getWeatherIcon(weather_12h));
    int minT = (int)(tempMin);
    int maxT = (int)(tempMax + 0.5);
    char s[16];
    sprintf(s, "%d / %d", minT, maxT);
    //M5.Lcd.drawCentreString(s, 42, 115, 1);
    M5.Lcd.setCursor(15, 115);
    M5.Lcd.println(s);
  }     
  loadData();
  
#if 0
  //手動 RTC 設定
  RTC_TimeTypeDef tm;
  tm.Hours = 23;
  tm.Minutes = 57;
  tm.Seconds = 0;
  M5.Rtc.SetTime(&tm);

  RTC_DateTypeDef dt;
  dt.WeekDay = 3;
  dt.Month = 9;
  dt.Date = 22;
  dt.Year = 2021;
  M5.Rtc.SetData(&dt);
#endif


  M5.Axp.ScreenBreath(scrBright);
  // バッテリー残量のパーセンテージ式の算出
  // 線分の傾きを計算
  bat_per_inclination = 100.0F/(bat_percent_max_vol-bat_percent_min_vol);
  // 線分の切片を計算
  bat_per_intercept = -1 * bat_percent_min_vol * bat_per_inclination;
    
  pinMode(26, INPUT_PULLUP);
  attachInterrupt(26, SensorTick, FALLING);
  pinMode(36, INPUT);
  gpio_pulldown_dis(GPIO_NUM_25);
  gpio_pullup_dis(GPIO_NUM_25);
  if (digitalRead(36) == HIGH) {
    securityMode = true;
  }

  // CPU速度を下げる
  while(!setCpuFrequencyMhz(10)){
    // nop
  }
}

//----------------------------------
const uint16_t* getSegData(int number, bool first) {
  switch (number) {
    case 0: {
      if (first) {
        return space;
      }
      return seg0;
    }
    case 1: return seg1;
    case 2: return seg2;
    case 3: return seg3;
    case 4: return seg4;
    case 5: return seg5;
    case 6: return seg6;
    case 7: return seg7;
    case 8: return seg8;
    case 9: return seg9;
  }
  return space;
}

//----------------------------------
const uint16_t* getClockSegData(int number) {
  switch (number) {
    case 1: return cseg1;
    case 2: return cseg2;
    case 3: return cseg3;
    case 4: return cseg4;
    case 5: return cseg5;
    case 6: return cseg6;
    case 7: return cseg7;
    case 8: return cseg8;
    case 9: return cseg9;
  }
  return cseg0;
}

//----------------------------------
void changeBrightness() {
    if (scrBright == 100) {
      scrBright = 20;
    }
    else if (scrBright == 20) {
      scrBright = 1;
    }
    else if (scrBright == 1) {
      scrBright = 100;
    }
    M5.Axp.ScreenBreath(scrBright); // 明るさを設定
}

//----------------------------------
void drawInfoHeader() {
    M5.Lcd.fillRect(0, 158, 135, 70, BLACK);
    if (mode == 0) {
      M5.Lcd.setCursor(15, 170);
      M5.Lcd.println("CLOCK");
      //M5.Lcd.drawCentreString("CLOCK", 67, 170, 1);
      M5.Lcd.drawBitmap( 68, 188, 26, 35, ccolon);      
    }
    if (mode == 1) {
      M5.Lcd.setCursor(15, 170);
      M5.Lcd.setTextColor(COLOR_ELAPSED);
      M5.Lcd.println("ELAPSED");
      M5.Lcd.drawBitmap( 68, 188, 26, 35, ccolon);      
    }
    if (mode == 2) {
      M5.Lcd.setCursor(15, 170);
      M5.Lcd.setTextColor(COLOR_TRIP);
      M5.Lcd.println("TRIP");
      M5.Lcd.setTextColor(WHITE);
    }
    if (mode == 3) {
      M5.Lcd.setCursor(15, 170);
      M5.Lcd.setTextColor(COLOR_ODO);
      M5.Lcd.println("ODO");
      M5.Lcd.setTextColor(WHITE);
    }
    if (mode == 4) {
      M5.Lcd.setCursor(15, 170);
      M5.Lcd.setTextColor(COLOR_MAXSPD);
      M5.Lcd.println("MAX");
      M5.Lcd.setCursor(100, 170);
      M5.Lcd.println("km/h");
      M5.Lcd.setTextColor(WHITE);
    }
    if (mode == 5) {
      M5.Lcd.setCursor(15, 170);
      M5.Lcd.setTextColor(COLOR_AVESPD);
      M5.Lcd.println("Ave.");
      M5.Lcd.setCursor(100, 170);
      M5.Lcd.println("km/h");
      M5.Lcd.setTextColor(WHITE);
    }
    if (mode == 6) {
      M5.Lcd.setCursor(15, 170);
      M5.Lcd.setTextColor(COLOR_BURN);
      M5.Lcd.println("Burn");
      M5.Lcd.setCursor(100, 170);
      M5.Lcd.println("kCal");
      M5.Lcd.setTextColor(WHITE);
    }
}

//----------------------------------
void drawInfo() {
  if (mode == 0) {
    // clock
    RTC_TimeTypeDef tm;
    M5.Rtc.GetTime(&tm);
    int hour = tm.Hours;
    int minute = tm.Minutes;
    M5.Lcd.drawBitmap(  8, 188, 26, 35, getClockSegData(hour/10));
    M5.Lcd.drawBitmap( 39, 188, 26, 35, getClockSegData(hour%10));
    M5.Lcd.drawBitmap( 75, 188, 26, 35, getClockSegData(minute/10));
    M5.Lcd.drawBitmap(104, 188, 26, 35, getClockSegData(minute%10));
  }
  else if (mode == 1) {
    // ElapsedTime
    M5.Lcd.setCursor(105, 170);
    M5.Lcd.setTextColor(COLOR_ELAPSED);
    int et = (int)elapsedTime;
    if (et >= 90*60) {
      et /= 60;
      M5.Lcd.println("MIN.");
    }
    else {
      M5.Lcd.println("SEC.");
    }
    int eh = et/60;
    int em = (et - eh*60);
    if (eh >= 10) {
      M5.Lcd.drawBitmap(  8, 188, 26, 35, getClockSegData(eh/10));
    }
    M5.Lcd.drawBitmap( 39, 188, 26, 35, getClockSegData(eh%10));
    M5.Lcd.drawBitmap( 75, 188, 26, 35, getClockSegData(em/10));
    M5.Lcd.drawBitmap(104, 188, 26, 35, getClockSegData(em%10));
  }
  else if ((mode == 2) || (mode == 3)) {
    // odometer or tripmeter
    double m = tripmeter;
    if (mode == 3) {
      m = odometer;
      M5.Lcd.setTextColor(COLOR_ODO);
    }
    else {
      M5.Lcd.setTextColor(COLOR_TRIP);
    }
    
    M5.Lcd.setCursor(110, 170);
    if (m < 1000) {
      M5.Lcd.println(" m");
    } else {
      M5.Lcd.println("km");
    }
    if (m < 1000) {
      // meters (    0m)
      // meters (0.000km)
      int meter = (int)m;
      M5.Lcd.drawBitmap( 96, 219 + 5, 5, 4, cleardot);
      //M5.Lcd.drawBitmap( 13, 188, 26, 35, getClockSegData(meter/1000));
      if (meter>=100) M5.Lcd.drawBitmap( 42, 188, 26, 35, getClockSegData((meter/100)%10));
      if (meter>=10) M5.Lcd.drawBitmap( 71, 188, 26, 35, getClockSegData((meter/10)%10));
      M5.Lcd.drawBitmap(100, 188, 26, 35, getClockSegData(meter%10));
      //M5.Lcd.drawBitmap( 38, 219 + 5, 5, 4, dot);
    }
    else if (m < 1000000) {
      // kilo-meters (000.0km)
      int m100 = (int)(m / 100);
      M5.Lcd.drawBitmap( 38, 219 + 5, 5, 4, cleardot);
      if (m100>=1000) M5.Lcd.drawBitmap( 13, 188, 26, 35, getClockSegData((m100/1000)%10));
      if (m100>=100) M5.Lcd.drawBitmap( 42, 188, 26, 35, getClockSegData((m100/100)%10));
      else M5.Lcd.fillRect(42, 188, 26, 35, BLACK); // 900m から 1.0km になった時に必要
      M5.Lcd.drawBitmap( 71, 188, 26, 35, getClockSegData((m100/10)%10));
      M5.Lcd.drawBitmap(100, 188, 26, 35, getClockSegData(m100%10));
      M5.Lcd.drawBitmap( 96, 219 + 5, 5, 4, dot);
    }
    else {
      int km = (int)(m / 1000);
      if (km >= 10000) km = 9999;
      M5.Lcd.drawBitmap( 38, 219 + 5, 5, 4, cleardot);
      M5.Lcd.drawBitmap( 13, 188, 26, 35, getClockSegData((km/1000)%10));
      M5.Lcd.drawBitmap( 42, 188, 26, 35, getClockSegData((km/100)%10));
      M5.Lcd.drawBitmap( 71, 188, 26, 35, getClockSegData((km/10)%10));
      M5.Lcd.drawBitmap(100, 188, 26, 35, getClockSegData(km%10));
    }
  }
  else if ((mode == 4) || (mode == 5)) {
    // max speed or average speed
    int spd10 = (int)(maxSpeed*10);
    if (mode == 5) {
      spd10 = 0;
      if (elapsedTime > 0) {
        double averageSpeed = ((tripmeter/1000.0) / (elapsedTime / 3600));
        spd10 = (int)(averageSpeed * 10);
      }
    }
    if (spd10 > 9999) spd10 = 9999;
    if (spd10 >= 1000) {
      M5.Lcd.drawBitmap( 13, 188, 26, 35, getClockSegData(spd10/1000));
    }
    if (spd10 >= 100) {
      M5.Lcd.drawBitmap( 42, 188, 26, 35, getClockSegData((spd10/100)%10));
    }
    M5.Lcd.drawBitmap( 71, 188, 26, 35, getClockSegData((spd10/10)%10));
    M5.Lcd.drawBitmap(100, 188, 26, 35, getClockSegData(spd10%10));
    M5.Lcd.drawBitmap( 94, 219 + 5, 5, 4, dot);
  }
  else if (mode == 6) {
    // kcal
    int kcal = tripKcal;
    if (kcal > 9999) kcal = 9999;
    if (kcal >= 1000) {
      M5.Lcd.drawBitmap( 13, 188, 26, 35, getClockSegData(kcal/1000));
    }
    if (kcal >= 100) {
      M5.Lcd.drawBitmap( 42, 188, 26, 35, getClockSegData((kcal/100)%10));
    }
    if (kcal >= 10) {
      M5.Lcd.drawBitmap( 71, 188, 26, 35, getClockSegData((kcal/10)%10));
    }
    M5.Lcd.drawBitmap(100, 188, 26, 35, getClockSegData(kcal%10));
  }
}

//----------------------------------
void drawBattery() {
  bat_vol = M5.Axp.GetVbatData() * 1.1 / 1000;   // V
  bat_per = bat_per_inclination * bat_vol + bat_per_intercept;    // %
  if(bat_per > 100.0){
      bat_per = 100.0;
  }
  //M5.Lcd.setCursor(10, 100);
  //M5.Lcd.printf("%1.1f V / %3.1f%%_", bat_vol, bat_per);

  // battery
  uint16_t color = WHITE;
  if (bat_per < 25) {
    color = RED;
  }
  M5.Lcd.fillRect(96, 141, 3, 5, color);
  M5.Lcd.fillRect(98, 136, 24, 15, color);
  M5.Lcd.fillRect(100, 138, 20, 11, BLACK);

  M5.Lcd.fillRect(100, 138, 20, 11, BLACK);
  if (bat_per >= 75) {
    M5.Lcd.fillRect(102, 140, 4, 7, WHITE);
  }
  if (bat_per >= 50) {
    M5.Lcd.fillRect(108, 140, 4, 7, WHITE);
  }
  if (bat_per >= 25) {
    M5.Lcd.fillRect(114, 140, 4, 7, WHITE);
  }
}

//----------------------------------
void drawSpeed() {
  int spd = (int)speed;
  M5.Lcd.drawBitmap(14, 10, 48, 67, getSegData(spd/10, true));
  M5.Lcd.drawBitmap(71, 10, 48, 67, getSegData(spd%10, false));
}

const int sleepTime = 700;
const int buttonCheckTime = 100;
//----------------------------------
void loop() {
    // 停車時 LED 点滅
  if ((securityMode) && (speed < 0.1)) {
    if (stopTimeCounter >= 0) {
      stopTimeCounter += sleepTime;
    }
    if (stopTimeCounter > 10*1000) {
      stopTimeCounter = -1;
      M5.Axp.ScreenBreath(0); // 明るさを設定
    }

    bool key = digitalRead(36);
    if (key == HIGH) {
      counter = (counter + 1) % 5;
      if (counter == 0) {
        digitalWrite(LED_PIN, LOW);
      }
    }
    else {
      M5.Axp.ScreenBreath(15); // 明るさを設定
      digitalWrite(LED_PIN, LOW);
      M5.Lcd.fillScreen(RED);
      M5.Lcd.setTextColor(BLACK);
      // センサー引き抜きアラーム
      while (1) {
        digitalWrite(LED_PIN, LOW);
        M5.Lcd.fillScreen(RED);
        M5.Beep.tone(3000);
        delay(300);
        digitalWrite(LED_PIN, HIGH);
        M5.Lcd.drawCentreString("ALERT", 67, 120, 4);
        M5.Beep.tone(2000);
        delay(300);
        M5.update();
        if (M5.BtnB.pressedFor(3000) || (digitalRead(36) == HIGH)) {
          M5.Axp.ScreenBreath(scrBright); // 明るさを設定
          stopTimeCounter = 0;
          M5.Beep.mute();
          M5.Lcd.fillScreen(BLACK);
          M5.Lcd.setTextColor(WHITE);
          drawDefault();
          drawInfoHeader();
          if (M5.BtnB.isPressed()) {
            securityMode = false;
          }
          while (1) {
            M5.update();
            if (M5.BtnB.isPressed() == false) {
              break;
            }
          }
          break;
        }
      }
    }
  }

  if (false) {
    // これをループ
    //M5.Beep.mute();
  }

  // キー操作
  M5.update();

  if (M5.BtnB.pressedFor(3000)) {
    M5.Axp.ScreenBreath(scrBright); // 明るさを設定
    stopTimeCounter = 0;
    M5.Beep.tone(3300);
    delay(40);
    M5.Beep.mute();
    if (mode == 0) {
      securityMode = !securityMode;
    }
    else {
      tripmeter = 0;
      maxSpeed = 0;
      tripKcal = 0;
      elapsedTime = 0;
      saveData();
    }
    drawInfoHeader();
    while (1) {
      delay(100);
      M5.update();
      if (M5.BtnB.isReleased()) break;
    }
  }
  // 電源ボタン
  if(M5.Axp.GetBtnPress() == 1){
    changeBrightness();
    stopTimeCounter = 0;
  }

  // 走行→停車 の際に FLASH メモリに状態を保存
  if (speed > 0.1) {
    if (stopTimeCounter < 0) {
      // 走行開始
      M5.Axp.ScreenBreath(scrBright); // 明るさを設定
      startTime = millis();
    }
    isRun = true;
    stopTimeCounter = 0;
  }
  else {
    if (isRun) {
      saveData();
      stopTimeCounter = 0;
    }
    isRun = false;
  }

  drawSpeed();
  drawInfo();
  drawBattery();
  
  digitalWrite(LED_PIN, HIGH);

  unsigned long t1 = millis();
  long t = t1 - t0;
  if (t > 3000) {
    speed = 0;
  }

  for (int i = 0; i < sleepTime; i+=buttonCheckTime) {
    delay(buttonCheckTime);
    M5.update();
    if (M5.BtnA.isPressed()) {
      if (stopTimeCounter < 0) {
        M5.Axp.ScreenBreath(scrBright); // 明るさを設定
      }
      else {
        mode = (mode + 1) % 7;
        drawInfoHeader();
        drawInfo();
      }
      stopTimeCounter = 0;
      while (1) {
        M5.update();
        if (M5.BtnA.isPressed() == false) {
          break;
        }
        delay(50);
      }
      break;
    }
  }
}

//----------------------------------
static void SensorTick() 
{
  unsigned long t1 = millis();
  long t = t1 - t0;

  if (isRun) {
    elapsedTime += (t1 - startTime) / 1000.0;
    startTime = t1;
  }
  
  double lastSpeed = tempSpeed;
  if (t > 3000) {
    speed = 0;
    tempSpeed = 0;
    t0 = t1;
  }
  else if (t >= 50) {
    double spd = (BICYCLE_CIRCUMFERENCE * 3600) / t;

    //if (abs(spd - speed) < 30) {
      if (spd > 99) {
        spd = 99;
      }

      tempSpeed = spd;
      // 2回計測のうち小さい方で最大速度を取る（１回の計測ミスを反映しないため）
      if (tempSpeed < lastSpeed) {
        if (tempSpeed > maxSpeed) {
          maxSpeed = tempSpeed;
        }
        speed = tempSpeed;
      }
      else {
        if (lastSpeed > maxSpeed) {
          maxSpeed = lastSpeed;
        }
        speed = lastSpeed;
      }
    //}
    t0 = t1;
    odometer += BICYCLE_CIRCUMFERENCE;
    tripmeter += BICYCLE_CIRCUMFERENCE;
    double mets = (0.38636 + 0.04 * ((spd/1.6)-11)/4) * spd;
    if (mets < 0) mets = 0;
    //tripKcal += BODY_WEIGHT * mets * (t / 1000.0 / 3600.0); //消費エネルギー量(kcal)＝体重(kg)×METS数×運動時間(h)×1.05(kcal/METS/kg/h)
    tripKcal += 0.00001666666 * t * mets;
  }
}

//----------------------------------
// 0.19(19cm) * 6.283 = 1.1937m / t(200ms)
// 1.1937m * (1000/200) = 5.9685m / s
// 21486.6m / h
// 21.486km/h

// 1.1937 * (1000/t) * 3600 / 1000
// 1.1937 * (1000/t) * 3.6
// 4.29732 * (1000/t)
// 4297.32 / t
