#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "display.h"

/**
 * SSD1306 display driver.
 */
class Ssd1306 : public Display {
public:
    Ssd1306();
    Ssd1306(TwoWire* wire);

    void begin() final;
    void soft_reset() final;
    void clear() final;
    void clear_pixel(int16_t x, int16_t y) final;
    void clear_area(int16_t x_start, int16_t y_start, int16_t x_end, int16_t y_end) final;
    void flush() final;
    bool next_segment() final;
    void draw_pixel(int16_t x, int16_t y) final;
    void draw_bitmap(int16_t x, int16_t y, Bitmap&& bitmap) final;
    void invert_area(int16_t x_start, int16_t y_start, int16_t x_end, int16_t y_end) final;
    void draw_h_line(int16_t y, int16_t x_start, int16_t x_end, char linestyle) final;
    void draw_v_line(int16_t x, int16_t y_start, int16_t y_end, char linestyle) final;
    void draw_line(int16_t x_start, int16_t y_start, int16_t x_end, int16_t y_end, char linestyle) final;
    void draw_circle(int16_t x, int16_t y, uint8_t radius) final;
    void draw_arc(int16_t x, int16_t y, uint8_t radius, uint16_t angle_start, uint16_t angle_end) final;

private:
    void command(uint8_t cmd);
    void commands(uint8_t* cmd, uint8_t n);

    uint8_t m_buffer[width * height / 8];
    TwoWire* m_wire;
};
