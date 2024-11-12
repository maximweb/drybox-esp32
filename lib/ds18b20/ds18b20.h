#pragma once

/* Deactivate the alarm function of DallasTemperature library */
#define REQUIRESALARMS false

#include <Arduino.h>

#include "tsensor.h"
#include <DallasTemperature.h>
#include <OneWire.h>

class Ds18b20 : public TemperatureSensor {
public:
    Ds18b20(uint8_t, uint8_t, uint16_t, uint16_t);

    void begin() final;
    void update() final;
    uint8_t n_sensors() final;
    float temperature(uint8_t id) final;
    unsigned int last_seen(uint8_t id) final;
    bool is_connected(uint8_t id) final;

private:
    uint8_t m_pin;
    OneWire m_wire;
    DallasTemperature m_sensors;
    const uint8_t m_resolution{9};
    const uint16_t m_timeout_connected_ms{2000};   // set by constructor
    const uint16_t m_interval_reconnect_ms{10000}; // set by constructor
    uint16_t m_conversion_duration_ms{1000};       // set by constructor
    DeviceAddress m_address[3];                    // initialization in begin()
    float m_last_temperature[3] = {0.0f, 0.0f, 0.0f};
    unsigned long m_last_seen[3]{0, 0, 0}; // millis of last successfull temperature read
    unsigned long m_last_request{0};       // millis of last temperature request
    unsigned long m_last_reconnect{0};     // millis of last reconnect attempt
    bool m_disconnected[3]{true, true, true};

    void reset();
    void reset(uint8_t id);
    void delete_address(uint8_t id);
    bool compare_address(DeviceAddress addrA, DeviceAddress addrB);
    uint8_t find_address_in_memory(DeviceAddress addr);
};
