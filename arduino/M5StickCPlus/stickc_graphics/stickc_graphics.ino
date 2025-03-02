#include <M5Unified.h>

static M5Canvas canvas(&M5.Lcd);
static int counter = 0;

void setup() {
  // put your setup code here, to run once:
  M5.begin();
#if 0
  pinMode(36, INPUT);
  gpio_pulldown_dis(GPIO_NUM_25); // Disable pull-down on GPIO.
  gpio_pullup_dis(GPIO_NUM_25); // Disable pull-up on GPIO.  
  ledcSetup(0, 12000, 8);
  ledcAttachPin(26, 0);
#endif
  pinMode(26, OUTPUT_OPEN_DRAIN);

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);

  canvas.setColorDepth(16);
  canvas.createSprite(135, 240);
}

void loop() {
  M5.update();

  canvas.fillScreen(TFT_BLACK);

  int a = analogRead(36);
  counter++;
  char s[32];
  sprintf(s, "cnt:%02d, ana:%d", counter%100, a);
  canvas.drawString(s, 10, 10);

  float d = (counter / 100.0f) * 6.283f;
  int x0 = 68+(int)(100 * cos(d));
  int y0 = 120+(int)(100 * sin(d));
  int x1 = 68+(int)(100 * cos(d + 1.6f));
  int y1 = 120+(int)(100 * sin(d + 1.6f));
  int x2 = 68+(int)(100 * cos(d + 4.8f));
  int y2 = 120+(int)(100 * sin(d + 4.8f));
  canvas.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_RED);

  canvas.pushSprite(0, 0);


  if (M5.BtnA.isPressed()) {
    canvas.drawString("Pressed", 10, 30);
  }
  //ledcWrite(0, 128);

  delay(16);
}
