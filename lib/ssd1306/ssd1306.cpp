#include "ssd1306.h"

Ssd1306::Ssd1306()
: m_wire{&Wire}
{
}

Ssd1306::Ssd1306(TwoWire* wire)
: m_wire{wire}
{
}

void Ssd1306::command(uint8_t cmd)
{
    (*m_wire).beginTransmission(0x3C);
    (*m_wire).write(0x80);
    (*m_wire).write(cmd);
    (*m_wire).endTransmission();
}

void Ssd1306::commands(uint8_t* cmds, uint8_t n)
{
    (*m_wire).beginTransmission(0x3C);
    (*m_wire).write(0x00);
    (*m_wire).write(cmds, n);
    (*m_wire).endTransmission();
}

void Ssd1306::begin()
{
    /*
        SSD1306 is an OLED/PLED driver for 128x64 displays
    */

    (*m_wire).setBufferSize(max(129, I2C_BUFFER_LENGTH)); // 1 command byte + 128 data bytes per PAGE
    (*m_wire).begin();

    delay(5); // TODO: reduce?

    // Set Display Off
    command(0xAE); // "sleep mode" (RESET)

    // Empty RAM
    clear();
    flush();

    // Set Display Clock Divide Ratio/Oscillator Frequency
    // lower four bits[3:0]: Divide ratio of display clock
    // upper four bits[7:4]: Oscillator frequency; range 0000b - 1111b; RESET=1111b
    // VALUE: 0x80: 1000b (frequency) 0000b (divider = 1)
    uint8_t cmds[2];
    cmds[0] = 0xD5; // COMMAND
    cmds[1] = 0x80; // VALUE
    commands(cmds, 2);

    // Set Multiplex Ratio
    // duty = 1/32 (0x1F)  or duty = 1/64 (0x3F) TODO: MEANING??
    cmds[0] = 0xA8; // COMMAND
    cmds[1] = 0x3F; // VALUE
    commands(cmds, 2);

    // Set Display Offset (vertical shift by COM from 0d - 63d)
    // 0x00: first column (automatically set to 0x00 by RESET)
    cmds[0] = 0xD3; // COMMAND
    cmds[1] = 0x00; // VALUE
    commands(cmds, 2);

    // Set Display Start Line (TODO: not needed when REST?)
    // 0x40 - 0x7F: 0 - 63 in lower 6 bits (set to 0x40: 0 during RESET)
    command(0x40);

    // Charge Pump Setting
    cmds[0] = 0x8D; // COMMAND
    cmds[1] = 0x14; // VALUE: 0x10: disabled; 0x14: enabled
    commands(cmds, 2);

    // Set Memory Addressing Mode
    // Page Mode does NOT increment PAGE when data exceed width
    // 0x00: horizontal; 0x01: vertical; 0x02: Page Addressing Mode (RESET)
    cmds[0] = 0x20; // COMMAND
    cmds[1] = 0x02; // VALUE
    commands(cmds, 2);

    // Set Lower Column Start Address for Page Addressing Mode
    // 0x00 - 0x0F (reset to 0x00 after RESET)
    command(0x00);

    // Set Higher Column Start Address for Page Addressing Mode
    // 0x10 - 0x1F (reset to 0x10 after RESET)
    command(0x10);

    // Set Page Start Address for Page Addressing Mode
    // 0xB0 - 0xB7: Set GDDRAM Page Start Address (PAGE0 - PAGE7)
    command(0xB0);

    // Set Segment Re-map (horizontal flip)
    // 0xA0: column address 0 mapped to SEGO (RESET); 0xA1: column address 127 mapped to SEG0
    command(0xA1);

    // Set COM Output Scan Direction (vertical flip)
    // 0xC0: normal mode (RESET) scan COM0 -> COM[N-1]
    // 0xC8: remapped Scan COM[N-1] -> COM0
    // where N is Multiplex ratio
    command(0xC8);

    // Set COM Pins Hardware Configuration
    // binary 00 A[5] A[4] 0010
    // A[4]: 0: sequential; 1: alternative (RESET)
    // A[5]: 0: disable COM left/right remap; 1: enable COM left/right remap
    // 0x02: sequential; disabled COM remap
    // 0x12: alternative; disabled COM remap
    // 0x22: sequential; enabled COM remap
    // 0x32: alternative; enable COM remap
    cmds[0] = 0xDA; // COMMAND
    cmds[1] = 0x12; // VALUE: 0x12: 00010010b A[4]=0; A[5]=0
    commands(cmds, 2);

    // Set Contrast Control
    cmds[0] = 0x81; // COMMAND
    cmds[1] = 0xFF; // VALUE: (double byte 1 to 256, 0x80=128)
    commands(cmds, 2);

    // Set Pre-charge Period
    // lower four bits[3:0]: Phase 1 period up to 15 DCLK clocks (0 is invalid); RESET=0x?2
    // upper four bits[7:4]: Phase 2 period up to 15 DCLK clokcs (0 is invalid); RESET=0x2?
    cmds[0] = 0xD9; // COMMAND
    cmds[1] = 0xF1; // VALUE: 0xF1: 1111b (phase 2) 0001b (phase 1)
    commands(cmds, 2);

    // Set V_COMH Deselect Level
    // binary 0 A[6] A[5] A[4] 0000
    // A[6:4]: 000b: 0.65*V_CC; 010b: 0.77*V_CC (RESET); 011b: 0.83*V_CC
    cmds[0] = 0xDB; // COMMAND
    cmds[1] = 0x20; // VALUE: 0x30: 00110000b 0.83*V_CC
    commands(cmds, 2);

    // Set Display to resume from RAM
    command(0xA4);

    // Set Normal/Inverse Display
    // 0xA6: normal (RESET) (0 in RAM is OFF)
    // 0xA7: inverse (0 in RAM is ON)
    command(0xA6);

    // Deactivate scroll
    command(0x2E);

    // Set Display On
    command(0xAF);
}

