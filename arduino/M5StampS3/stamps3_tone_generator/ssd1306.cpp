#include "ssd1306.h"
#include <Wire.h>

#define SSD1306_ADDR 0x3C
#define WIDTH 128
#define HEIGHT 64
#define PAGES (HEIGHT / 8)

uint8_t buffer[WIDTH * PAGES * 2];
int32_t buffer_idx = 0;

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
  int32_t ofs = buffer_idx * WIDTH * PAGES;
  buffer_idx = 1 - buffer_idx;
  for (uint8_t page = 0; page < PAGES; page++) {
    ssd1306_command(0xB0 + page);
    ssd1306_command(0x00);
    ssd1306_command(0x10);
#if 0
    for (int offset = 0; offset < WIDTH; offset += 16) {
      Wire.beginTransmission(SSD1306_ADDR);
      Wire.write(0x40); // Data mode
      for (uint8_t i = 0; i < 16; i++) {
        Wire.write(buffer[page * WIDTH + offset + i + ofs]);
      }
      Wire.endTransmission();
    }
#else
      Wire.beginTransmission(SSD1306_ADDR);
      Wire.write(0x40); // Data mode
      for (uint8_t i = 0; i < WIDTH; i++) {
        Wire.write(buffer[page * WIDTH + i + ofs]);
      }
      Wire.endTransmission();
#endif
  }
}

void clearBuffer() {
  int32_t ofs = buffer_idx * WIDTH * PAGES;
  memset(buffer + ofs, 0x00, WIDTH * PAGES);
}

void drawPixel(int x, int y, bool color) {
  int32_t ofs = buffer_idx * WIDTH * PAGES;
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
  int index = x + (y / 8) * WIDTH;
  uint8_t mask = 1 << (y & 7);
  if (color) buffer[index + ofs] |= mask;
  else buffer[index + ofs] &= ~mask;
}

void fillRect(int x, int y, int w, int h, bool color) {
  if (w <= 0 || h <= 0) return;
  int x0 = x;
  int y0 = y;
  int x1 = x + w - 1;
  int y1 = y + h - 1;
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 >= WIDTH) x1 = WIDTH - 1;
  if (y1 >= HEIGHT) y1 = HEIGHT - 1;

  for (int yy = y0; yy <= y1; yy++) {
    for (int xx = x0; xx <= x1; xx++) {
      drawPixel(xx, yy, color);
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
      bool color = (c != 0x0000);  // 0ˆÈŠO‚È‚ç”’

      drawPixel(px, py, color);
    }
  }
}
