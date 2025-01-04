#pragma once

#include <Arduino.h>
#include <DHT20.h>
#include <Wire.h>

#include "humidity_sensor.h"

#define INVALID_FLOAT -127.0f

class Dht20 : public HumiditySensor {
public:
    Dht20();
    Dht20(TwoWire* wire);

    void begin() final;
    void update() final;

    bool is_connected() final;
    uint32_t last_seen() final;
    float temperature() final;
    float relative_humidity() final;
    float absolute_humidity() final;

private:
    TwoWire* m_wire;
    DHT20* m_sensor;

    bool m_valid{false};
    uint32_t m_time_last_seen{0};
    float m_temperature{INVALID_FLOAT};
    float m_humidity{INVALID_FLOAT};
};