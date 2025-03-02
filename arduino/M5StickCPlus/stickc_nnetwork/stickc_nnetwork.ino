#include <M5Unified.h>
#include "nnetwork.h"

static M5Canvas canvas(&M5.Lcd);

// ニューラルネットワークのインスタンスを作成（例：2入力、3隠れニューロン、1出力）
NeuralNetwork nn(2, 4, 1);
int counter = 0;

void setup() {
  // put your setup code here, to run once:
  M5.begin();

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);

  canvas.setColorDepth(16);
  canvas.createSprite(240, 135);

  Serial.begin(115200);
}

void loop() {
  M5.update();

  std::vector<std::vector<float>> training_data = {
      {0, 0}, {0, 1}, {1, 0}, {1, 1}};
  std::vector<std::vector<float>> targets = {
      {0}, {1}, {1}, {0}};
  for (int j = 0; j < 100; j++) {
    for (size_t i = 0; i < training_data.size(); i++) {
      nn.train(training_data[i], targets[i]);
      counter++;
    }
  }

  canvas.fillScreen(TFT_BLACK);

  std::vector<std::vector<float>> test_data = {
      {0, 0}, {0, 1}, {1, 0}, {1, 1}};

  char s[32];
  int y = 10;
  for (auto &inputs : test_data) {
    std::vector<float> result = nn.forward(inputs);
    sprintf(s, "{%1.1f, %1.1f} XOR = %1.4f", inputs[0], inputs[1], result[0]);
    canvas.drawString(s, 10, y); y += 20;
  }
  sprintf(s, "count=%d", counter);
  canvas.drawString(s, 10, y+5);

  if (M5.BtnA.isPressed()) {
    canvas.drawString("Pressed", 10, y+20);
  }

  canvas.pushSprite(0, 0);

  delay(20);
}
