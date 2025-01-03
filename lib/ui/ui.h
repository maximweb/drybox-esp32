#pragma once

#include "display.h"
#include "fonts.h"
#include <Arduino.h>

// TODO
//
// left full height two digit temperature (32x64)
//
// right TOP 70-127 = 57 -> 2-3 digit temperatures + ICON (set temp and duct temp)
// 57 / 6 = 9.5 font width?   8x13 -> 26 total height, 38 remaining for symbols
//
// header with two/three temperatures: target temp, duct temp (avg. side temps) -> small font (top)
// * target ICON + SPACE + 2 digits; duct 3 digits + SPACE + ICON -> 10 chars incl. space 128/10 = 12.8 --> 6x13
// https://pictogrammers.com/library/mdi/
// thermometer for target -> https://pictogrammers.com/library/mdi/icon/thermometer-auto/
//
// target temp inverted? -> swap with current temp when in menu?
// wifi -> smaller, move down
// icon for heating ON
// icon for fan on?
// current temperature (avg. of all sensors OR only dht20 sensor -> auto depending on active sensors)
// menu for setting temperature with encoder (inverted display when setting?) -> large font
// reminder: display 64x128
// -> header 12px height fonts?
// ->
class Ui {
public:
    /**
     * States for multiple UI layouts.
     *
     */
    enum Layout : uint8_t {
        LayoutA = 1,
        LayoutB = 2,
        Menu = 3,
    };

    enum MenuState : uint8_t {
        Off = 0,

    };

    enum WifiState : uint8_t {
        WIFIDisconnected = 1,
        WIFIConnecting = 2,
        WIFIConnectedVeryWeak = 3,
        WIFIConnectedWeak = 4,
        WIFIConnectedOK = 5,
        WIFIConnectedGood = 6,
    };

    enum MqttState : uint8_t {
        MQTTDisconnected = 1,
        MQTTConnecting = 2,
        MQTTConnected = 3,
    };

    Ui(Display& display);

    /**
     * Enable/disable layout switching.
     */
    void set_layout_switching(bool enabled);

    /**
     * Freeze current layout.
     *
     * @return Current layout.
     */
    Layout freeze_layout(bool freeze);

    /**
     * Get active layout.
     */
    Layout current_layout();

    /**
     * Set current layout.
     */
    void set_layout(Layout layout);

    void set_wifi_state(WifiState state);

    void set_mqtt_state(MqttState state);

    void set_current_box_temperature(float temperature);
    void set_current_duct_temperature(float temperature);
    void set_current_humidity(float humidity);

    /**
     * Update internal state and refresh display if necessary.
     */
    void update();

private:
    void wifi_symbol(uint8_t strength);
    void wifi_update();
    void mqtt_update();

    void draw_large_number(const char* s, bool invert = false);

    void draw_footer_box_temperature();
    void draw_footer_humidity();
    void draw_footer_duct_temperature();

    Display& m_display;
    Number8x16 m_n8x16;
    Number18x32 m_n18x32;
    bool m_layout_switching{false};
    bool m_freeze_layout{false};
    Layout m_current_layout{LayoutA};
    unsigned long m_last_layout_switch{0};

    WifiState m_wifi_state{WifiState::WIFIDisconnected};
    MqttState m_mqtt_state{MqttState::MQTTDisconnected};

    float m_current_box_temperature{-127.0f};  // invalid start value
    float m_current_duct_temperature{-127.0f}; // invalid start value
    float m_current_humidity{-127.0f};         // invalid start value

    bool m_refresh{true};
    unsigned long m_last_update{0};
    const char* m_version{nullptr};
};