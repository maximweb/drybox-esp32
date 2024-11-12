#pragma once

#include <Arduino.h>

// Settings for logic levels, if inverted
#define HEAT_HIGH HIGH
#define HEAT_LOW LOW
#define FAN_HIGH HIGH
#define FAN_LOW LOW

// Settings for controller hysteresis
#define DELTA_T 0.5
#define DELTA_H 1.0

// Settings for timing
#define FAN_DELAY 60    // seconds the fan continues to run after heater off, 0 means always run TODO: what todo when 0 and "Cooldown" state?
#define SWITCH_DELAY 30 // minimum seconds between heater changes

// Settings for safety
#define MAX_DUCT_TEMPERATURE 120.0

class Controller {
public:
    enum ControllerState : uint8_t {
        Idle = 0,
        Heating = 1,
        HotDuctPause = 2, // identical to HotFan, but we want to record this
        HotFan = 3,
        HotNoFan = 4,
        Drying = 5,
        DryDuctPause = 6, // identical to DryFan, but we want to record this
        DryFan = 7,
        DryNoFan = 8,
        Cooldown = 9, // keep fans running before returning to idle
    };

    Controller(uint8_t pin_heater, uint8_t pin_fan);

    void begin();

    void set_current_box_temperature(float temperature);
    void set_current_duct_temperature(float temperature);
    void set_current_humidity(float humidity);
    void set_target_temperature(float temperature);
    void set_target_humidity(float humidity);
    void set_duration(uint32_t duration_s);

    void update();

    ControllerState get_state();

private:
    bool switch_pins(bool heater, bool fans);

    uint8_t m_pin_heater;
    uint8_t m_pin_fan;

    ControllerState m_state{ControllerState::Idle};

    float m_current_box_temperature{0};
    float m_current_duct_temperature{0};
    float m_current_humidity{0};

    float m_target_temperature{0};
    float m_target_humidity{0};

    uint32_t m_duration{0};            // desired heating/drying duration
    uint32_t m_time_target_reached{0}; // for timed heating/drying
    uint32_t m_time_last_switched{0};  // for control of SWITCH_DELAY
};