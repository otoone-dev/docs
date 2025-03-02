#include <M5Unified.h>
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

//-------------------------------------
void SetLedColor(int r, int g, int b) {
    neopixelWrite(GPIO_NUM_21, r, g, b);
}

//-------------------------------------
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

#if defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040)
  // Manual begin() is required on core without built-in support for TinyUSB such as mbed rp2040
  TinyUSB_Device_Init(0);
#endif
  MIDI.begin(MIDI_CHANNEL_OMNI);

  SetLedColor(255, 255, 255);
  delay(300);
  SetLedColor(0, 0, 0);
  delay(300);

  Serial.begin(115200);
  while (!TinyUSBDevice.mounted()) delay(1);
}

unsigned long previous=0;

//-------------------------------------
void loop() {
    unsigned long current=millis();
    if (current-previous>800)
    {  
      SetLedColor(0, 255, 0);
      MIDI.sendNoteOn(100, 127, 1);
      Serial.println("send");
      previous=current;
      delay(100);
      SetLedColor(0, 0, 0);
    }
  MIDI.read();  
}