void Ssd1306::clear()
{
    for (size_t i = 0; i < width * height / 8; i++) {
        m_buffer[i] = 0;
    }
}

void Ssd1306::clear_pixel(uint8_t x, uint8_t y)
{
    if (x > width || y > height)
        return;

    m_buffer[(y / 8) * width + x] &= ~(1 << (y % 8));
}

void Ssd1306::clear_area(uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end)
{
    for (uint8_t x = x_start; x <= x_end; x++) {
        for (uint8_t y = y_start; y <= y_end; y++) {
            clear_pixel(x, y);
        }
    }
}

bool Ssd1306::next_segment()
{
    return false;
}

void Ssd1306::flush()
{
    uint8_t* buffer = m_buffer;
    uint8_t cmds[3];

    // write data
    for (uint8_t page = 0; page < (height / 8); page++) {

        cmds[0] = 0x22; // COMMAND: Set Page Address
        cmds[1] = page; // VALUE: Page start address 0 - 7
        cmds[2] = page; // VALUE: Page end address 0 - 7
        commands(cmds, 3);

        cmds[0] = 0x21; // COMMAND: Set Column Address
        cmds[1] = 0x00; // VALUE: Column start address 0 - 127
        cmds[2] = 0x7F; // VALUE: Column end anddress 0 - 127
        commands(cmds, 3);

        // Begin data transmission
        (*m_wire).beginTransmission(0x3C);
        (*m_wire).write(0x40);

        for (size_t i = 0; i < width; i++) {

            (*m_wire).write(buffer[i]);
        }
        (*m_wire).endTransmission();

        buffer += width;
    }
}

void Ssd1306::draw_pixel(uint8_t x, uint8_t y)
{
    if (x > width || y > height || x < 0 || y < 0)
        return;

    // example: first byte 0xFF equals vertical line starting at upper left and 8px length
    m_buffer[(y / 8) * width + x] |= (1 << (y % 8));
}

void Ssd1306::draw_bitmap(uint8_t x, uint8_t y, Bitmap&& bitmap)
{
    uint8_t byte_width = (bitmap.width + 7) / 8;

    for (uint8_t j = 0; j < bitmap.height; j++) {
        for (uint8_t i = 0; i < bitmap.width; i++) {
            // Seems stupid to read the same byte over and over again ...
            if (pgm_read_byte(bitmap.data + j * byte_width + i / 8) & (128 >> (i & 7))) {
                draw_pixel(x + i, y + j);
            }
        }
    }
}

