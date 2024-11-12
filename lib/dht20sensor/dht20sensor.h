#pragma once

#include "humidity_sensor.h"
#include <Arduino.h>
#include <DHT20.h>
#include <Wire.h>

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
    float m_temperature;
    float m_humidity;
};