#include "config.h"

#include "ui.h"

// TODO: reinitialize display every x seconds? prevents misconfiguration on glitches

Ui::Ui(Display& display, Dht20& dht20, Ds18b20& temp_sensors, Ky040& encoder, Button& encoder_button, Controller& controller)
: m_display{display}
, m_n8x16{display}
, m_n18x32{display}
, m_dht20{dht20}
, m_temp_sensors{temp_sensors}
, m_encoder{encoder}
, m_encoder_button{encoder_button}
, m_controller{controller}
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

void Ui::draw_wifi()
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

// void Ui::set_mqtt_state(MqttState state)
// {
//     if (m_mqtt_state != state) {
//         m_mqtt_state = state;
//         m_refresh = true;
//     }
// }

void Ui::sensor_update()
{
    // get current time once
    const auto now{millis()};

    // current dht20 values
    uint32_t last_seen{m_dht20.last_seen()};
    if (((now - last_seen) < 60000) && (last_seen > 0)) {
        // valid sensor data less than 60s old
        const auto box_temperature{round(m_dht20.temperature() * 10) / 10.0f};
        const auto humidity{round(m_dht20.relative_humidity())};
        const auto absolute_humidity{round(m_dht20.absolute_humidity() * 10) / 10.0f};

        if (m_box_temperature != box_temperature) {
            m_box_temperature = box_temperature;
            m_refresh = true;
        }
        if (m_humidity != humidity) {
            m_humidity = humidity;
            m_refresh = true;
        }
        if (m_absolute_humidity != absolute_humidity) {
            m_absolute_humidity = absolute_humidity;
            m_refresh = true;
        }
    }
    else {
        // invalid sensor data
        m_box_temperature = INVALID_FLOAT;
        m_humidity = INVALID_FLOAT;
        m_absolute_humidity = INVALID_FLOAT;
    }

    // current duct temperature
    last_seen = m_temp_sensors.last_seen(DS18B20_DUCT_ID);
    if (((now - last_seen) < 6000) && (last_seen > 0)) {
        // valid sensor data less than 60s old
        const auto duct_temperature{round(m_temp_sensors.temperature(DS18B20_DUCT_ID) * 10) / 10.0f};

        if (m_duct_temperature != duct_temperature) {
            m_duct_temperature = duct_temperature;
            m_refresh = true;
        }
    }
    else {
        // invalid sensor data
        m_duct_temperature = INVALID_FLOAT;
    }

    // controller target values
    const auto target_temperature{m_controller.get_target_temperature()};
    const auto target_humidity{m_controller.get_target_humidity()};
    const auto time_remaining{m_controller.get_time_remaining()};
    if (m_target_temperature != target_temperature) {
        m_target_temperature = target_temperature;
        m_refresh = true;
    }
    if (m_target_humidity != target_humidity) {
        m_target_humidity = target_humidity;
        m_refresh = true;
    }
    if (m_time_remaining != time_remaining) {
        m_time_remaining = time_remaining;
        m_refresh = true;
    }
}

void Ui::draw_target_value()
{
    // target temperature/humidity, top left

    // early checks
    if ((m_target_temperature == INVALID_FLOAT) && (m_target_humidity == INVALID_FLOAT)) {
        // draw nothing when controller is disabled
        return;
    }
    if ((m_target_temperature != INVALID_FLOAT) && (m_target_humidity != INVALID_FLOAT)) {
        // draw nothing when conflicting values
        return;
    }

    // convert float to string
    char buffer[4];
    if (m_target_temperature != INVALID_FLOAT) {
        sprintf(buffer, "%.0f C", m_target_temperature);
    }
    else {
        sprintf(buffer, "%.0f %%", m_target_humidity); // literal '%' needs to be escaped
    }

    // draw
    m_n8x16.draw(buffer, 27, -2, 'r');
}

