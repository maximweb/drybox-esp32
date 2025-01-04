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
    void clear() final;
    void clear_pixel(uint8_t x, uint8_t y) final;
    void clear_area(uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end) final;
    void flush() final;
    bool next_segment() final;
    void draw_pixel(uint8_t x, uint8_t y) final;
    void draw_bitmap(uint8_t x, uint8_t y, Bitmap&& bitmap) final;
    void invert_area(uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end) final;
    void draw_h_line(uint8_t y, uint8_t x_start, uint8_t x_end, char linestyle) final;
    void draw_v_line(uint8_t x, uint8_t y_start, uint8_t y_end, char linestyle) final;
    void draw_line(uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end, char linestyle) final;
    void draw_circle(uint8_t x, uint8_t y, uint8_t radius) final;
    void draw_arc(uint8_t x, uint8_t y, uint8_t radius, uint16_t angle_start, uint16_t angle_end) final;

private:
    void command(uint8_t cmd);
    void commands(uint8_t* cmd, uint8_t n);

    uint8_t m_buffer[width * height / 8];
    TwoWire* m_wire;
};
