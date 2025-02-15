#pragma once

#include <Arduino.h>

/**
 * Abstract humidity sensor interface.
 */
class HumiditySensor {
public:
    virtual void begin() = 0;
    virtual void update() = 0;
    virtual bool is_connected() = 0;
    virtual uint32_t last_seen() = 0;
    virtual float temperature() = 0;
    virtual float relative_humidity() = 0;
    virtual float absolute_humidity() = 0;
};

/**
 * Mock humidity sensor implementation for testing.
 */

class MockHumiditySensor : public HumiditySensor {
public:
    void begin() final {}
    void update() final {}
    bool is_connected() final { return true; }
    uint32_t last_seen() final { return millis(); }
    float temperature() final { return -127.0; }
    float relative_humidity() final { return -127.0; }
    float absolute_humidity() final { return -127.0; }
};