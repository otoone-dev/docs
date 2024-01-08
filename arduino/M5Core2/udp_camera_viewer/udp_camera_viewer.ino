#include <M5Unified.h>
#include <WiFi.h>
#include <AsyncUDP.h>
#include <string>
#include <malloc.h>
#include <SD.h>

#define ENABLE_SDCARD (1)

#define PORTNUM (11765)

#define APMODE (1)
#if APMODE
// Camera と直接接続する
const char* ssid = "MTCF110"; // SSID, Password を Camera 側と合わせる
const char* password = "6F99z1qm_8yp@m"; // 数字の１
#else
// 自宅等の WiFi router 経由で接続 (非推奨)
const char* ssid = "ssid"; // 自宅等の SSID, Password を入れる
const char* password = "pass";
#endif

std::string ipaddr = "";
bool show_info = false;

#define BUFF_SIZE (320*480)

#if ENABLE_SDCARD
File file;
static int fileCount = 0;
static int blackFrame = 0;
#endif

//-------------------
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setBrightness(255);
  M5.Display.setRotation(1);
  M5.Display.fillScreen(WHITE);
  delay(300);
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(WHITE, BLACK);
  M5.Display.setTextSize(1);

#if 0
  auto dt = M5.Rtc.getDateTime();
  dt.date.year = 2023; //時刻を手動設定する場合
  dt.date.month = 11;
  dt.date.date = 24;
  dt.time.hours = 0;
  dt.time.minutes = 0;
  dt.time.seconds = 0;
  M5.Rtc.setDateTime(dt);
#endif

#if ENABLE_SDCARD
  while (false == SD.begin(GPIO_NUM_4, SPI, 15000000)) {
    Serial.println("SD Wait...");
    delay(500);
  }
#endif

  Serial.begin(115200);
  Serial.println("serial start");

  WiFi.mode(WIFI_AP);

#if APMODE
  WiFi.softAP(ssid, password);
  ipaddr = WiFi.softAPIP().toString().c_str();
#else
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.printf(".");
  }
  Serial.printf("\n");
  ipaddr = WiFi.localIP().toString().c_str();
#endif
}

//-------------------
struct Frame {
  int _frameCount = -1;
  int _imgSize;
  bool _received[256];
  uint8_t* _buffer;
  bool _rendered;

  Frame() {
    Reset();
    _buffer = reinterpret_cast<uint8_t*>(malloc(BUFF_SIZE));
  }
  int FrameCount() { return _frameCount; }
  void Reset() {
    for (int i = 0; i < 256; i++) {
      _received[i] = false;
    }
    _imgSize = 0;
    _rendered = false;
  }
  void Write(int frameCount, int seqNum, int seqTotal, const uint8_t* data, int size, uint8_t sum) {
    if (frameCount != _frameCount) {
      Reset();
      _frameCount = frameCount;
    }
    uint8_t _sum = 0;
    for (int i = 0; i < size; i++) {
      int p = seqNum * 1024 + i;
      if (p >= BUFF_SIZE) {
        Serial.printf("buffer overrun (seqNum=%d/%d, size=%d)\n", seqNum, seqTotal, size);
        break;
      }
      uint8_t d = data[i];
      _buffer[p] = d;
      _imgSize++;
      _sum += d;
    }
    if (_sum == sum) {
      _received[seqNum] = true;
    }
    Disp(seqTotal);
  }
  void Disp(int seqTotal) {
    if (_rendered) {
      return;
    }
    for (int i = 0; i < seqTotal; i++) {
      if (!_received[i]) {
        return;
      }
    }
#if ENABLE_SDCARD
    if (blackFrame > 0) {
      M5.Display.fillScreen(BLACK);
      blackFrame--;
      if (blackFrame == 0) {
        char s[32];
        sprintf(s, "test%04d.jpg", fileCount);
        file = SD.open(s, FILE_WRITE);
        if (file) {
          file.write(_buffer, _imgSize);
          file.close();
        }
      }
    } else
#endif
    {
      M5.Display.startWrite();
      M5.Display.drawJpg(_buffer, _imgSize, 0, 0, 320, 240);
      if (show_info) {
        auto dt = M5.Rtc.getDateTime();
        M5.Display.setCursor(10, 10);
        M5.Display.printf("RTC : %04d/%02d/%02d  %02d:%02d:%02d", dt.date.year, dt.date.month, dt.date.date, dt.time.hours, dt.time.minutes, dt.time.seconds);
      }
      M5.Display.endWrite();
      _rendered = true;
    }
  }
};

