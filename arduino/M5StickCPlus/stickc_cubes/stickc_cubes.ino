#include <M5Unified.h>
#include <Kalman.h>
#include "3dmath.h"

#include <vector>
#include <algorithm>
#define M_PIf     (3.14159265358979323846f) /* pi */
#define M_halfPIf (3.14159265358979323846f / 2.0f);
/*
<- Front    0 1 2    9 10 11  18 19 20
            3 4 5   12 13 14  21 22 23   (13 is stable)
            6 7 8   15 16 17  24 25 26  -> Back
*/

const Vector4 box_points[8] = {
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(-1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(-1.0f, -1.0f, 1.0f, 1.0f),
  Vector4(1.0f, -1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, -1.0f, 1.0f),
  Vector4(-1.0f, 1.0f, -1.0f, 1.0f),
  Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
  Vector4(1.0f, -1.0f, -1.0f, 1.0f),
};
const int box_polygons[12*3] = {
  0,1,2,
  2,3,0,
  7,6,5,
  5,4,7,
  1,5,6,
  6,2,1,
  4,0,3,
  3,7,4,
  3,2,6,
  6,7,3,
  4,5,1,
  1,0,4,
};
const int lines[6*4] = {
  0,1,2,3,
  4,0,3,7,
  5,4,7,6,
  1,5,6,2,
  5,1,0,4,
  7,3,2,6,
};

struct Cube {
  Matrix44 mat;
  uint16_t color[6];
};

static M5Canvas canvas(&M5.Lcd);
static int counter = 0;
static Cube cubes[27];
static Vector4 points[27*12*3];
static Vector4 line_points[8];
static uint16_t colors[27*12];
static std::vector<int> polygon_order;
const float cubeSize = 0.75f;
static Matrix44 viewMatrix;
static Matrix44 projectionMatrix;
static Quaternion qRot;
static float screenWidth = 128.0f;
static float screenHeight = 128.0f;

void InitCubes() {
  for (int i = 0; i < 27; i++) {
    int z = (i/9)-1;
    int y = ((i/3)%3)-1;
    int x = (i%3)-1;
    cubes[i].mat.SetTrans(cubeSize * x, cubeSize * y, cubeSize * z);
    for (int j = 0; j < 6; j++) {
      int r = rand()%256;
      int g = rand()%256;
      int b = rand()%256;
      uint16_t col = TFT_BLACK;
      switch (j) {
        case 0:
          if (z == 1) col = TFT_RED;
          break;
        case 1:
          if (z == -1) col = M5.Lcd.color565(255, 100, 0);
          break;
        case 2:
          if (x == -1) col = TFT_YELLOW;
          break;
        case 3:
          if (x == 1) col = M5.Lcd.color565(0xE0, 0xE0, 0xE0);
          break;
        case 4:
          if (y == -1) col = M5.Lcd.color565(0, 40, 255);
          break;
        case 5:
          if (y == 1) col = M5.Lcd.color565(0, 160, 0);
          break;
      }
      //cubes[i].color[j] = static_cast<uint16_t>((b & 0x1F) | (g & 0x3F) << 5 | (r & 0x1F) << 11);
      cubes[i].color[j] = col;
      if (i == 13) {
        cubes[i].color[j] = TFT_BLACK;
      }
    }
  }  
  polygon_order.reserve(27*12);
}
void Update(Matrix44 mat) {
  int k = 0;
  int l = 0;
  polygon_order.clear();
  for (int i = 0; i < 27; i++) {
    auto& cube = cubes[i];
    if (i == 13) continue;

    float halfCubeSize = cubeSize * 0.5f;
    for (int j = 0; j < 12; j++) {
      int i0 = box_polygons[j*3+0];
      int i1 = box_polygons[j*3+1];
      int i2 = box_polygons[j*3+2];
      Matrix44 m = viewMatrix * mat * cube.mat;
      points[k++] = m * (box_points[i0] * halfCubeSize * 0.9f);
      points[k++] = m * (box_points[i1] * halfCubeSize * 0.9f);
      points[k++] = m * (box_points[i2] * halfCubeSize * 0.9f);
      polygon_order.push_back(l);
      colors[l++] = cube.color[j/2];
    }
  }
  {
    Matrix44 m = viewMatrix * mat;
    for (int i = 0; i < 8; i++) {
      Vector4 v = box_points[i];
      v = m * (v * 1.5f * cubeSize);
      line_points[i] = Matrix44::ProjectTo2D(v, projectionMatrix, screenWidth, screenHeight);
    }
  }

  // Z位置順にソート
  std::sort(polygon_order.begin(), polygon_order.end(),
    [points](int a, int b) -> bool {
      const Vector4& va0 = points[a*3+0];
      const Vector4& va1 = points[a*3+1];
      const Vector4& va2 = points[a*3+2];
      float da = va0.z + va1.z + va2.z;
      const Vector4& vb0 = points[b*3+0];
      const Vector4& vb1 = points[b*3+1];
      const Vector4& vb2 = points[b*3+2];
      float db = vb0.z + vb1.z + vb2.z;
        return da < db;
    });

  for (int j = 0; j < k; j++) {
    points[j].w = 1.0f;
    points[j] = Matrix44::ProjectTo2D(points[j], projectionMatrix, screenWidth, screenHeight);
    points[j].w = 1.0f;
  }
}

void RotateX(int lineNum, float angle) {
  Quaternion qX = Quaternion::Rotate(angle, Vector4(1.0f, 0.0f, 0.0f, 0.0f));
  Matrix44 mat;
  mat.FromQuaternion(qX);

  for (int i = 0; i < 27; i++) {
    auto& cube = cubes[i];
    if (i == 13) continue;
    Vector4 pos = Vector4(cube.mat.GetTransX(), cube.mat.GetTransY(), cube.mat.GetTransZ(), 1.0f);
    if (abs(pos.x + cubeSize * lineNum) < 0.01f) {
      Matrix44 tm0, tm1;
      tm0.SetTrans(pos.x, pos.y, pos.z);
      tm1.SetTrans(-pos.x, -pos.y, -pos.z);
      pos = mat * pos;
      cube.mat = mat * (tm1 * cube.mat);
      cube.mat.SetTrans(pos.x, pos.y, pos.z);
    }
  }
}

void RotateY(int lineNum, float angle) {
  Quaternion qY = Quaternion::Rotate(angle, Vector4(0.0f, 1.0f, 0.0f, 0.0f));
  Matrix44 mat;
  mat.FromQuaternion(qY);

  for (int i = 0; i < 27; i++) {
    auto& cube = cubes[i];
    if (i == 13) continue;
    Vector4 pos = Vector4(cube.mat.GetTransX(), cube.mat.GetTransY(), cube.mat.GetTransZ(), 1.0f);
    if (abs(pos.y + cubeSize * lineNum) < 0.01f) {
      Matrix44 tm0, tm1;
      tm0.SetTrans(pos.x, pos.y, pos.z);
      tm1.SetTrans(-pos.x, -pos.y, -pos.z);
      pos = mat * pos;
      cube.mat = mat * (tm1 * cube.mat);
      cube.mat.SetTrans(pos.x, pos.y, pos.z);
    }
  }
}

void RotateZ(int lineNum, float angle) {
  Quaternion qZ = Quaternion::Rotate(angle, Vector4(0.0f, 0.0f, 1.0f, 0.0f));
  Matrix44 mat;
  mat.FromQuaternion(qZ);

  for (int i = 0; i < 27; i++) {
    auto& cube = cubes[i];
    if (i == 13) continue;
    Vector4 pos = Vector4(cube.mat.GetTransX(), cube.mat.GetTransY(), cube.mat.GetTransZ(), 1.0f);
    if (abs(pos.z + cubeSize * lineNum) < 0.01f) {
      Matrix44 tm0, tm1;
      tm0.SetTrans(pos.x, pos.y, pos.z);
      tm1.SetTrans(-pos.x, -pos.y, -pos.z);
      pos = mat * pos;
      cube.mat = mat * (tm1 * cube.mat);
      cube.mat.SetTrans(pos.x, pos.y, pos.z);
    }
  }
}

float acc[3];
float accOffset[3];
float gyro[3];
float gyroOffset[3];

float kalAngleX;
float kalAngleY;
Kalman kalmanX;
Kalman kalmanY;

long lastMs = 0;
long tick = 0;
float posX = 0;
float posY = 0;
int spinSpeed = 0;
int spinMode = -1; // 0,1,2:X 3,4,5:Y 6,7,8:Z
float spinAngle = 0.0f;

void sensorUpdate() {
  M5.Imu.getGyroData(&gyro[0], &gyro[1], &gyro[2]);
  M5.Imu.getAccelData(&acc[0], &acc[1], &acc[2]);
}

float getRoll(){
  return atan2(acc[1], acc[2]) * RAD_TO_DEG;
}
float getPitch(){
  return atan(-acc[0] / sqrt(acc[1]*acc[1] + acc[2]*acc[2])) * RAD_TO_DEG;
}

void calibration(){
  //補正値を求める
  float gyroSum[3];
  float accSum[3];
  for(int i = 0; i < 500; i++){
    sensorUpdate();
    gyroSum[0] += gyro[0];
    gyroSum[1] += gyro[1];
    gyroSum[2] += gyro[2];
    accSum[0] += acc[0];
    accSum[1] += acc[1];
    accSum[2] += acc[2];
    delay(2);
  }
  gyroOffset[0] = gyroSum[0]/500;
  gyroOffset[1] = gyroSum[1]/500;
  gyroOffset[2] = gyroSum[2]/500;
  accOffset[0] = accSum[0]/500;
  accOffset[1] = accSum[1]/500;
  accOffset[2] = accSum[2]/500 - 1.0;//重力加速度1G
}

void applyCalibration(){
  gyro[0] -= gyroOffset[0];
  gyro[1] -= gyroOffset[1];
  gyro[2] -= gyroOffset[2];
  acc[0] -= accOffset[0];
  acc[1] -= accOffset[1];
  acc[2] -= accOffset[2];
}


void setup() {
  // put your setup code here, to run once:
  M5.begin();
  M5.Imu.init();

  // カメラ設定
  Vector4 eye(0.0f, 5.0f, 0.0f, 1.0f);  // カメラ位置
  Vector4 target(0.0f, 0.0f, 0.0f, 1.0f);  // カメラの注視点
  //Vector4 up(0.0f, 1.0f, 0.0f, 0.0f);  // カメラの上方向
  Vector4 up(0.0f, 0.0f, -1.0f, 0.0f);  // カメラの上方向

  viewMatrix = Matrix44::LookAt(eye, target, up);
  qRot = Quaternion();

  // スクリーンサイズ
  screenWidth = 128.0f;
  screenHeight = 128.0f;

  if (M5.getBoard() == m5::board_t::board_M5StickCPlus || M5.getBoard() == m5::board_t::board_M5StickCPlus2) {
    screenWidth = 240.0f;
    screenHeight = 135.0f;
    pinMode(26, OUTPUT_OPEN_DRAIN);
  }
  // 投影設定
  float fov = 50.0f * (PI / 180.0f);  // 視野角 (ラジアン)
  float aspect = screenWidth / screenHeight;  // アスペクト比
  float near = 0.1f;  // ニアクリップ面
  float far = 100.0f;  // ファークリップ面
  projectionMatrix = Matrix44::Perspective(fov, aspect, near, far);

  InitCubes();

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextSize(2);

  canvas.setColorDepth(16);
  canvas.createSprite((int)screenWidth, (int)screenHeight);

  calibration();
  sensorUpdate();
  kalmanX.setAngle(getRoll());
  kalmanY.setAngle(getPitch());
  lastMs = micros();
}

void loop() {
  canvas.fillScreen(TFT_BLACK);

  counter++;
  sensorUpdate();
  applyCalibration();
  float dt = (micros() - lastMs) / 1000000.0;
  lastMs = micros();
  float roll = getRoll();
  float pitch = getPitch();
  
  kalAngleX = kalmanX.getAngle(roll, gyro[0], dt);
  kalAngleY = kalmanY.getAngle(pitch, gyro[1], dt);

  posX -= kalAngleX * 0.1f;
  posY += kalAngleY * 0.1f;

  // 背景のグリッド
  int ofsX = (int)(posX) % 30;
  int ofsY = (int)(posY) % 30;
  const uint16_t darkGrey = M5.Lcd.color565(32, 60, 32);
  int b = (int)screenWidth;
  if (b < (int)screenHeight) b = (int)screenHeight;
  for (int i = 0; i < (b/30)+2; i++) {
    int x = i*30+ofsX;
    int y = i*30+ofsY;
    canvas.drawLine(x, 0, x, (int)screenHeight, darkGrey);
    canvas.drawLine(0, y, (int)screenWidth, y, darkGrey);
  }

  // 座標系を回転
  Quaternion qX = Quaternion::Rotate(-kalAngleX / 360.0f * 6.283f * 0.1f, Vector4(0.0f, 0.0f, 1.0f, 0.0f));
  Quaternion qY = Quaternion::Rotate(-kalAngleY / 360.0f * 6.283f * 0.1f, Vector4(1.0f, 0.0f, 0.0f, 0.0f));
  Quaternion q = qX * qY;
  qRot = q * qRot;

  // 頂点の回転
  Matrix44 mat;
  mat.FromQuaternion(qRot);

  // オブジェクト更新
  Update(mat);

  // 描画
  for (auto it = polygon_order.begin(); it != polygon_order.end(); ++it) {
    int j = *it;
    const Vector4& p0 = points[j*3+0];
    const Vector4& p1 = points[j*3+1];
    const Vector4& p2 = points[j*3+2];
    bool cw = Vector4::IsClockwise(p0, p1, p2);
    if (cw) {
      canvas.fillTriangle((int)p0.x, p0.y, (int)p1.x, p1.y, p2.x, p2.y, colors[j]);
    }
  }
#if 0
  for (int i = 0; i < 6; i++) {
    int i0 = lines[i*4+0];
    int i1 = lines[i*4+1];
    int i2 = lines[i*4+2];
    int i3 = lines[i*4+3];
    const Vector4& p0 = line_points[i0];
    const Vector4& p1 = line_points[i1];
    const Vector4& p2 = line_points[i2] - p1;
    const Vector4& p3 = line_points[i3] - p0;
    bool cw = Vector4::IsClockwise(p0, p1, p2);
    if (cw) {
      Vector4 pp0 = p0;
      Vector4 pp1 = p1;
      canvas.drawLine((int)p0.x, (int)p0.y, (int)p1.x, (int)p1.y, TFT_BLACK);
      pp0 = pp0 + p3 * 0.333333f;
      pp1 = pp1 + p2 * 0.333333f;
      canvas.drawLine((int)pp0.x, (int)pp0.y, (int)pp1.x, (int)pp1.y, TFT_BLACK);
      pp0 = pp0 + p3 * 0.333333f;
      pp1 = pp1 + p2 * 0.333333f;
      canvas.drawLine((int)pp0.x, (int)pp0.y, (int)pp1.x, (int)pp1.y, TFT_BLACK);
    }
  }
#endif
  canvas.pushSprite(0, 0);

  M5.update();

  if (M5.BtnA.wasClicked()) {
    spinSpeed = (spinSpeed+1)%4; // 0,1,2,3
  }
  if (spinSpeed > 0) {
    if (spinMode < 0) {
      spinMode = rand() % 9;
      spinAngle = M_halfPIf;
    }
  }
  if (spinMode >= 0) {
    float speed = 0.05f;
    if (spinSpeed == 2) {
      speed = 0.1f;
    }
    else if (spinSpeed == 3) {
      speed = 0.5f;
    }
    float angle = (spinAngle > speed) ? speed : spinAngle;
    spinAngle -= angle;
    switch (spinMode) {
      case 0:
      case 1:
      case 2:
        RotateX(spinMode-1, angle);
        break;
      case 3:
      case 4:
      case 5:
        RotateY(spinMode-4, angle);
        break;
      case 6:
      case 7:
      case 8:
        RotateZ(spinMode-7, angle);
        break;
    }
    if (spinAngle <= 0.0f) {
      spinMode = -1;
    }
  }
  delay(16);
}