void Ssd1306::invert_area(uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end)
{
    // flip min/max
    // clip to display area
    uint8_t pixel{0};

    for (uint8_t y = y_start; y <= y_end; y++) {
        for (uint8_t x = x_start; x <= x_end; x++) {
            pixel = m_buffer[(y / 8) * width + x] & (1 << (y % 8));
            if (pixel > 0) {
                // pixel was on, need to turn it off
                // m_buffer[(y / 8) * width + x] &= ~(1 << (y % 8));
                clear_pixel(x, y);
            }
            else {
                // pixel was off, need to turn it on
                // m_buffer[(y / 8) * width + x] |= (1 << (y % 8));
                draw_pixel(x, y);
            }
        }
    }
}

void Ssd1306::draw_h_line(uint8_t y, uint8_t x_start, uint8_t x_end, char linestyle)
{
    const uint8_t x_min = x_end > x_start ? x_start : x_end;
    const uint8_t x_max = x_end > x_start ? x_end : x_start;

    for (uint8_t x = x_min; x <= x_max; x++) {
        if (linestyle == '.') {
            if ((x - x_min) % 2 == 1) {
                continue;
            }
        }
        else if (linestyle == ',') {
            if ((x - x_min) % 5 >= 3) {
                continue;
            }
        }
        draw_pixel(x, y);
    }
}

void Ssd1306::draw_v_line(uint8_t x, uint8_t y_start, uint8_t y_end, char linestyle)
{
    const uint8_t y_min = y_end > y_start ? y_start : y_end;
    const uint8_t y_max = y_end > y_start ? y_end : y_start;

    for (uint8_t y = y_min; y <= y_max; y++) {
        if (linestyle == '.') {
            if ((y - y_start) % 2 == 1) {
                continue;
            }
        }
        else if (linestyle == ',') {
            if ((y - y_min) % 5 >= 4) {
                continue;
            }
        }
        draw_pixel(x, y);
    }
}

/**
 * @brief Draw a line between two points.
 *
 * Uses compact variant of Bresenham algorithm
 *
 * @see https://de.wikipedia.org/wiki/Bresenham-Algorithmus#Kompakte_Variante
 *
 * @param x_start
 * @param y_start
 * @param x_end
 * @param y_end
 * @param linestyle
 */
