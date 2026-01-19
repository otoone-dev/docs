#include <Arduino.h>
#include <M5Unified.h>

M5Canvas canvas(&M5.Display);

#define BUFFER_MAX (240)
volatile float buffer[BUFFER_MAX];
volatile float mid = 0.0f;

static void ReadTask(void *parameter) {
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 1 / portTICK_PERIOD_MS; // 1ms
    int c = 0;
    float f = 0;
    while (1) {
      int r = analogRead(26);
      mid += (r - mid) * 0.005f;
      if (c++ % 2 == 0) {
        f = abs(r - mid) / 200.0f;
      }
      else {
        for (int i = 0; i < BUFFER_MAX-1; i++) {
          buffer[i] = buffer[i+1];
        }
        float f2 = abs(r - mid) / 100.0f;
        if (f2 > f) {
          buffer[BUFFER_MAX-1] = f2;
        }
        else {
          buffer[BUFFER_MAX-1] = f;
        }
      }

      xTaskDelayUntil( &xLastWakeTime, xFrequency );
    }
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.clearDisplay(TFT_BLACK);
  M5.Lcd.setRotation(1);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("Piezo Test");
  delay(1000);
  canvas.createSprite(M5.Display.width(), M5.Display.height());
  canvas.setTextSize(2);
  pinMode(26, INPUT);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  xTaskCreatePinnedToCore(ReadTask, "ReadTask", 4096, nullptr, 1, NULL, 0);
}

float buffer2[BUFFER_MAX];
void loop() {
  M5.update();
  canvas.clear(TFT_BLACK);
  for (int i = 0; i < BUFFER_MAX; i++) {
    buffer2[i] = buffer[i];
  }
  int d0 = 130 - (int)(buffer2[0] * 130.0f);
  for (int i = 1; i < BUFFER_MAX; i++) {
    int d1 = 130 - (int)(buffer2[i] * 130.0f);
    canvas.drawLine(i-1, d0, i, d1, TFT_GREEN);
    d0 = d1;
  }
  canvas.pushSprite(0, 0);
  delay(10);
}
