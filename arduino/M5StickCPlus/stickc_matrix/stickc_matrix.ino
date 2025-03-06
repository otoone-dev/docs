#include <M5Unified.h>
#include <Kalman.h>
#include "3dmath.h"

#include <vector>
#include <algorithm>

static M5Canvas canvas(&M5.Lcd);
static int counter = 0;

    // カメラ設定
    Vector4 eye(0.0f, 5.0f, 0.0f, 1.0f);  // カメラ位置
    Vector4 target(0.0f, 0.0f, 0.0f, 1.0f);  // カメラの注視点
    //Vector4 up(0.0f, 1.0f, 0.0f, 0.0f);  // カメラの上方向
    Vector4 up(0.0f, 0.0f, -1.0f, 0.0f);  // カメラの上方向

    Matrix44 viewMatrix = Matrix44::LookAt(eye, target, up);
    Quaternion qRot = Quaternion();

    // スクリーンサイズ
    float screenWidth = 128.0f;
    float screenHeight = 128.0f;

    // 投影設定
    float fov = 50.0f * (PI / 180.0f);  // 視野角 (ラジアン)
    float aspect = screenWidth / screenHeight;  // アスペクト比
    float near = 0.1f;  // ニアクリップ面
    float far = 100.0f;  // ファークリップ面
    Matrix44 projectionMatrix = Matrix44::Perspective(fov, aspect, near, far);

    // 投影する3Dポイント
    // 0 1  --  4 5 
    // 3 2  --  7 6
    Vector4 points[8] = {
      Vector4(1.0f, 1.0f, 1.0f, 1.0f),
      Vector4(-1.0f, 1.0f, 1.0f, 1.0f),
      Vector4(-1.0f, -1.0f, 1.0f, 1.0f),
      Vector4(1.0f, -1.0f, 1.0f, 1.0f),
      Vector4(1.0f, 1.0f, -1.0f, 1.0f),
      Vector4(-1.0f, 1.0f, -1.0f, 1.0f),
      Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
      Vector4(1.0f, -1.0f, -1.0f, 1.0f)
    };

    int lines[12*2] = {
      0, 1,
      1, 2,
      2, 3,
      3, 0,
      0, 4,
      1, 5,
      2, 6,
      3, 7,
      4, 5,
      5, 6,
      6, 7,
      7, 4
    };

    int polygons[12*3] = {
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

    std::vector<int> polygon_order = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    std::vector<uint16_t> polygon_color = {TFT_RED, TFT_RED, TFT_RED, TFT_RED, TFT_GREEN, TFT_GREEN,TFT_GREEN,TFT_GREEN,TFT_BLUE,TFT_BLUE,TFT_BLUE,TFT_BLUE};

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
int mode = 0;
float posX = 0;
float posY = 0;

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

  if (M5.getBoard() == m5::board_t::board_M5StickCPlus || M5.getBoard() == m5::board_t::board_M5StickCPlus2) {
    screenWidth = 240.0f;
    screenHeight = 135.0f;
    aspect = screenWidth / screenHeight;
    projectionMatrix = Matrix44::Perspective(fov, aspect, near, far);
    pinMode(26, OUTPUT_OPEN_DRAIN);
  }
#if 0
  pinMode(36, INPUT);
  gpio_pulldown_dis(GPIO_NUM_25); // Disable pull-down on GPIO.
  gpio_pullup_dis(GPIO_NUM_25); // Disable pull-up on GPIO.  
  ledcSetup(0, 12000, 8);
  ledcAttachPin(26, 0);
#endif

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
  Vector4 p[8];
  for (int i = 0; i < 8; i++) {
    p[i] = Matrix44::ProjectTo2D(points[i], viewMatrix * mat, projectionMatrix, screenWidth, screenHeight);
  }

  // Z位置順にソート
  std::sort(polygon_order.begin(), polygon_order.end(),
    [p](int a, int b) -> bool {
      int a0 = polygons[a*3+0];
      int a1 = polygons[a*3+1];
      int a2 = polygons[a*3+2];
      float da = p[a0].z + p[a1].z + p[a2].z;
      int b0 = polygons[b*3+0];
      int b1 = polygons[b*3+1];
      int b2 = polygons[b*3+2];
      float db = p[b0].z + p[b1].z + p[b2].z;
        return da > db;
    });

  // 描画
  for (int i = 0; i < 12; i++) {
    int j = polygon_order[i];
    int j0 = polygons[j*3+0];
    int j1 = polygons[j*3+1];
    int j2 = polygons[j*3+2];
    bool cw = Vector4::IsClockwise(p[j0], p[j1], p[j2]);
    if (cw) {
      canvas.fillTriangle((int)p[j0].x, (int)p[j0].y, (int)p[j1].x, (int)p[j1].y, (int)p[j2].x, (int)p[j2].y, polygon_color[j]);
    }
  }

  canvas.pushSprite(0, 0);

  M5.update();
  if (M5.BtnA.isPressed()) {
    mode = 1 - mode;
    while (M5.BtnA.isPressed()) {
      M5.update();
      delay(16);
    }
  }
  //ledcWrite(0, 128);

  delay(16);
}
