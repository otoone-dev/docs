#pragma once
#include "Parameters.h"

void DebugMIDIMessage() {
    Serial.begin(115200);
    for (int i = 0; i < 5; i++) {
        Serial.println("Serial Port to USB");
        delay(1000);
    }
    Serial2.begin(31250, SERIAL_8N1, MIDI_IN_PIN, MIDI_OUT_PIN);
    uint64_t startTime = 0;
    int bufferSize = 100;
    int buffer[bufferSize];
    float time[1024];
    int bufferPos = 0;
    uint8_t readBuffer[256];
    int status = 0;
    bool playing = false;
    while (1) {
        uint64_t t = micros();
        int count = Serial2.read(readBuffer, bufferSize);
        if (count == 0) {
            //SetLED(0, 0, 0);
        }
        else {
            for (int i = 0; i < count; i++) {
                int d = readBuffer[i];
                int st = d & 0xF0;
                if (st >= 0x80) {
                    status = st;
                    if (status == 0x90) {
                        playing = true;
                    }
                    if (playing && (status == 0x90 || status == 0x80 || status == 0xB0)) {
                        //SetLED(0, 250, 0);
                        buffer[bufferPos] = d;
                        time[bufferPos] = ((int)(t - startTime)) / 1000.0f;
                        if (bufferPos < bufferSize-1) bufferPos++;
                    }
                }
                else {
                    if (playing && (status == 0x90 || status == 0x80 || status == 0xB0)) {
                        //SetLED(0, 0, 250);
                        buffer[bufferPos] = d;
                        time[bufferPos] = ((int)(t - startTime)) / 1000.0f;
                        if (bufferPos < bufferSize-1) bufferPos++;
                    }
                }
                if (bufferPos >= bufferSize-2) {
                    //SetLED(250, 0, 0);
                    int g = 0;
                    for (int i = 0; i < bufferPos; i++) {
                        uint8_t e = buffer[i];
                        if (e >= 0x80) {
                            Serial.printf("\n%1.4fs : %02X,", time[i] / 1000.0f, buffer[i]);
                            g = 0;
                        }
                        else {
                            Serial.printf("%02X,", e);
                            g = (g + 1) % 2;
                            if (g == 0) {
                                Serial.printf(" - ");
                            }
                        }
                    }
                    bufferPos = 0;
                }
            }
        }
        delay(5);
    }
}