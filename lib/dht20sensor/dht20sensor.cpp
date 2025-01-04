#include "dht20sensor.h"

Dht20::Dht20()
{
    m_wire = &Wire;
    m_sensor = new DHT20(m_wire);
}

Dht20::Dht20(TwoWire* wire)
: m_wire{wire}
, m_sensor{new DHT20(m_wire)}
{
}

void Dht20::begin()
{
    (*m_wire).begin();
    (*m_sensor).begin();
    (*m_sensor).requestData();
}

void Dht20::update()
{
    // Reconnect if necessary
    if (!(*m_sensor).isConnected()) {
        m_valid = false;
        (*m_sensor).requestData();
        return;
    }

    // Wait 1s between requestData() and readData()
    const uint32_t now{millis()};
    if (((*m_sensor).lastRequest() + 1000) >= now) {
        return;
    }

    // Read data
    const int16_t data = (*m_sensor).readData(); // returns number of bytes received or DHT20 error code < 0
    if (data <= 0 || (*m_sensor).convert() != DHT20_OK) {
        // Invalid data, start new async request
        m_valid = false;
        (*m_sensor).requestData();
        return;
    }

    // Store valid retrieved data
    m_temperature = (*m_sensor).getTemperature();
    m_humidity = (*m_sensor).getHumidity();
    m_time_last_seen = now;
    m_valid = true;

    // Trigger next async data request
    (*m_sensor).requestData();
}

bool Dht20::is_connected()
{
    return m_valid;
}

uint32_t Dht20::last_seen()
{
    return m_time_last_seen;
}

float Dht20::temperature()
{
    /**
     * DHT20 temperature resolution is 0.01 °C, but accuraccy is
     * typical +-0.5 °C (max +-0.7 °C) between 10 and 55 °C
     * and up to +-1.7 °C between -40 and 80 °C.
     */
    return m_temperature;
}

float Dht20::relative_humidity()
{
    /**
     * DHT20 relative humidity resolution is 0.024 %, but accuracy is
     * typical +-3 % (max +-4 %) between 20 and 80 %
     * and up to +- 6 % between 0 and 100 %.
     */
    return m_humidity;
}

float Dht20::absolute_humidity()
{
    /**
     * Absolute humidity in g_water / m^3_moistAir
     *
     * = M~_water[g/mol] * RH[%] * p*_water(T)[Pa] / (R~[J/(mol*K)] * T[K])
     *
     * Approximation (Anotine equation-like) for factor on relative humidity in %
     * f(T/°C) = relHumidity[%] * 10 ^ (6.498 - 2214 / (T + 286.8)) [in g_water / m^3_moistAir]
     *
     * Examples:
     * 20°C, 50% RH -> 8.7 g/m^3
     * 50°C, 50% RH -> 41.4 g/m^3
     * 80°C, 20% RH -> 58.2 g/m^3
     * 80°C, 100% RH -> 290.9 g/m^3
     */

    if (m_temperature < -40 || m_temperature > 80) {
        return INVALID_FLOAT;
    }

    float absolute_humidity = m_humidity * pow(10, 6.498 - 2214 / (m_temperature + 286.8));

    return absolute_humidity;
}