void Ui::draw_time_remaining()
{
    // time remaining, top right

    // draw nothing when no duration set or target value not yet reached
    if (m_time_remaining == 0) {
        return;
    }

    // convert milliseconds to string
    // - max time: 99:59 h -> 4.17 days
    char buffer[7];
    if (m_time_remaining >= 3600000) { // >= 1 h
        sprintf(buffer, "%lu:%02lu h", m_time_remaining / 3600000, (m_time_remaining % 3600000) / 60000);
    }
    else if (m_time_remaining >= 60000) { // >= 1 min
        sprintf(buffer, "%lu:%02lu m", m_time_remaining / 60000, (m_time_remaining % 60000) / 1000);
    }
    else if (m_time_remaining >= 1000) { // >= 1 s
        sprintf(buffer, "%lu:%02lu s", m_time_remaining / 1000, (m_time_remaining % 1000) / 10);
    }
    else { // < 1 s
        sprintf(buffer, "0:%lu s", m_time_remaining);
    }

    // draw
    m_n8x16.draw(buffer, 95, -2, 'r');
}

void Ui::draw_large_number(const char* s, bool invert)
{
    m_n18x32.draw(s, 63, 15, 'c');

    // m_display.invert_area(0, 13, 127, 49);
}

void Ui::draw_footer_box_temperature()
{
    // current box temperature, bottom left

    // draw nothing when invalid
    if (m_box_temperature == INVALID_FLOAT) {
        return;
    }

    // convert float to string
    char buffer[6];
    int pixel_offset{0};
    if (m_box_temperature >= 100) {
        sprintf(buffer, "%.0f C", m_box_temperature);
    }
    else {
        sprintf(buffer, "%.1f C", m_box_temperature);
    }

    // draw
    m_n8x16.draw(buffer, 39, 52, 'r');

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
    if (m_humidity == INVALID_FLOAT) {
        return;
    }

    // convert float to string
    char buffer[6];
    sprintf(buffer, "%.0f %%", m_humidity); // literal '%' needs to be escaped

    // draw
    // m_n8x16.draw(buffer, 80, 52, 'l');
    m_n8x16.draw(buffer, 79, 52, 'r');

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
    if (m_duct_temperature == INVALID_FLOAT) {
        return;
    }

    // convert float to string
    char buffer[5];
    sprintf(buffer, "%.0f", m_duct_temperature);

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

void Ui::update()
{
    const auto now{millis()};

    // Bail out early if there is nothing to redraw.
    if ((now - m_last_update) < 50 || !m_refresh) {
        return;
    }

    m_last_update = now;
    sensor_update();

    // Check encoder button for button press -> menu/confirm
    const auto menu_button{m_encoder_button.getState()};
    const auto encoder_direction{m_encoder.direction()};
    if (m_current_layout < Layout::MenuStart && menu_button == Button::State::LongPress) {
        // Entering menu

        // TODO: save current layout and m_freeze_layout states (for restore after menu)

        // TODO: Reset encoder counter

        // TODO: freeze layout and set fixed Layout::MenuTemperature
    }
    else if (m_current_layout >= Layout::MenuStart && menu_button == Button::State::Click) {
        // Toggle between different menu items
    }
    else if (m_current_layout >= Layout::MenuStart && menu_button == Button::State::Idle && encoder_direction != Encoder::Direction::None) {
        // Already in menu and encoder was turned

        // reset timeout and update temporary target value
    }
    else if (m_current_layout >= Layout::MenuStart && menu_button == Button::State::LongPress) {
        // Leave menu and save temporary target value
    }
    else if (m_current_layout >= Layout::MenuStart && menu_button == Button::State::Idle && encoder_direction == Encoder::Direction::None) {
        // Leave menu and discard temporary target value -> reset to previous value
    }

    // Toggle between different layouts
    // TODO: make time interval configurable
    if (m_layout_switching && !m_freeze_layout) {
        switch (m_current_layout) {
            case LayoutBoxTemperature:
                if (m_last_layout_switch + LAYOUT_SWITCH_INTERVAL < now) {
                    m_current_layout = LayoutRelHumidity;
                    m_last_layout_switch = now;
                }
                break;
            case LayoutRelHumidity:
                if (m_last_layout_switch + LAYOUT_SWITCH_INTERVAL < now) {
                    m_current_layout = LayoutAbsHumidity;
                    m_last_layout_switch = now;
                }
                break;
            case LayoutDuctTemperature:
                // not included in layout rotation (always in footer)
                if (m_last_layout_switch + LAYOUT_SWITCH_INTERVAL < now) {
                    m_current_layout = LayoutAbsHumidity;
                    m_last_layout_switch = now;
                }
                break;
            case LayoutAbsHumidity:
                if (m_last_layout_switch + LAYOUT_SWITCH_INTERVAL < now) {
                    m_current_layout = LayoutBoxTemperature;
                    m_last_layout_switch = now;
                }
                break;
            default:
                break;
        }
    }

    do {
        // Reinitalize display
        // m_display.begin(); // TODO: will blink too much, do less? or only on layout change?
        // Clear display
        m_display.clear();

        /*
         * Common layout elements
         */

        /* HEADER
         * target value: left, width 28px, x=0-27px, ending at 27px
         * gap: 28-47px (20px)
         * remaining time: centerered, width 48px, x=48-95px, ending at 95px
         * gap: 96-115px (20px)
         * wifi symbol: right, width 11px, x=117-127px, centered at 122px
         */
        //
        draw_wifi();

        // m_target_temperature = 42.5f; // for testing
        draw_target_value();
        // m_time_remaining = now; // for testing
        draw_time_remaining();

        /* FOOTER
         * box temperature: left, width 40px, x=0-39px, ending at 39px
         * gap: 40-51px (12px)
         * humidity: center, width 28px, x=52-79px, ending at 79px
         * gap: 80-91px (12px)
         * duct temperature: right, width 36px, x=92-127px, ending at 127px
         */

        draw_footer_box_temperature();
        draw_footer_duct_temperature();
        draw_footer_humidity();

        /*
         * Layout specific elements (large number)
         */
        char buffer[10];
        switch (m_current_layout) {
            case LayoutBoxTemperature:
                // current box temperature
                Serial.println("LayoutBoxTemperature");

                // TODO: similar formatting as in draw_footer_box_temperature() ?
                if (m_box_temperature != INVALID_FLOAT) {
                    sprintf(buffer, "%.1f C", m_box_temperature);
                    draw_large_number(buffer);
                } // TODO: what to print for invalid values?
                break;

            case LayoutRelHumidity:
                // current humidity
                Serial.println("LayoutRelHumidity");

                // TODO: similar formatting as in draw_footer_humidity() ?
                if (m_humidity != INVALID_FLOAT) {
                    sprintf(buffer, "%.0f %%", m_humidity); // literal '%' needs to be escaped
                    draw_large_number(buffer);
                } // TODO: what to print for invalid values?
                break;

            case LayoutDuctTemperature:
                // current duct temperature
                Serial.println("LayoutDuctTemperature");

                if (m_duct_temperature != INVALID_FLOAT) {
                    sprintf(buffer, "%.0f C", m_duct_temperature);
                    draw_large_number(buffer);
                } // TODO: what to print for invalid values?
                break;

            case LayoutAbsHumidity:
                // current absolute humidity
                Serial.println("LayoutAbsHumidity");

                if (m_absolute_humidity != INVALID_FLOAT) {
                    // Expected values: 0.0 - 290.9 g/m^3
                    // Realistic values: < 100 g/m^3
                    if (m_absolute_humidity < 100) {
                        sprintf(buffer, "%.1f #", m_absolute_humidity); // use '#' for g/m3 in font
                    }
                    else {
                        sprintf(buffer, "%.0f #", m_absolute_humidity); // use '#' for g/m3 in font
                    }
                    draw_large_number(buffer);
                } // TODO: what to print for invalid values?
                break;

            default:
                Serial.println("default layout");
                break;
        }

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