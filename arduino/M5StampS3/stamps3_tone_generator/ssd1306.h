#ifndef _SSD1306_H_
#define _SSD1306_H_
#include <cstdint>

extern void ssd1306_command(uint8_t c);
extern void ssd1306_init();
extern void ssd1306_update();
extern void clearBuffer();
extern void drawPixel(int x, int y, bool color);
extern void fillRect(int x, int y, int w, int h, bool color);
extern void pushImage(int x, int y, int w, int h, const uint16_t *data);

#endif // _SSD1306_H_
