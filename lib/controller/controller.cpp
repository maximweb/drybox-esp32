#include "controller.h"

Controller::Controller(uint8_t pin_heater, uint8_t pin_fan)
: m_pin_heater{pin_heater}
, m_pin_fan{pin_fan}
{
}

void Controller::begin()
{
    // TODO: does this inverse order ensure that at startup it is set correctly?
    pinMode(m_pin_heater, HEAT_LOW);
    pinMode(m_pin_heater, OUTPUT);

    pinMode(m_pin_fan, FAN_LOW);
    pinMode(m_pin_fan, OUTPUT);

    switch_pins(HEAT_LOW, FAN_LOW); // redundancy
}

void Controller::set_current_box_temperature(float temperature)
{
    m_current_box_temperature = temperature;
}

void Controller::set_current_duct_temperature(float temperature)
{
    m_current_duct_temperature = temperature;
}

void Controller::set_target_temperature(float temperature)
{
    // assume anything below 20C is off
    if (temperature < 20.0) {
        m_target_temperature = INVALID_FLOAT;
    }
    else {
        if ((m_target_temperature != INVALID_FLOAT) && (min(temperature, (float) 75.0) > m_target_temperature)) {
            // target temperature increased -> reset timer
            m_time_target_reached = 0;
        }
        m_target_temperature = min(temperature, (float) 75.0);
    }
}

void Controller::set_duration(uint32_t duration_s)
{
    m_duration = duration_s * 1000;
}

bool Controller::switch_pins(bool heater, bool fan)
{

    const auto fan_current{digitalRead(m_pin_fan)};
    const auto heat_current{digitalRead(m_pin_heater)};

    if (heater == HEAT_LOW) {
        // set heater off
        // * fan state does not matter
        // * first disable heater, then handle fan

        // 1. heater off
        if (heat_current == HEAT_HIGH) {
            digitalWrite(m_pin_heater, HEAT_LOW);
        }

        // 2. fan to desired value
        if (fan_current != fan) {
            digitalWrite(m_pin_fan, fan);
        }
    }
    else if (heater == HEAT_HIGH) {
        // set heater on
        // * SAFETY: fans must also be on, otherwise do nothing
        // * first enable fan, then enable heater
        if (fan != FAN_HIGH) {
            // heater on and fan not -> will not comply
            return false;
        }

        // 1. fan on
        digitalWrite(m_pin_fan, FAN_HIGH);

        // 2. heater on
        digitalWrite(m_pin_heater, HEAT_HIGH);
    }
    else {
        // shoud never get here, SAFETY: heater off
        digitalWrite(m_pin_heater, HEAT_LOW);
    }

    // Recheck new pin state and return if successfull
    if ((digitalRead(m_pin_heater) == heater) && (digitalRead(m_pin_fan) == fan)) {
        return true;
    }
    else {
        return false;
    }
}

