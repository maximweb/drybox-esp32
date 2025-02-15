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
    // External control of layout switching only when not in menu
    if ((layout < MenuStart) && (m_current_layout < MenuStart)) {
        m_current_layout = layout;
    }
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

void Ui::sensor_update()
{
    // get current time once
    const auto now{millis()};

    // Average box temperature
    float avg_temperature{0};
    uint8_t n_avg{0};

    // current dht20 values
    uint32_t last_seen{m_dht20.last_seen()};
    if (((now - last_seen) < 10000) && (last_seen > 0)) {
        // valid sensor data less than 10s old
        const auto box_temperature{round(m_dht20.temperature() * 10) / 10.0f};
        const auto humidity{round(m_dht20.relative_humidity())};
        const auto absolute_humidity{round(m_dht20.absolute_humidity() * 10) / 10.0f};

        avg_temperature += box_temperature;
        n_avg += 1;

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
        m_humidity = INVALID_FLOAT;
        m_absolute_humidity = INVALID_FLOAT;
    }

    // current duct temperature
    last_seen = m_temp_sensors.last_seen(DS18B20_DUCT_ID);
    if (((now - last_seen) < 10000) && (last_seen > 0)) {
        // valid sensor data less than 10s old
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
    last_seen = m_temp_sensors.last_seen(DS18B20_LEFT_ID);
    if (((now - last_seen) < 10000) && (last_seen > 0)) {
        // valid sensor data less than 10s old
        avg_temperature += m_temp_sensors.temperature(DS18B20_LEFT_ID);
        n_avg += 1;
    }
    last_seen = m_temp_sensors.last_seen(DS18B20_RIGHT_ID);
    if (((now - last_seen) < 10000) && (last_seen > 0)) {
        // valid sensor data less than 10s old
        avg_temperature += m_temp_sensors.temperature(DS18B20_RIGHT_ID);
        n_avg += 1;
    }

    // average temperature
    if (n_avg > 0) {
        avg_temperature /= n_avg;
        avg_temperature = round(avg_temperature * 10.0f) / 10.0f;

        if (m_box_temperature != avg_temperature) {
            m_box_temperature = avg_temperature;
            m_refresh = true;
        }
    }
    else {
        // none of the three sensors are available, set invalid
        m_box_temperature = INVALID_FLOAT;
    }

    // controller target values
    const auto target_temperature{m_controller.get_target_temperature()};
    if (m_target_temperature != target_temperature) {
        m_target_temperature = target_temperature;
        m_refresh = true;
    }

    // controller times
    int32_t time{0};
    if (m_controller.get_time_remaining() > 0) {
        // heating duration is set, use as countdown
        time = -m_controller.get_time_remaining() / 1000; // negative time in seconds
    }
    else if (m_controller.get_time_elapsed() > 0) {
        // heating duration unset, count up from when target temperature was first reached
        time = m_controller.get_time_elapsed() / 1000; // positive time in seconds
    }
    else {
        // if target temperature has not been reached, do not print time
        time = 0;
    }

    if (m_time != time) {
        m_time = time;
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

void Ui::draw_time()
{
    // time remaining, top right

    // draw nothing when no duration set or target value not yet reached
    if (m_time == 0) {
        return;
    }

    // convert milliseconds to string
    // - max time: 99:59 h -> 4.17 days
    char buffer[10];
    if (abs(m_time) >= 3600) { // >= 1 h
        sprintf(buffer, "%ld:%02lu h", m_time / 3600, (abs(m_time) % 3600) / 60);
    }
    else if (abs(m_time) >= 60) { // >= 1 min
        sprintf(buffer, "%ld:%02lu m", m_time / 60, (abs(m_time) % 60));
    }
    else {
        sprintf(buffer, "%ld s", m_time);
    }

    // draw
    if (m_time > 0) {
        m_n8x16.draw(buffer, 95, -2, 'r');
    }
    else {
        m_n8x16.draw(buffer, 99, -2, 'r');
    }
}

void Ui::draw_large_number(const char* s, int16_t x, char h_alignment /* = 'r' */, bool invert /* = false */)
{
    m_n18x32.draw(s, x, 15, h_alignment);

    // Invert area (for menu)
    if (invert) {
        m_display.invert_area(0, 13, 127, 49);
    }
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
    m_n8x16.draw(buffer, 79, 52, 'r');
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

    /*
     * Menu interaction
     */

    // Check encoder button for button press -> menu/confirm
    // We do not consume the states unless we use them.
    // Therefore, we use peek and reset the state in the respective cases
    const auto menu_button{m_encoder_button.peekState()};
    const auto encoder_direction{m_encoder.peekDirection()};
    if (m_current_layout < Layout::MenuStart && menu_button == Button::State::LongPress) {
        // Entering menu
        m_encoder_button.reset(); // consume button state

        // Save current layout states (for restore after menu)
        m_orig_layout = m_current_layout;
        m_orig_freeze_layout = m_freeze_layout;

        // Reset encoder direction
        m_encoder.reset(); // reset encoder direction

        // Freeze layout and set fixed Layout::MenuTemperature
        m_freeze_layout = true;
        m_current_layout = Layout::MenuTemperature;
        m_last_layout_switch = now; // reset and use as menu timeout

        // Set preliminary target values to current target values
        m_prelim_target_temperature = m_target_temperature;
        m_prelim_target_humidity = m_target_humidity;
        if (m_time < 0) {
            // only negative m_time corresponds to running heating duration timer
            m_prelim_target_time = -m_time;
        }
        else {
            // m_time >= 0 means no heating duration timer set
            if (m_controller.get_duration() > 0) {
                m_prelim_target_time = m_controller.get_duration();
            }
            else {
                m_prelim_target_time = INVALID_TIME;
            }
        }
    }
    else if (m_current_layout >= Layout::MenuStart && menu_button == Button::State::Click) {
        // Toggle between different menu items
        m_encoder_button.reset(); // reset button state
        m_encoder.reset();        // reset encoder direction

        switch (m_current_layout) {
            case Layout::MenuTemperature:
                // only switch to next menu itemif preliminary target temperature is set
                if (m_prelim_target_temperature != INVALID_FLOAT) {
                    m_current_layout = Layout::MenuTime;
                }
                break;
            case Layout::MenuTime:
                m_current_layout = Layout::MenuTemperature;
                break;
            default:
                break;
        }

        m_last_layout_switch = now;
    }
    else if (m_current_layout >= Layout::MenuStart && menu_button == Button::State::Idle && encoder_direction != Encoder::Direction::None) {
        // Already in menu and encoder was turned
        m_encoder.reset(); // consume encoder direction

        // Update temporary target value
        // Allow 20 to 75 °C
        // Allow 00:01 to 99:59 h
        switch (m_current_layout) {
            case Layout::MenuTemperature:
                // Update temporary target temperature
                if (encoder_direction >= Encoder::Direction::ClockwiseSlow) {
                    if (m_prelim_target_temperature == INVALID_FLOAT) {
                        m_prelim_target_temperature = 20.0f; // start at 20 °C
                    }
                    else if (m_prelim_target_temperature < 75.0f) {
                        switch (encoder_direction) {
                            case Encoder::Direction::ClockwiseSlow:
                                m_prelim_target_temperature += 1.0f;
                                break;
                            case Encoder::Direction::Clockwise:
                                m_prelim_target_temperature += 2.0f;
                                break;
                            case Encoder::Direction::ClockwiseFast:
                                m_prelim_target_temperature += 5.0f;
                                break;
                        }
                        m_prelim_target_temperature = min(m_prelim_target_temperature, 75.0f);
                    }
                }
                else if (encoder_direction <= Encoder::Direction::CounterClockwiseSlow) {
                    if (m_prelim_target_temperature <= 20.0f) {
                        // set to invalid value
                        m_prelim_target_temperature = INVALID_FLOAT;
                    }
                    else {
                        switch (encoder_direction) {
                            case Encoder::Direction::CounterClockwiseSlow:
                                m_prelim_target_temperature -= 1.0f;
                                break;
                            case Encoder::Direction::CounterClockwise:
                                m_prelim_target_temperature -= 2.0f;
                                break;
                            case Encoder::Direction::CounterClockwiseFast:
                                m_prelim_target_temperature -= 5.0f;
                                break;
                        }
                        m_prelim_target_temperature = max(m_prelim_target_temperature, 20.0f);
                    }
                }

                break;

            case Layout::MenuTime:
                // Update temporary target time
                // TODO: test what increments are comfortable

                if (encoder_direction >= Encoder::Direction::ClockwiseSlow) {
                    if (m_prelim_target_time == INVALID_TIME) {
                        m_prelim_target_time = 3600; // start at 1 h
                    }
                    else {
                        switch (encoder_direction) {
                            case Encoder::Direction::ClockwiseSlow:
                                m_prelim_target_time += 60; // +1 min
                                break;
                            case Encoder::Direction::Clockwise:
                                m_prelim_target_time += 30 * 60; // +30 min
                                break;
                            case Encoder::Direction::ClockwiseFast:
                                m_prelim_target_time += 120 * 60; // +2 h
                                break;
                        }
                        // clamp to 99:59 h
                        m_prelim_target_time = min(m_prelim_target_time, (int32_t) (99 * 60 * 60 + 59 * 60));
                    }
                }
                else if (encoder_direction <= Encoder::Direction::CounterClockwiseSlow) {
                    if ((m_prelim_target_time < 60) || (m_prelim_target_time == INVALID_TIME)) {
                        // set to invalid value
                        m_prelim_target_time = INVALID_TIME;
                    }
                    else {
                        switch (encoder_direction) {
                            case Encoder::Direction::CounterClockwiseSlow:
                                m_prelim_target_time -= 60; // -1 min
                                break;
                            case Encoder::Direction::CounterClockwise:
                                m_prelim_target_time -= 30 * 60; // -30 min
                                break;
                            case Encoder::Direction::CounterClockwiseFast:
                                m_prelim_target_time -= 120 * 60; // -2 h
                                break;
                        }
                        if (m_prelim_target_time < 60) {
                            // set to invalid value
                            m_prelim_target_time = INVALID_TIME;
                        }
                    }
                }

                break;

            default:
                break;
        }

        // reset timeout
        m_last_layout_switch = now;
    }
    else if (m_current_layout >= Layout::MenuStart && menu_button == Button::State::LongPress) {
        // Leave menu and save temporary target value to controller
        m_encoder_button.reset(); // consume button state
        m_encoder.reset();        // consume encoder direction

        // Save preliminary target values to controller
        m_controller.set_target_temperature(m_prelim_target_temperature);
        if (m_prelim_target_time != INVALID_TIME) {
            m_controller.set_duration((uint32_t) m_prelim_target_time);
        }
        else {
            m_controller.set_duration(0);
        }

        // Discard temporary target value
        m_prelim_target_temperature = INVALID_FLOAT;
        m_prelim_target_humidity = INVALID_FLOAT;
        m_prelim_target_time = INVALID_TIME;

        // Restore original layout states
        m_current_layout = m_orig_layout;
        m_freeze_layout = m_orig_freeze_layout;
        m_last_layout_switch = now; // reset and use as cycle timer
    }
    else if (m_current_layout >= Layout::MenuStart && menu_button == Button::State::Idle && encoder_direction == Encoder::Direction::None) {
        // Leave menu and discard temporary target value -> reset to previous value

        if ((now - m_last_layout_switch) > 10000) {
            // Timeout after 10s of inactivity in menu

            // Discard temporary target value
            m_prelim_target_temperature = INVALID_FLOAT;
            m_prelim_target_humidity = INVALID_FLOAT;
            m_prelim_target_time = INVALID_TIME;

            // Restore original layout states
            m_current_layout = m_orig_layout;
            m_freeze_layout = m_orig_freeze_layout;
            m_last_layout_switch = now; // reset and use as cycle timer
        }
    }

    // Toggle between different layouts
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
                    if (m_time != 0) {
                        m_current_layout = LayoutTime;
                    }
                    else {
                        m_current_layout = LayoutBoxTemperature;
                    }
                    m_last_layout_switch = now;
                }
                break;
            case LayoutTime:
                if (m_last_layout_switch + LAYOUT_SWITCH_INTERVAL < now) {
                    m_current_layout = LayoutBoxTemperature;
                    m_last_layout_switch = now;
                }
            default:
                // menu layouts are handled separately
                break;
        }
    }

    // Default UI (outside of menu)
    do {
        // Reinitalize display
        m_display.soft_reset(); // reset display settings, reduces glitches
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
        draw_target_value();
        draw_time(); // countup/countdown starting when target temperature was first reached

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
                // current box temperature 'dd.d C' -> 6 characters

                // Serial.println("LayoutBoxTemperature");

                // TODO: similar formatting as in draw_footer_box_temperature() ?
                if (m_box_temperature != INVALID_FLOAT) {
                    sprintf(buffer, "%.1f C", m_box_temperature);
                    draw_large_number(buffer, 115, 'r'); // always right aligned keeps unit in place
                } // TODO: what to print for invalid values?
                break;

            case LayoutRelHumidity:
                // current humidity 'dd %' -> 4 characters

                // Serial.println("LayoutRelHumidity");

                if (m_humidity != INVALID_FLOAT) {
                    sprintf(buffer, "%.0f %%", m_humidity); // literal '%' needs to be escaped
                    draw_large_number(buffer, 100, 'r');    // always right aligned keeps unit in place
                } // TODO: what to print for invalid values?
                break;

            case LayoutDuctTemperature:
                // current duct temperature 'dd.d C' or 'ddd C' -> 5/6 characters

                // Serial.println("LayoutDuctTemperature");

                if (m_duct_temperature != INVALID_FLOAT) {
                    if (m_duct_temperature >= 100) {
                        // 'ddd C'
                        sprintf(buffer, "%.0f C", m_duct_temperature);
                    }
                    else {
                        // 'dd.d C'
                        sprintf(buffer, "%.1f C", m_duct_temperature);
                    }
                    draw_large_number(buffer, 115, 'r'); // always right aligned keeps unit in place
                } // TODO: what to print for invalid values?
                break;

            case LayoutAbsHumidity:
                // current absolute humidity 'dd.d #' or 'ddd #' -> 5/6 characters
                // Expected values: 0.0 - 290.9 g/m^3
                // Realistic values: < 100 g/m^3

                // Serial.println("LayoutAbsHumidity");

                if (m_absolute_humidity != INVALID_FLOAT) {
                    if (m_absolute_humidity < 100) {
                        sprintf(buffer, "%.1f #", m_absolute_humidity); // use '#' for g/m3 in font
                    }
                    else {
                        sprintf(buffer, "%.0f #", m_absolute_humidity); // use '#' for g/m3 in font
                    }
                    draw_large_number(buffer, 115, 'r'); // always right aligned keeps unit in place
                } // TODO: what to print for invalid values?
                break;

            case LayoutTime:
                // elapsed/remaining time '[-]xx:yy h' -> 8 characters

                if (m_time != 0) {
                    // convert seconds to string
                    // - max time: 99:59 h -> 4.17 days
                    if (abs(m_time) >= 3600) { // >= 1 h
                        sprintf(buffer, "%ld:%02lu h", m_time / 3600, (abs(m_time) % 360) / 60);
                    }
                    else if (abs(m_time) >= 60) { // >= 1 min
                        sprintf(buffer, "%ld:%02lu m", m_time / 60, (abs(m_time) % 60));
                    }
                    else {
                        sprintf(buffer, "%ld s", m_time);
                    }
                    if (m_time > 0) {
                        draw_large_number(buffer, 120, 'r');
                    }
                    else if (m_time < 0) {
                        draw_large_number(buffer, 127, 'r');
                    }
                }
                break;

            /*
             * Menu specific elements (large number)
             */
            case MenuStart:
                // should not be used -> forward to first menu item
                m_current_layout = Layout::MenuTemperature;
                // no break -> fall through

            case MenuTemperature:
                // show inverted preliminary target temperature 'dd C' -> 4 characters

                if (m_prelim_target_temperature != INVALID_FLOAT) {
                    sprintf(buffer, "%.0f C", m_prelim_target_temperature);
                    draw_large_number(buffer, 100, 'r', true); // always right aligned keeps unit in place
                }
                else {
                    draw_large_number("-- C", 100, 'r', true); // always right aligned keeps unit in place
                }
                break;

            case MenuTime:
                // show inverted preliminary target time 'xx:yy h' -> 7 characters

                if (m_prelim_target_time != INVALID_TIME) {
                    sprintf(buffer, "%ld:%02lu h", m_prelim_target_time / 3600, (abs(m_prelim_target_time) % 3600) / 60);
                    draw_large_number(buffer, 120, 'r', true); // always right aligned keeps unit in place
                }
                else {
                    draw_large_number("--:-- h", 120, 'r', true); // always right aligned keeps unit in place
                }

                break;

            default:
                Serial.println("default layout");
                break;
        }

        // Update display
        m_display.flush();
    } while (m_display.next_segment());
}