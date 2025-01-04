#pragma once

#include <Arduino.h>

/**
 * Temperature sensor interface.
 */
class TemperatureSensor {
public:
    virtual void begin();

    virtual void update();

    virtual uint8_t n_sensors();

    /**
     * Read the current temperature.
     */
    virtual float temperature(uint8_t id) = 0;

    /**
     * Returns the time in ms of the last successful sensor reading.
     */
    virtual unsigned int last_seen(uint8_t id) = 0;

    /**
     * Return true if last sensor reading was successful. Indicates if the value
     * provided by 'temperature()' was successfully updated in the last cycle.
     * If false, the return temperature value is old.
     */
    virtual bool is_connected(uint8_t id) = 0;
};

/**
 * Mock temperature sensor implementation for testing.
 */
class MockTemperatureSensor : public TemperatureSensor {
public:
    void begin() final {}

    void update() final {}

    uint8_t n_sensors() final { return 3; }

    float temperature(uint8_t id) final { return 20.0f; }

    unsigned int last_seen(uint8_t id) final { return 0; }

    bool is_connected(uint8_t id) final { return true; }
};
