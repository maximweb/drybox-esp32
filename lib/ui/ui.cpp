#include "ui.h"
#include "fonts.h"

Ui::Ui(Display& display)
: m_display{display}
, m_n8x16{display}
, m_n18x32{display}
{
}

void Ui::set_layout_switching(bool enable)
{
    m_layout_switching = enable;
    // If layout switching is disabled, default to layout A (true)
    // m_current_layout = m_layout_switching ? m_current_layout : LayoutA;
}

Ui::Layout Ui::freeze_layout(bool freeze)
{
    m_freeze_layout = freeze;
    if (!m_freeze_layout) {
        m_last_layout_switch = millis();
    }
    return m_current_layout;
}

Ui::Layout Ui::current_layout()
{
    return m_current_layout;
}

void Ui::set_layout(Layout layout)
{
    m_current_layout = layout;
}

void Ui::set_wifi_state(WifiState state)
{
    if (m_wifi_state != state) {
        m_wifi_state = state;
        m_refresh = true;
    }
    else if (m_wifi_state == WifiState::WIFIConnecting) {
        // always request refresh for animated wifi symbol
        m_refresh = true;
    }
}

void Ui::wifi_symbol(uint8_t strength)
{
    // tiny wifi symbol

    uint8_t x_dot{122};
    uint8_t y_dot{7};

    switch (strength) { // no break!
        case 3:
            // upper arc
            m_display.draw_line(x_dot - 5, y_dot - 4, x_dot - 3, y_dot - 6);
            m_display.draw_line(x_dot - 3, y_dot - 6, x_dot + 3, y_dot - 6);
            m_display.draw_line(x_dot + 3, y_dot - 6, x_dot + 5, y_dot - 4);
        case 2:
            // middle arc
            m_display.draw_pixel(x_dot - 3, y_dot - 3);
            m_display.draw_line(x_dot - 2, y_dot - 4, x_dot + 2, y_dot - 4);
            m_display.draw_pixel(x_dot + 3, y_dot - 3);
        case 1:
            // lower arc
            m_display.draw_line(x_dot - 1, y_dot - 2, x_dot + 1, y_dot - 2);
    }
    // dot
    m_display.draw_pixel(x_dot, y_dot);
}

void Ui::wifi_update()
{
    uint8_t x_circle{119};
    uint8_t y_circle{11};

    switch (m_wifi_state) {
        case WifiState::WIFIDisconnected:
            break;

        case WifiState::WIFIConnecting:
            wifi_symbol(millis() / 150 % 4);
            break;

        case WifiState::WIFIConnectedVeryWeak:
            wifi_symbol(0);
            break;

        case WifiState::WIFIConnectedWeak:
            wifi_symbol(1);
            break;

        case WifiState::WIFIConnectedOK:
            wifi_symbol(2);
            break;

        case WifiState::WIFIConnectedGood:
            wifi_symbol(3);
            break;

        default:
            break;
    }
}

void Ui::set_mqtt_state(MqttState state)
{
    if (m_mqtt_state != state) {
        m_mqtt_state = state;
        m_refresh = true;
    }
}

void Ui::set_current_box_temperature(float temperature)
{
    const auto temperature_rounded{round(temperature * 10) / 10.0f};

    if (m_current_box_temperature != temperature_rounded) {
        m_current_box_temperature = temperature_rounded;
        m_refresh = true;
    }
}

void Ui::set_current_duct_temperature(float temperature)
{
    const auto temperature_rounded{round(temperature * 10) / 10.0f};

    if (m_current_duct_temperature != temperature_rounded) {
        m_current_duct_temperature = temperature_rounded;
        m_refresh = true;
    }
}

void Ui::set_current_humidity(float humidity)
{
    const auto humidity_rounded{round(humidity * 10) / 10.0f};

    if (m_current_humidity != humidity_rounded) {
        m_current_humidity = humidity_rounded;
        m_refresh = true;
    }
}

void Ui::draw_footer_box_temperature()
{
    // current box temperature, bottom left

    // draw nothing when invalid
    if (m_current_box_temperature == -127.0f) {
        return;
    }

    // convert float to string
    char buffer[6];
    int pixel_offset{0};
    if (m_current_box_temperature >= 100) {
        sprintf(buffer, "%.0f", m_current_box_temperature);
        pixel_offset = 4;
    }
    else if (m_current_box_temperature >= 10) {
        sprintf(buffer, "%.1f", m_current_box_temperature);
    }
    else {
        sprintf(buffer, "%.1f", m_current_box_temperature);
        pixel_offset = 4;
    }

    // add unit
    strcat(buffer, " C");

    // draw
    m_n8x16.draw(buffer, pixel_offset, 52);

    // m_display.invert_area(0, 50, 42, 63);
    // m_display.clear_pixel(0, 50);
    // m_display.clear_pixel(0, 63);
    // m_display.clear_pixel(42, 50);
    // m_display.clear_pixel(42, 63);
}

