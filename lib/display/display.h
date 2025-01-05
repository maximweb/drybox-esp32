#pragma once

#include <Arduino.h>

/**
 * A bitmap description.
 */
struct Bitmap {
    /// Width of bitmap in number of pixels.
    const uint8_t width;
    /// Height of bitmap in number of pixels.
    const uint8_t height;
    /// Pointer to bitmap data with at least (width * height / 8) bytes.
    const uint8_t* data;
};

class Display {
public:
    /**
     * One-time initialization to be called in setup().
     */
    virtual void begin() = 0;

    virtual void soft_reset() = 0;

    /**
     * Clear the frame buffer.
     */
    virtual void clear() = 0;

    virtual void clear_pixel(int16_t x, int16_t y) = 0;

    virtual void clear_area(int16_t x_start, int16_t y_start, int16_t x_end, int16_t y_end) = 0;

    /**
     * Write the frame buffer to the display.
     */
    virtual void flush() = 0;

    /**
     * Return true if segmented display buffer is utilized and current segment is not the last segment.
     */
    virtual bool next_segment() = 0;

    /**
     * Draw a pixel at coordinate (@p x, @p y) if it falls within width and
     * height.
     *
     * @param x X coordinate.
     * @param y Y coordinate.
     */
    virtual void draw_pixel(int16_t x, int16_t y) = 0;

    /**
     * Draw a bitmap beginning with the top-left corner at (@p x, @p y).
     *
     * @param x X corner of the bitmap draw position.
     * @param y Y corner of the bitmap draw position.
     * @param bitmap Bitmap description.
     */
    virtual void draw_bitmap(int16_t x, int16_t y, Bitmap&& bitmap) = 0;
    virtual void invert_area(int16_t x_start, int16_t y_start, int16_t x_end, int16_t y_end) = 0;

    virtual void draw_h_line(int16_t y, int16_t x_start, int16_t x_end, char linestyle = '-') = 0;
    virtual void draw_v_line(int16_t x, int16_t y_start, int16_t y_end, char linestyle = '-') = 0;

    virtual void draw_line(int16_t x_start, int16_t y_start, int16_t x_end, int16_t y_end, char linestyle = '-') = 0;
    virtual void draw_circle(int16_t x, int16_t y, uint8_t radius) = 0;
    virtual void draw_arc(int16_t x, int16_t y, uint8_t radius, uint16_t angle_start, uint16_t angle_end) = 0;

    // This is slightly unfortunate and fixes all deriving displays to be of
    // this dimension.
    static constexpr size_t width{128};
    static constexpr size_t height{64};
};

/**
 * A non-functional display.
 */
class MockDisplay : public Display {
public:
    void begin() final {}
    void soft_reset() final {}

    void clear() final {}
    void clear_pixel(int16_t x, int16_t y) final {}
    void clear_area(int16_t x_start, int16_t y_start, int16_t x_end, int16_t y_end) final {}

    void flush() final {}
    bool next_segment() final { return false; }

    void draw_pixel(int16_t, int16_t) final {}
    void draw_bitmap(int16_t, int16_t, Bitmap&&) final {}
    void invert_area(int16_t, int16_t, int16_t, int16_t) final {}

    void draw_h_line(int16_t y, int16_t x_start, int16_t x_end, char linestyle) final {}
    void draw_v_line(int16_t x, int16_t y_start, int16_t y_end, char linestyle) final {}
    void draw_line(int16_t x_start, int16_t y_start, int16_t x_end, int16_t y_end, char linestyle) final {}

    void draw_circle(int16_t x, int16_t y, uint8_t radius) final {}
    void draw_arc(int16_t x, int16_t y, uint8_t radius, uint16_t angle_start, uint16_t angle_end) final {}
};
