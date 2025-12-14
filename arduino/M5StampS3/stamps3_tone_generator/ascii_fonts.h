#ifndef _ASCII_FONTS_H_
#define _ASCII_FONTS_H_
#include "ssd1306.h"

//--------------------------
// sx, sy の位置に str を描画（英数字のみです）
extern void DrawString(const char* str, int sx, int sy);

#endif // _ASCII_FONTS_H_
