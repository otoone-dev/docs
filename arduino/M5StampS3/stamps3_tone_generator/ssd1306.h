#pragma once
#include <Wire.h>

#include <Wire.h>

#define SSD1306_ADDR 0x3C
#define WIDTH 128
#define HEIGHT 64
#define PAGES (HEIGHT / 8)

uint8_t buffer[WIDTH * PAGES];

void ssd1306_command(uint8_t c) {
  Wire.beginTransmission(SSD1306_ADDR);
  Wire.write(0x00); // Command mode
  Wire.write(c);
  Wire.endTransmission();
}

void ssd1306_init() {
  delay(100);
  ssd1306_command(0xAE); // Display OFF
  ssd1306_command(0x20); // Set Memory Addressing Mode
  ssd1306_command(0x00); // Horizontal addressing mode
  ssd1306_command(0x40); // Set Display Start Line
  ssd1306_command(0xA1); // Segment remap
  ssd1306_command(0xC8); // COM scan direction remap
  ssd1306_command(0xDA);
  ssd1306_command(0x12); // COM pins for 128x64
  ssd1306_command(0x81);
  ssd1306_command(0x7F); // Contrast
  ssd1306_command(0xA4); // Resume to RAM content
  ssd1306_command(0xA6); // Normal display
  ssd1306_command(0xD5);
  ssd1306_command(0x80); // Clock divide
  ssd1306_command(0x8D);
  ssd1306_command(0x14); // Enable charge pump
  ssd1306_command(0xAF); // Display ON
  ssd1306_command(0x21); // Set Column Address
  ssd1306_command(0x00);
  ssd1306_command(0x7f); // 0-127
}

void ssd1306_update() {
  for (uint8_t page = 0; page < PAGES; page++) {
    ssd1306_command(0xB0 + page);
    ssd1306_command(0x00);
    ssd1306_command(0x10);
#if 0
    for (int offset = 0; offset < WIDTH; offset += 16) {
      Wire.beginTransmission(SSD1306_ADDR);
      Wire.write(0x40); // Data mode
      for (uint8_t i = 0; i < 16; i++) {
        Wire.write(buffer[page * WIDTH + offset + i]);
      }
      Wire.endTransmission();
    }
#else
      Wire.beginTransmission(SSD1306_ADDR);
      Wire.write(0x40); // Data mode
      for (uint8_t i = 0; i < WIDTH; i++) {
        Wire.write(buffer[page * WIDTH + i]);
      }
      Wire.endTransmission();
#endif
  }
}

void clearBuffer() {
  memset(buffer, 0x00, sizeof(buffer));
}

void drawPixel(int x, int y, bool color) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
  int index = x + (y / 8) * WIDTH;
  uint8_t mask = 1 << (y & 7);
  if (color) buffer[index] |= mask;
  else buffer[index] &= ~mask;
}

void pushImage0(int x, int y, int w, int h, const uint16_t *data) {
  if (w <= 0 || h <= 0) return;

  for (int iy = 0; iy < h; iy++) {
    int py = y + iy;
    if (py < 0 || py >= HEIGHT) continue;

    for (int ix = 0; ix < w; ix++) {
      int px = x + ix;
      if (px < 0 || px >= WIDTH) continue;

      uint16_t c = data[iy * w + ix];
      bool color = (c != 0x0000);  // 0以外なら白

      drawPixel(px, py, color);
    }
  }
}

void pushImage(int x, int y, int w, int h, const uint16_t *data) {
  if (w <= 0 || h <= 0) return;

  for (int iy = 0; iy < h; iy++) {
    int py = (y + iy) * 2 + 1; // 32 -> 64
    if (py < 0 || py >= HEIGHT) continue;

    for (int ix = 0; ix < w; ix++) {
      int px = x + ix;
      if (px < 0 || px >= WIDTH) continue;

      uint16_t c = data[iy * w + ix];
      bool color = (c != 0x0000);  // 0以外なら白

      drawPixel(px, py, color);
    }
  }
}
