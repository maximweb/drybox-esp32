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

void Controller::set_current_humidity(float humidity)
{
    m_current_humidity = humidity;
}

void Controller::set_target_temperature(float temperature)
{
    // assume anything below 20C is off
    if (temperature < 20.0) {
        m_target_temperature = 0;
    }
    else {
        if ((m_target_temperature != 0) && (min(temperature, (float) 90.0) > m_target_temperature)) {
            // target temperature increased -> reset timer
            m_time_target_reached = 0;
        }
        m_target_temperature = min(temperature, (float) 90.0);
    }
    m_target_humidity = 0;

    // update(); // don't do this here, as we want a single point (after update()) to get state for MQTT
}

void Controller::set_target_humidity(float humidity)
{
    // assume anything outside 5 to 95 off
    // TODO: narrow down to realistic values
    if (humidity < 5.0 || humidity > 95.0) {
        m_target_humidity = 0;
    }
    else {
        if ((m_target_humidity != 0) && (max(min(humidity, (float) 95.0), (float) 5.0) < m_target_humidity)) {
            // target humidity decreased -> reset timer
            m_time_target_reached = 0;
        }
        m_target_humidity = humidity;
    }
    m_target_temperature = 0;

    // update(); // don't do this here, as we want a single point (after update()) to get state for MQTT
}

