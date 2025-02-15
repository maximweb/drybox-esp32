#pragma once

#include <Arduino.h>

#include "button.h"
#include "controller.h"
#include "dht20sensor.h"
#include "display.h"
#include "ds18b20.h"
#include "fonts.h"
#include "ky040.h"

#define INVALID_FLOAT -127.0f
#define INVALID_TIME 1000000000u
#define LAYOUT_SWITCH_INTERVAL 2000 // ms

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
     */
    enum Layout : uint8_t {
        LayoutBoxTemperature = 0,
        LayoutRelHumidity = 1,
        LayoutDuctTemperature = 2,
        LayoutAbsHumidity = 3,
        LayoutTime = 4,
        MenuStart = 5, // must be after all layouts and before all menus
        MenuTemperature = 6,
        MenuTime = 7,
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

    Ui(Display& display, Dht20& dht20, Ds18b20& temp_sensors, Ky040& encoder, Button& encoder_button, Controller& controller);

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
    // void set_mqtt_state(MqttState state);

    /**
     * Update internal state and refresh display if necessary.
     */
    void update();

private:
    Display& m_display;
    Number8x16 m_n8x16;
    Number18x32 m_n18x32;
    bool m_layout_switching{false};                // enable/disable layout switching
    bool m_freeze_layout{false};                   // freeze layout setting
    Layout m_current_layout{LayoutBoxTemperature}; // current layout
    unsigned long m_last_layout_switch{0};         // millis of last layout switch or menu interaction
    // menu
    bool m_orig_freeze_layout{false};                 // original layout freeze state (saved when entering menu)
    Layout m_orig_layout{LayoutBoxTemperature};       // original layout (saved when entering menu)
    float m_prelim_target_temperature{INVALID_FLOAT}; // unconfirmed target temperature
    float m_prelim_target_humidity{INVALID_FLOAT};    // unchanged target humidity
    int32_t m_prelim_target_time{INVALID_TIME};       // unchanged target time

    Dht20& m_dht20;
    float m_box_temperature{INVALID_FLOAT};   // invalid start value
    float m_humidity{INVALID_FLOAT};          // invalid start value
    float m_absolute_humidity{INVALID_FLOAT}; // invalid start value

    Ds18b20& m_temp_sensors;
    float m_duct_temperature{INVALID_FLOAT}; // invalid start value

    Ky040& m_encoder;
    Button& m_encoder_button;

    Controller& m_controller;
    float m_target_temperature{INVALID_FLOAT}; // invalid start value
    float m_target_humidity{INVALID_FLOAT};    // invalid start value
    int32_t m_time{0};                         // invalid start value

    void wifi_symbol(uint8_t strength);
    void draw_wifi();
    WifiState m_wifi_state{WifiState::WIFIDisconnected};
    // void mqtt_update();
    // MqttState m_mqtt_state{MqttState::MQTTDisconnected};
    void sensor_update();

    void draw_target_value();
    void draw_time();

    void draw_large_number(const char* s, int16_t x, char h_alignment = 'r', bool invert = false);

    void draw_footer_box_temperature();
    void draw_footer_humidity();
    void draw_footer_duct_temperature();

    bool m_refresh{true};
    unsigned long m_last_update{0};
    const char* m_version{nullptr};
};