void Ui::draw_footer_humidity()
{
    // current humidity, bottom center

    // draw nothing when invalid
    if (m_current_humidity == -127.0f) {
        return;
    }

    // convert float to string
    char buffer[5];
    int pixel_offset{0};
    if (m_current_humidity >= 100) {
        pixel_offset = -4;
    }
    else if (m_current_humidity >= 10) {
    }
    else {
        pixel_offset = 8;
    }
    sprintf(buffer, "%.0f", m_current_humidity);

    // add unit
    strcat(buffer, " %");

    // draw
    m_n8x16.draw(buffer, 52 + pixel_offset, 52);

    // m_display.invert_area(51, 50, 81, 63);
    // m_display.clear_pixel(51, 50);
    // m_display.clear_pixel(51, 63);
    // m_display.clear_pixel(81, 50);
    // m_display.clear_pixel(81, 63);
}

void Ui::draw_footer_duct_temperature()
{
    // current duct temperature, bottom right

    // draw nothing when invalid
    if (m_current_duct_temperature == -127.0f) {
        return;
    }

    // convert float to string
    char buffer[5];
    sprintf(buffer, "%.0f", m_current_duct_temperature);

    // add unit
    strcat(buffer, " C");

    // draw
    m_n8x16.draw(buffer, 127, 52, 'r');

    // m_display.invert_area(89, 50, 127, 63);
    // m_display.clear_pixel(89, 50);
    // m_display.clear_pixel(89, 63);
    // m_display.clear_pixel(127, 50);
    // m_display.clear_pixel(127, 63);
}

void Ui::draw_large_number(const char* s, bool invert)
{
    m_n18x32.draw(s, 63, 15, 'c');

    // m_display.invert_area(0, 13, 127, 49);
}

void Ui::update()
{
    const auto now{millis()};

    // Bail out early if there is nothing to redraw.
    if ((now - m_last_update) < 15 || !m_refresh) {
        return;
    }

    m_last_update = now;

    // Switch between different layouts
    // TODO: make time interval configurable
    if (m_layout_switching && !m_freeze_layout) {
        switch (m_current_layout) {
            case LayoutA:
                if (m_last_layout_switch + 5000 < now) {
                    m_current_layout = LayoutB;
                    m_last_layout_switch = now;
                }
                break;
            case LayoutB:
                if (m_last_layout_switch + 5000 < now) {
                    m_current_layout = LayoutA;
                    m_last_layout_switch = now;
                }
            default:
                break;
        }
    }

    do {
        // Clear display
        m_display.clear();

        /*
         * Common layout elements
         */

        wifi_update();

        draw_footer_box_temperature();
        draw_footer_duct_temperature();
        draw_footer_humidity();

        /*
         * Layout specific elements
         */
        char buffer[6];
        switch (m_current_layout) {
            case LayoutA:
                // current box temperature
                // TODO: similar formatting as in draw_footer_box_temperature() ?
                if (m_current_box_temperature != -127.0f) {
                    sprintf(buffer, "%.1f", m_current_box_temperature);
                    strcat(buffer, " C");
                    draw_large_number(buffer);
                }
                break;

            case LayoutB:
                // current humidity
                // TODO: similar formatting as in draw_footer_humidity() ?
                if (m_current_humidity != -127.0f) {
                    sprintf(buffer, "%.0f", m_current_humidity);
                    strcat(buffer, " %");
                    draw_large_number(buffer);
                }
                break;

            default:
                break;
        }

        // == header test

        // target temperature/humidity
        // width = 4*8px + 2*4px = 40
        m_n8x16.draw("42.5 %", 0, -2);

        // time remaining
        // TODO a '>' symbol when target temperature not yet reached
        // TODO: max duration (also in controller and menu): 99:59 h -> 4.17 days
        // free space 116 - 40 = 76
        // width_A = 6*8px + 2*4px = 48 + 8 = 56  -> 40 + 76/2 - 56/2 = 40 + 38 - 28 = 50
        // width_B = 5*8px + 2*4px = 40 + 8 = 48  -> 40 + 76/2 - 48/2 = 40 + 38 - 24 = 54
        // x_start =
        m_n8x16.draw(">99:59 h", 50, -2);
        // m_n8x16.draw("99:59 h", 58, -2);
        // m_n8x16.draw("59:59 m", 58, -2);
        // m_display.invert_area(48, 0, 106, 12);

        // == main number test

        // current temperature
        // width = 3*18px digits + 12px dot + + 10px space + 16px unit= 54 + 12 + 10 + 16 = 92
        // centered = 128/2 - 94/2 = 64 - 46 = 18
        // m_n18x32.draw("42.5 C", 18, 16);

        // current humidity
        // m_n18x32.draw("42.5 %", 18, 15);
        // m_display.invert_area(0, 13, 127, 49);

        // TODO: set temperature / humidity
        // * inverted
        // * must also support dash
        // * increments of 0.5

        // TODO: remaining time "hh:mm h" oder "mm:ss min"

        m_display.flush();
    } while (m_display.next_segment());
}