void Controller::update()
{

    // TODO: calculate delays etc once
    // TODO: handle global events first?
    // * duct temperature
    // * duration reached
    // * controller disabled

    const auto now{millis()};

    switch (m_state) {
        case ControllerState::Idle:
            // ensure heater off, fan off
            switch_pins(HEAT_LOW, FAN_LOW);

            m_time_enabled = 0;

            if (m_target_temperature > INVALID_FLOAT) {
                // begin heating
                m_time_enabled = now;

                if ((m_target_temperature - m_current_box_temperature) > DELTA_T_LOW) {
                    // target temperature is more than DELTA_T higher than current temperature
                    m_state = ControllerState::Heating;
                    m_time_last_switched = now;
                }
                else {
                    // target temperature already reached
                    // no need to set time last switched, as heater remains off
                    if (FAN_DELAY == 0) {
                        // fan always on
                        m_state = ControllerState::HotFan;
                    }
                    else {
                        // as controller was idle before, no need to enable fans (yet)
                        m_state = ControllerState::HotNoFan;
                    }
                    if (m_time_target_reached == 0) {
                        // record time at which target temperature was reached
                        m_time_target_reached = now;
                    }
                }
                return;
            }

            // Reset timers (if we get here)
            m_time_target_reached = 0;
            m_time_last_switched = 0;

            break;

        case ControllerState::Heating:
            // enable fan and heater, if not already on
            switch_pins(HEAT_HIGH, FAN_HIGH);

            // check duct safety
            if (m_current_duct_temperature > MAX_DUCT_TEMPERATURE) {
                m_state = ControllerState::HotDuctPause;
                m_time_last_switched = now;
                return;
            }

            // check if duration is set and reached
            if ((m_duration != 0) && (m_time_target_reached != 0) && ((now - m_time_target_reached) > m_duration)) {
                m_state = ControllerState::Cooldown;
                m_time_last_switched = now;
                m_time_enabled = 0;
                m_time_target_reached = 0;
                m_target_temperature = INVALID_FLOAT;
                return;
            }

            // check if controller was disabled
            if (m_target_temperature == INVALID_FLOAT) {
                m_state = ControllerState::Cooldown;
                m_time_last_switched = now;
                m_time_enabled = 0;
                m_time_target_reached = 0;
                return;
            }

            // check if target temperature above upper hysteresis limit
            if ((m_current_box_temperature - m_target_temperature) > DELTA_T_HIGH) {
                if (m_time_target_reached == 0) {
                    // reached target temperature for the first time, save start time
                    m_time_target_reached = now;
                }

                m_state = ControllerState::HotFan;
                m_time_last_switched = now;
                // won't check for SWITCH_DELAY here, as turning off is always instant
                return;
            }

            // otherwise, stay in Heating state
            break;

        case ControllerState::HotDuctPause:
            // pause heating when duct is too hot
            // same action as HotFan, only for tracking purposes (no break)
        case ControllerState::HotFan:
            // ensure heater off, fan on
            switch_pins(HEAT_LOW, FAN_HIGH);

            // check if duration is set and reached
            if ((m_duration != 0) && (m_time_target_reached != 0) && ((now - m_time_target_reached) > m_duration)) {
                m_state = ControllerState::Cooldown;
                m_time_last_switched = now;
                m_time_enabled = 0;
                m_time_target_reached = 0;
                m_target_temperature = INVALID_FLOAT;
                return;
            }

            // check if controller was disabled
            if (m_target_temperature == INVALID_FLOAT) {
                m_state = ControllerState::Cooldown;
                m_time_last_switched = now;
                m_time_enabled = 0;
                m_time_target_reached = 0;
                return;
            }

            // check if SWITCH delay reached and temperature too low
            if ((now - m_time_last_switched) > (SWITCH_DELAY * 1000)) {
                // SWITCH_DELAY has passed
                if ((m_target_temperature - m_current_box_temperature) > DELTA_T_LOW) {
                    // below lower hysteresis threshold, go back to heating
                    m_state = ControllerState::Heating;
                    m_time_last_switched = now;
                    return;
                }
            }

            // 5. check if FAN timeout
            if ((FAN_DELAY != 0) && ((now - m_time_last_switched) > (FAN_DELAY * 1000))) {
                // fan delay reached -> turn fan off
                m_state = ControllerState::HotNoFan;
                // no update of switch timer
                return;
            }
            break;

        case ControllerState::HotNoFan:
            // ensure heater off, fan off
            switch_pins(HEAT_LOW, FAN_LOW);

            // 1. check if duration is set and reached
            // TODO: make function?
            if ((m_duration != 0) && (m_time_target_reached != 0) && ((now - m_time_target_reached) > m_duration)) {
                m_state = ControllerState::Idle;
                // do not record time of switch

                // TODO
                return;
            }

            // 2. check if controller was disabled
            // TODO: make function?
            if (m_target_temperature == INVALID_FLOAT) {
                m_state = ControllerState::Idle;
                // do not record time of switch

                // TODO disable all timers
                return;
            }

            // 4. check if SWITCH delay reached and temperature too low
            if ((now - m_time_last_switched) > (SWITCH_DELAY * 1000)) {
                // SWITCH_DELAY has passed
                if ((m_target_temperature - m_current_box_temperature) > DELTA_T_LOW) {
                    // below lower hysteresis threshold, go back to heating
                    m_state = ControllerState::Heating;
                    m_time_last_switched = now;
                    return;
                }
            }

            break;

        case ControllerState::Cooldown:
            // ensure heater off, fan on
            switch_pins(HEAT_LOW, FAN_HIGH);

            // when FAN timeout is disabled, assume 15 min timeout
            if ((FAN_DELAY == 0) && ((now - m_time_last_switched) > 15 * 60 * 1000)) {
                // default fan delay reached -> go back to idle
                m_state = ControllerState::Idle;
            }

            // check if FAN timeout was reached
            if ((FAN_DELAY != 0) && ((now - m_time_last_switched) > (FAN_DELAY * 1000))) {
                // fan delay reached -> go back to idle
                m_state = ControllerState::Idle;
                // no update of switch timer
                return;
            }

            // 2. similar behavior as idle, but only go to states with fan enabled
            if (m_target_temperature > INVALID_FLOAT) {
                // begin heating
                m_time_enabled = now;
                if ((m_target_temperature - m_current_box_temperature) > DELTA_T_LOW) {
                    // target temperature is more than DELTA_T higher than current temperature
                    m_state = ControllerState::Heating;
                    m_time_last_switched = now;
                }
                else {
                    // target temperature already exceeded
                    // no need to set time last switched, as heater remains off
                    m_state = ControllerState::HotFan;
                    if (m_time_target_reached == 0) {
                        m_time_target_reached = now;
                    }
                }
                return;
            }

            // Stay in Cooldown, ensure that everything is reset
            m_time_enabled = 0;
            m_time_target_reached = 0;
            m_target_temperature = INVALID_FLOAT;

            break;

        default:
            // unknown state, go to Cooldown (for safety)
            m_state = ControllerState::Cooldown;
            m_time_last_switched = now;
            m_time_enabled = 0;
            m_time_target_reached = 0;
            m_target_temperature = INVALID_FLOAT;
            break;
    }
}

Controller::ControllerState Controller::get_state()
{
    return m_state;
}

float Controller::get_target_temperature()
{
    return m_target_temperature;
}

uint32_t Controller::get_time_remaining() // ms
{
    const auto now{millis()};

    // no remaining time when no duration set or target value not yet reached
    if ((m_duration == 0) || (m_time_target_reached == 0)) {
        return 0;
    }

    // calculate remaining time
    if ((now - m_time_target_reached) > m_duration) {
        return 0;
    }

    return (m_duration - (now - m_time_target_reached));
}

uint32_t Controller::get_time_elapsed() // ms
{
    const auto now{millis()};

    if (m_time_target_reached == 0) {
        return 0;
    }

    // calculate time since target value reached
    return now - m_time_target_reached;
}

uint32_t Controller::get_time_start() // ms
{
    const auto now{millis()};

    if (m_time_enabled == 0) {
        return 0;
    }

    // calculate time since controller enabaled
    return now - m_time_enabled;
}

uint32_t Controller::get_duration() // seconds
{
    return m_duration / 1000;
}