void Ssd1306::draw_line(uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end, char linestyle = '-')
{
    // vertical line
    if (y_start == y_end) {
        draw_h_line(y_start, x_start, x_end, linestyle);
        return;
    }

    // horizontal line
    if (x_start == x_end) {
        draw_v_line(x_start, y_start, y_end, linestyle);
        return;
    }

    // diagonal line using a generalized Bresenham algorithm
    int16_t dx = abs((int16_t) x_end - (int16_t) x_start);
    int16_t sx = x_start < x_end ? 1 : -1;
    int16_t dy = -abs((int16_t) y_end - (int16_t) y_start);
    int16_t sy = y_start < y_end ? 1 : -1;
    int16_t err = dx + dy;
    int16_t e2;

    uint8_t x{x_start};
    uint8_t y{y_start};
    while (true) {
        draw_pixel(x, y);
        if (x == x_end && y == y_end) {
            break;
        }
        e2 = 2 * err;
        if (e2 > dy) {
            err += dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

/**
 * @brief Draw a circle by midpoint and radius.
 *
 * @param x_center x coordinate of circle center
 * @param y_center y coordinate of circle center
 * @param radius radius of circle
 */
void Ssd1306::draw_circle(uint8_t x_center, uint8_t y_center, uint8_t radius)
{
    /**
     * Andres algorithm (with swapped x and y), as other algorithms result in holes when filling with loop over radius.
     *
     * https://fr.wikipedia.org/wiki/Algorithme_de_tracé_de_cercle_d%27Andres#Algorithme
     */

    uint8_t x = radius;
    uint8_t y = 0;
    int16_t d = radius - 1;

    while (x >= y) {
        draw_pixel(x + x_center, y + y_center);
        draw_pixel(y + x_center, x + y_center);
        draw_pixel(x + x_center, -y + y_center);
        draw_pixel(-y + x_center, x + y_center);
        draw_pixel(-x + x_center, y + y_center);
        draw_pixel(y + x_center, -x + y_center);
        draw_pixel(-x + x_center, -y + y_center);
        draw_pixel(-y + x_center, -x + y_center);

        if (d >= 2 * y) {
            d += -2 * y - 1;
            y += 1;
        }
        else if (d < 2 * (radius - x)) {
            d += 2 * x - 1;
            x -= 1;
        }
        else {
            d += 2 * (x - y - 1);
            x -= 1;
            y += 1;
        }
    }
}

/**
 * @brief Draw a circle arc by midpoint, radius, start and end angle.
 *
 * Uses Jesko's method for circle drawing combined with
 * arc handling as proposed by @motla for u8g2 library.
 *
 * @see https://github.com/olikraus/u8g2/discussions/1740#discussioncomment-6680633
 * @see https://github.com/olikraus/u8g2/pull/2281
 *
 * @param x_center
 * @param y_center
 * @param radius
 * @param angle_start
 * @param angle_end
 */
void Ssd1306::draw_arc(uint8_t x_center, uint8_t y_center, uint8_t radius, uint16_t angle_start, uint16_t angle_end)
{
    /**
     * Andres algorithm (with swapped x and y), as other algorithms result in holes when filling with loop over radius.
     *
     * https://fr.wikipedia.org/wiki/Algorithme_de_tracé_de_cercle_d%27Andres#Algorithme
     */

    // Angle inputs
    if (angle_start == angle_end) {
        return;
    }
    uint8_t inverted = (angle_start > angle_end);
    uint16_t as = inverted ? (angle_end % 360) : (angle_start % 360);
    uint16_t ae = inverted ? (angle_start % 360) : (angle_end % 360);
    uint32_t ratio;

    // Andres circle algorithm
    uint8_t x = radius;
    uint8_t y = 0;
    int16_t d = radius - 1;

    while (x >= y) {
        // Percentagge of 1/8th circle with approximated arctan
        // See: https://github.com/olikraus/u8g2/issues/2243#issuecomment-1763484358
        ratio = y * 359 / x;
        ratio = ratio * (770195 - (ratio - 255) * (ratio + 941)) / 6137491; // arctan(x/y) [0..45]

        // Angle is counterclockwise starting at right

        // First octant
        if ((ratio >= as && ratio < ae) ^ inverted) {
            draw_pixel(x + x_center, -y + y_center);
        }

        // Second octant
        if (((ratio + ae) > 89 && (ratio + as) <= 89) ^ inverted) {
            draw_pixel(y + x_center, -x + y_center);
        }

        // Third octant
        if (((ratio + 90) >= as && (ratio + 90) < ae) ^ inverted) {
            draw_pixel(-y + x_center, -x + y_center);
        }

        // Fourth octant
        if (((ratio + ae) > 179 && (ratio + as) < 179) ^ inverted) {
            draw_pixel(-x + x_center, -y + y_center);
        }

        // Fifth octant
        if (((ratio + 180) >= as && (ratio + 180) < ae) ^ inverted) {
            draw_pixel(-x + x_center, y + y_center);
        }

        // Sixths octant
        if (((ratio + ae) > 269 && (ratio + as) < 269) ^ inverted) {
            draw_pixel(-y + x_center, x + y_center);
        }

        // Sevenths octant
        if (((ratio + 270) >= as && (ratio + 270) < ae) ^ inverted) {
            draw_pixel(y + x_center, x + y_center);
        }

        // Eiths octant
        if (((ratio + ae) > 359 && (ratio + as) < 359) ^ inverted) {
            draw_pixel(x + x_center, y + y_center);
        }

        // Andres circle algorithm
        if (d >= 2 * y) {
            d += -2 * y - 1;
            y += 1;
        }
        else if (d < 2 * (radius - x)) {
            d += 2 * x - 1;
            x -= 1;
        }
        else {
            d += 2 * (x - y - 1);
            x -= 1;
            y += 1;
        }
    }
}