//-------------------
void loop() {
  AsyncUDP udp;
  M5.Display.fillScreen(BLACK);
  M5.Display.setCursor(10, 10);
  M5.Display.printf("Waiting...\n");
  M5.Display.printf("%s:%d\n", ipaddr.c_str(), PORTNUM);

  Frame frame[2];
  int lastWriteBufferIndex = 1;

  if(udp.listen(PORTNUM)) {
    Serial.printf("UDP Listening on IP: %s (%d)\n", ipaddr.c_str(), PORTNUM);
    udp.onPacket([&](AsyncUDPPacket packet) {
        int len = packet.length();
        const uint8_t *buff = reinterpret_cast<uint8_t*>(packet.data());
        if (len >= 7) {
          uint8_t frameCount = *buff++;
          uint8_t seqNum = *buff++;
          uint8_t seqTotal = *buff++;
          uint8_t sum = *buff++;
          int d0 = *buff++;
          int d1 = *buff++;
          int dataSize = d0 | (d1 << 8);
          //Serial.printf("recv %df (%d/%d) %d, sum=%02x\n", frameCount, seqNum, seqTotal, dataSize, sum);

          if (frame[0].FrameCount() == frameCount) {
            frame[0].Write(frameCount, seqNum, seqTotal, buff, dataSize, sum);
          }
          else if (frame[1].FrameCount() == frameCount) {
            frame[1].Write(frameCount, seqNum, seqTotal, buff, dataSize, sum);
          }
          else {
            lastWriteBufferIndex = 1 - lastWriteBufferIndex;
            frame[lastWriteBufferIndex].Write(frameCount, seqNum, seqTotal, buff, dataSize, sum);
          }
        }
        else if (len == 1) {
          uint8_t status = *buff;
          if (show_info) {
            M5.Display.startWrite();
            M5.Display.setCursor(10, 30);
            if (status == 0xEF) {
              // Sended 
              M5.Display.printf("Success______");
            }
            else if (status == 0xEE) {
              // Camera Fail
              M5.Display.printf("Failure______");
            }
            M5.Display.endWrite();
          }
        }
        else {
          if (show_info) {
            M5.Display.startWrite();
            M5.Display.setCursor(10, 30);
            M5.Display.printf("Receive Error");
            M5.Display.endWrite();
          }
        }
#if 0
            Serial.print("UDP Packet Type: ");
            Serial.print(packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast");
            Serial.print(", From: ");
            Serial.print(packet.remoteIP());
            Serial.print(":");
            Serial.print(packet.remotePort());
            Serial.print(", To: ");
            Serial.print(packet.localIP());
            Serial.print(":");
            Serial.print(packet.localPort());
            Serial.print(", Length: ");
            Serial.print(packet.length());
            Serial.println();
#endif
    });
  }
  while (1) {
    M5.update();
    if (M5.Touch.isEnabled()) {
      auto t = M5.Touch.getDetail();
      auto x = M5.Touch.getTouchPointRaw().x;
      auto y = M5.Touch.getTouchPointRaw().y;
      if (t.wasClicked() && (y > 230)) {
        if (x < 120) {
          show_info = !show_info;
        }
        else if (x < 240) {

        }
        else {
#if ENABLE_SDCARD
          blackFrame = 5;
#endif
        }
      }
    }
    delay(200);
  }
}
