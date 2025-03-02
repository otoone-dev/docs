#include <M5Unified.h>

static M5Canvas canvas(&M5.Lcd);

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  canvas.setColorDepth(24);
  canvas.createSprite(M5.Lcd.width(), M5.Lcd.height());
}

//-------------------------------------
class Vector4f {
public:
  float x;
  float y;
  float z;
  float w;
  Vector4f() 
  {
    x = 0;
    y = 0;
    z = 0;
    w = 0;
  }
  Vector4f(float _x, float _y, float _z, float _w = 0.0f) {
    x = _x;
    y = _y;
    z = _z;
    w = _w;
  }
  Vector4f operator+ (Vector4f& other) {
    return Vector4f(x + other.x, y + other.y, z + other.z, w);
  }
  Vector4f operator- (Vector4f& other) {
    return Vector4f(x - other.x, y - other.y, z - other.z, w);
  }
};

class Matrix44f {
public:
  float m[4][4];

  Matrix44f::Matrix44f() {
    *this = Matrix44f::Identity();
  }

  static Matrix44f Identity() {
    Matrix44f result;
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        result.m[y][x] = ((x == y && x < 3) ? 1.0f : 0.0f);
      }
    }
    return result;
  }

  Matrix44f operator* (Matrix44f& other) {
    Matrix44f result;
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        float v = 0;
        for (int a = 0; a < 4; a++) {
          v += m[y][a] * other.m[a][x];
        }
        result.m[y][x] = v;
      }
    }
    return result;
  }
  Vector4f operator* (Vector4f& other) {
    Vector4f result;
    int i = 0;
    result.x = other.x * m[0][0] + other.y * m[0][1] + other.z * m[0][2] + other.w * m[0][3];
    result.y = other.x * m[1][0] + other.y * m[1][1] + other.z * m[1][2] + other.w * m[1][3];
    result.z = other.x * m[2][0] + other.y * m[2][1] + other.z * m[2][2] + other.w * m[2][3];
    result.w = other.x * m[3][0] + other.y * m[3][1] + other.z * m[3][2] + other.w * m[3][3];
    return result;
  }
};

//-------------------------------------
void loop() {
  canvas.fillScreen(BLACK);

  unsigned long t0 = micros();
  {
    for (int i = 0; i < 100; i++) {
      canvas.fillTriangle(rand()%128, rand()%128, rand()%128, rand()%128, rand()%128, rand()%128, rand()%2==0 ? TFT_RED : TFT_BLUE);
    }
  }
  int td = static_cast<int>(micros() - t0);
  char s[32];
  Vector4 v0 = Vector4f(1.0f, 2.0f, 3.0f);
  Vector4 v1 = Vector4f(2.0f, 3.0f, 4.0f);
  v0 = v0 + v1;
  sprintf(s, "%1.1f, %1.1f, %1.1f", v0.x, v0.y, v0.z);
  //sprintf(s, "%d us", td);
  //sprintf(s, "%d", ana);
  canvas.drawString(s, 10, 10, 2);

  canvas.pushSprite(0, 0);

  int tdms = static_cast<int>(micros() - t0) / 1000.0f;
  if (tdms > 31.0f) {
    delay(1);
  }
  else {
    delay(33 - tdms); // 30fps
  }
}