void Controller::set_duration(uint32_t duration_s)
{
    m_duration = duration_s * 1000;

    // update(); // don't do this here, as we want a single point (after update()) to get state for MQTT
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

    switch (m_state) {
        case ControllerState::Idle:
            // ensure heater off, fan off
            switch_pins(HEAT_LOW, FAN_LOW);

            if (m_target_temperature > 0) {
                // begin heating
                if ((m_target_temperature - m_current_box_temperature) > DELTA_T) {
                    // target temperature is more than DELTA_T higher than current temperature
                    m_state = ControllerState::Heating;
                    m_time_last_switched = millis();
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
                    m_time_target_reached = millis();
                }
                return;
            }

            if (m_current_humidity > 0) {
                // begin drying
                if ((m_current_humidity - m_target_humidity) > DELTA_H) {
                    // target humidity is more than DELTA_H lower than current humidity
                    m_state = ControllerState::Drying;
                    m_time_last_switched = millis();
                }
                else {
                    // target humidity already reached
                    if (FAN_DELAY == 0) {
                        // fan always on
                        m_state = ControllerState::DryFan;
                    }
                    else {
                        // as controller was idle before, no need to enable fans (yet)
                        m_state = ControllerState::DryNoFan;
                    }
                    m_time_target_reached = millis();
                }
                return;
            }

            // Reset timers
            m_time_target_reached = 0;
            m_time_last_switched = 0;

            break;

        case ControllerState::Heating:
            // enable fan and heater, if not already on
            switch_pins(HEAT_HIGH, FAN_HIGH);

            // 1. check duct safety
            if (m_current_duct_temperature > MAX_DUCT_TEMPERATURE) {
                m_state = ControllerState::HotDuctPause;
                m_time_last_switched = millis();
                return;
            }

            // 2. check if duration is set and reached
            if ((m_duration != 0) && (m_time_target_reached != 0) && ((millis() - m_time_target_reached) > m_duration)) {
                m_state = ControllerState::Cooldown;
                m_time_last_switched = millis();
                return;
            }

            // 3. check if controller was disabled
            if ((m_target_temperature == 0) && (m_target_humidity == 0)) {
                m_state = ControllerState::Cooldown;
                m_time_last_switched = millis();
                return;
            }

            // 4. check if controller mode was changed
            if (m_target_humidity != 0) {
                m_state = ControllerState::Drying;
                // do not record time of switch
                return;
            }

            // 5. check if target temperature reached for the first time -> begin timer
            if (((m_current_box_temperature - m_target_temperature) > 0) && m_time_target_reached == 0) {
                m_time_target_reached = millis();
            }

            // 6. check if target temperature above upper hysteresis limit
            if ((m_current_box_temperature - m_target_temperature) > DELTA_T) {
                m_state = ControllerState::HotFan;
                m_time_last_switched = millis();
                // won't check for SWITCH_DELAY here, as turning off always immediate
                return;
            }

            // otherwise, stay in Heating state

            break;

        case ControllerState::HotDuctPause:
            // same action as HotFan, only for tracking purposes
        case ControllerState::HotFan:
            // ensure heater off, fan on
            switch_pins(HEAT_LOW, FAN_HIGH);

            // 1. check if duration is set and reached
            // TODO: make function?
            if ((m_duration != 0) && (m_time_target_reached != 0) && ((millis() - m_time_target_reached) > m_duration)) {
                m_state = ControllerState::Cooldown;
                m_time_last_switched = millis();
                return;
            }

            // 2. check if controller was disabled
            // TODO: make function?
            if ((m_target_temperature == 0) && (m_target_humidity == 0)) {
                m_state = ControllerState::Cooldown;
                m_time_last_switched = millis();
                return;
            }

            // 3. check if controller mode was changed
            if (m_target_humidity != 0) {
                m_state = ControllerState::DryFan;
                // do not record time of switch
                return;
            }

            // 4. check if SWITCH delay reached and temperature too low
            if ((millis() - m_time_last_switched) > (SWITCH_DELAY * 1000)) {
                // SWITCH_DELAY has passed
                if ((m_target_temperature - m_current_box_temperature) > DELTA_T) {
                    // below lower hysteresis threshold, go back to heating
                    m_state = ControllerState::Heating;
                    m_time_last_switched = millis();
                    return;
                }
            }

            // 5. check if FAN timeout
            if ((FAN_DELAY != 0) && ((millis() - m_time_last_switched) > (FAN_DELAY * 1000))) {
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
            if ((m_duration != 0) && (m_time_target_reached != 0) && ((millis() - m_time_target_reached) > m_duration)) {
                m_state = ControllerState::Idle;
                // do not record time of switch
                return;
            }

            // 2. check if controller was disabled
            // TODO: make function?
            if ((m_target_temperature == 0) && (m_target_humidity == 0)) {
                m_state = ControllerState::Idle;
                // do not record time of switch
                return;
            }

            // 3. check if controller mode was changed
            if (m_target_humidity != 0) {
                m_state = ControllerState::DryNoFan;
                // do not record time of switch
                return;
            }

            // 4. check if SWITCH delay reached and temperature too low
            if ((millis() - m_time_last_switched) > (SWITCH_DELAY * 1000)) {
                // SWITCH_DELAY has passed
                if ((m_target_temperature - m_current_box_temperature) > DELTA_T) {
                    // below lower hysteresis threshold, go back to heating
                    m_state = ControllerState::Heating;
                    m_time_last_switched = millis();
                    return;
                }
            }

            break;

        case ControllerState::Drying:
            break;

        case ControllerState::DryDuctPause:
            // same action as DryFan, only for tracking purposes
        case ControllerState::DryFan:
            break;

        case ControllerState::DryNoFan:
            break;

        case ControllerState::Cooldown:
            // ensure heater off, fan on
            switch_pins(HEAT_LOW, FAN_HIGH);

            // TODO: we will never get out of this state when fan timeout disabled and no new controller setting
            // should we add a long timeout, e.g. 30 min after which fan turn off?

            // 1. check if FAN timeout
            if ((FAN_DELAY != 0) && ((millis() - m_time_last_switched) > (FAN_DELAY * 1000))) {
                // fan delay reached -> go back to idle
                m_state = ControllerState::Idle;
                // no update of switch timer
                return;
            }

            // 2. similar behavior as idle, but only go to states with fan enabled
            if (m_target_temperature > 0) {
                // begin heating
                if ((m_target_temperature - m_current_box_temperature) > DELTA_T) {
                    // target temperature is more than DELTA_T higher than current temperature
                    m_state = ControllerState::Heating;
                    m_time_last_switched = millis();
                }
                else {
                    // target temperature already reached
                    // no need to set time last switched, as heater remains off
                    m_state = ControllerState::HotFan;
                    m_time_target_reached = millis();
                }
                return;
            }

            if (m_current_humidity > 0) {
                // begin drying
                if ((m_current_humidity - m_target_humidity) > DELTA_H) {
                    // target humidity is more than DELTA_H lower than current humidity
                    m_state = ControllerState::Drying;
                    m_time_last_switched = millis();
                }
                else {
                    // target humidity already reached
                    // no need to set time last switched, as heater remains off
                    m_state = ControllerState::DryFan;
                    m_time_target_reached = millis();
                }
                return;
            }

            // Only reset duration timer, state switch may still be needed for long fan timeout
            m_time_target_reached = 0;

            break;

        default:
            // unknown state, go for Cooldown for safety
            m_state = ControllerState::Cooldown;
            m_time_last_switched = millis();
            break;
    }
}