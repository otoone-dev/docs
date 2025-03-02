#include "M5Atom.h"

#define MIDI_OUT_PIN (32)
#define MIDI_IN_PIN (26)
#define CORE1 (1)

static unsigned long t0 = millis();
static int timCount = 0;
volatile int ledG = 20;
volatile int ledR = 0;

//-------------------------
void TickThread(void *pvParameters) {
  while (1) {
    if (ledG > 20) ledG -= 2;
    else ledG = 20;
    if (ledR > 0) ledR -= 2;
    else ledR = 0;
    if (ledR > 0) {
      M5.dis.drawpix(0, ledR << 16); 
    }
    else 
    {
      M5.dis.drawpix(0, ledG << 8); 
    }
    delay(5);
  }
}

//-------------------------
void setup() {
    M5.begin(true, false, true); 
    delay(50);
    M5.dis.drawpix(0, 0x002200); 

    Serial.begin(115200);
    Serial2.begin(31250, SERIAL_8N1, MIDI_OUT_PIN, MIDI_IN_PIN);

    xTaskCreatePinnedToCore(TickThread, "TickThread", 4096, NULL, 1, NULL, CORE1);
}

//-------------------------
void loop() {
    if (M5.Btn.wasPressed()) {
      Serial2.write(0x90);
      Serial2.write(61);
      Serial2.write(60);
      ledR = 200;
    }
    char d = Serial2.read();
    if (d == 0xFE) {
      // Active Sensing
    }
    else if (d == 0xF8) {
      // Timing Clock
      timCount = (timCount + 1) % 24;
      if (timCount == 0) {
        ledG = 200;
        unsigned long t = millis();
        int tempo = (int)((60*1000) / (t - t0));
        t0 = t;
        Serial.printf("tempo:%d\n", tempo);
      }
    }

    delay(8);
    M5.update();
}
