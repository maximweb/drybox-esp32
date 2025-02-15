#pragma once

#include <Arduino.h>

// Settings for logic levels, if inverted
#define HEAT_HIGH HIGH
#define HEAT_LOW LOW
#define FAN_HIGH HIGH
#define FAN_LOW LOW

// Settings for controller hysteresis
#define DELTA_T_HIGH 1.0 // threshold above target temperature
#define DELTA_T_LOW 0    // threshold below target temperature

// Settings for timing
#define FAN_DELAY 600   // seconds the fan continues to run after heater off, 0 means always run TODO: what todo when 0 and "Cooldown" state?
#define SWITCH_DELAY 10 // minimum seconds between heater changes

// Settings for safety
#define MAX_DUCT_TEMPERATURE 110.0

#define INVALID_FLOAT -127.0f

class Controller {
public:
    enum ControllerState : uint8_t {
        Idle = 0,
        Heating = 1,
        HotDuctPause = 2, // identical to HotFan, but we want to record this
        HotFan = 3,
        HotNoFan = 4,
        Cooldown = 5, // keep fans running before returning to idle
    };

    Controller(uint8_t pin_heater, uint8_t pin_fan);

    void begin();

    void set_current_box_temperature(float temperature);
    void set_current_duct_temperature(float temperature);
    void set_target_temperature(float temperature);
    void set_duration(uint32_t duration_s);

    void update();

    ControllerState get_state();
    float get_target_temperature();
    uint32_t get_time_remaining();
    uint32_t get_time_elapsed();
    uint32_t get_time_start();
    uint32_t get_duration();

private:
    bool switch_pins(bool heater, bool fans);

    uint8_t m_pin_heater;
    uint8_t m_pin_fan;

    ControllerState m_state{ControllerState::Idle};

    float m_current_box_temperature{INVALID_FLOAT};
    float m_current_duct_temperature{INVALID_FLOAT};
    float m_current_humidity{INVALID_FLOAT};

    float m_target_temperature{INVALID_FLOAT};

    uint32_t m_duration{0};            // desired heating/drying duration
    uint32_t m_time_target_reached{0}; // for timed heating/drying
    uint32_t m_time_last_switched{0};  // for control of SWITCH_DELAY
    uint32_t m_time_enabled{0};        // time when controller was enabled
};