#include <M5Unified.h>

static M5Canvas canvas(&M5.Lcd);

static int def1 = 0;
static int def2 = 0;

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  // Init
  Serial.begin(115200);

  canvas.setColorDepth(16);
  canvas.createSprite(M5.Lcd.width(), M5.Lcd.height());

  delay(100);
  int d1 = 0;
  int d2 = 0;
  const int loop = 100;
  for (int i = 0; i < loop; i++) {
    d1 += (touchRead(1)/100);
    delay(10);
  }
  for (int i = 0; i < loop; i++) {
    d2 += (touchRead(2)/100);
    delay(10);
  }
  def1 = d1 / loop;
  def2 = d2 / loop;
}

void loop() {

  char s[32];
  int d1 = (touchRead(1)/100)-def1;
  delay(8);
  int d2 = (touchRead(2)/100)-def2;
  delay(8);

  canvas.fillScreen(BLACK);
  sprintf(s, "%d, %d", d1, d2);
  canvas.drawString(s, 10, 10, 2);
  if (d1 < 1) d1 = 1;
  if (d2 < 1) d2 = 1;
  if (d1 > 128) d1 = 128;
  if (d2 > 128) d2 = 128;
  canvas.fillRect(10, 50, d1, 30, TFT_WHITE);
  canvas.fillRect(10, 100, d2, 30, TFT_WHITE);

  canvas.pushSprite(0, 0);
}
