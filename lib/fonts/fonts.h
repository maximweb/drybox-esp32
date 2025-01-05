#pragma once

#include <Arduino.h>

#include "display.h"

class Number8x16 {
public:
    Number8x16(Display& display);

    void draw(const char* s, int16_t x, int16_t y, char h_alignment = 'l');
    void draw_char(char c, int16_t x, int16_t y);

private:
    Display& m_display;
};

extern const PROGMEM uint8_t NUMBER_8_16[288];

class Number18x32 {
public:
    Number18x32(Display& display);

    void draw(const char* s, int16_t x, int16_t y, char h_alignment = 'l');
    void draw_char(char c, int16_t x, int16_t y);

private:
    Display& m_display;
};

extern const PROGMEM uint8_t DASH_18_4[12];
extern const PROGMEM uint8_t DOT_18_6[18];
extern const PROGMEM uint8_t DIGITS_18_32[10][96];
extern const PROGMEM uint8_t CHARS_16_22[6][66];
