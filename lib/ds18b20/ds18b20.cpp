#include "ds18b20.h"

Ds18b20::Ds18b20(uint8_t pin, uint8_t bit_resolution, uint16_t timeout_connected_ms, uint16_t interval_reconnect_ms)
: m_pin{pin}
, m_wire{pin}
, m_sensors{&m_wire}
, m_resolution{bit_resolution}
, m_timeout_connected_ms{timeout_connected_ms}
, m_interval_reconnect_ms{interval_reconnect_ms}
, m_conversion_duration_ms{m_sensors.millisToWaitForConversion(bit_resolution * 1.1)}
{
}

void Ds18b20::begin()
{
    // Initialize all addresses to zero
    for (uint8_t id = 0; id < 3; id++) {
        delete_address(id);
    }

    // Find and store sensor addresses in memory
    reset();
}

void Ds18b20::reset()
{
    m_wire.reset();
    m_sensors.begin();

    // Identify and store 8-byte addresses of all device
    for (uint8_t id = 0; id < 3; id++) {
        reset(id); // reinitialize address search
    }

    // Initial timing
    const auto time{millis()};
    m_last_reconnect = time;

    // Settings
    m_sensors.setWaitForConversion(false);
    m_sensors.setResolution(m_resolution);

    // Initial temperature request to all sensors
    m_sensors.requestTemperatures();
    m_last_request = time;
}

uint8_t Ds18b20::find_address_in_memory(DeviceAddress addr)
{
    for (uint8_t id = 0; id < 3; id++) {
        if (compare_address(addr, m_address[id])) {
            return id;
        }
    }
    return 255;
}

void Ds18b20::reset(uint8_t id)
{
    delete_address(id);
    for (uint8_t i_wire = 0; i_wire < m_sensors.getDeviceCount(); i_wire++) {
        DeviceAddress newAddress{0, 0, 0, 0, 0, 0, 0, 0};

        // no address found, no reason to continue
        if (!m_sensors.getAddress(newAddress, i_wire)) {
            continue; // continue with next found i_wire address
        }

        if (newAddress[0] != 0x28) {
            // not a ds18xxx device, ignoring
            continue;
        }

        // check if other ids already store same address
        if (find_address_in_memory(newAddress) < 255) {
            // found address in memory, continue with next wire address
            continue;
        }

        // newly found address not yet in memory, store it and apply resolution
        for (uint8_t k = 0; k < 8; k++) {
            m_address[id][k] = newAddress[k];
        }
        m_sensors.setResolution(m_address[id], m_resolution, true);
    }
}

void Ds18b20::delete_address(uint8_t id)
{
    for (uint8_t i = 0; i < 8; i++) {
        m_address[id][i] = 0x00;
    }
}

bool Ds18b20::compare_address(DeviceAddress addrA, DeviceAddress addrB)
{
    for (uint8_t i = 0; i < 8; i++) {
        if (addrA[i] != addrB[i]) {
            return false;
        }
    }
    return true;
}

void Ds18b20::update()
{
    const auto time{millis()};
    const uint32_t elapsed_last_seen_ms[3] = {
      time - m_last_seen[0],
      time - m_last_seen[1],
      time - m_last_seen[2],
    };
    const uint32_t elapsed_request_ms = {time - m_last_request};
    const uint32_t elapsed_reconnect_ms = {time - m_last_reconnect};

    bool reset_required{false};
    bool request_required{false};

    for (uint8_t id = 0; id < 3; id++) {
        if (elapsed_last_seen_ms[id] > m_timeout_connected_ms) { // timeout until sensor deemed disconnected
            m_disconnected[id] = true;
            if (elapsed_reconnect_ms > m_interval_reconnect_ms) { // try reconnect when interval reached
                reset_required = true;                            // need to reconnect, call once for all sensors
            }
        }

        // do not request temperature from invalid address
        if (m_address[id][0] != 0x28) {
            continue;
        }

        if (elapsed_request_ms > m_conversion_duration_ms) {
            int16_t raw_T = m_sensors.getTemp(m_address[id]);
            if ((raw_T > DEVICE_DISCONNECTED_RAW) && ((raw_T >> (12 + 3 - m_resolution)) != (0x0550 >> (12 - m_resolution)))) {
                m_last_temperature[id] = m_sensors.rawToCelsius(raw_T);
                m_last_seen[id] = time;
                m_disconnected[id] = false;
                request_required = true; // triggers new temperature request, call once for all sensors
            }
        }
    }

    if (reset_required) {
        // single attempt to get (new) sensor addresses for all sensors
        reset(); // includes temperature request
    }
    else if (request_required) {
        // single temperature request for all sensors
        m_sensors.requestTemperatures();
        m_last_request = time;
    }
}

uint8_t Ds18b20::n_sensors()
{
    return m_sensors.getDeviceCount();
}

unsigned int Ds18b20::last_seen(uint8_t id)
{
    return m_last_seen[id];
}

bool Ds18b20::is_connected(uint8_t id)
{
    return !m_disconnected[id];
}

float Ds18b20::temperature(uint8_t id)
{
    return m_last_temperature[id];
}