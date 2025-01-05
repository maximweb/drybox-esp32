#include "ky040.h"

Ky040::Ky040(uint8_t dt, uint8_t clk)
: m_pin_dt{dt}
, m_pin_clk{clk}
{
}

void Ky040::begin()
{
    // Initialize encoder
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    m_encoder.attachSingleEdge(digitalPinToGPIONumber(m_pin_dt), digitalPinToGPIONumber(m_pin_clk));
    m_encoder.setCount(0);
}

void Ky040::update()
{
    // Encoder
    const auto counter{m_encoder.getCount()};
    const auto counter_change{counter - m_counter};

    if (counter_change > 0 && counter_change <= 3) {
        reset();
        m_direction = Encoder::Direction::ClockwiseSlow;
    }
    else if (counter_change > 3 && counter_change <= 7) {
        reset();
        m_direction = Encoder::Direction::Clockwise;
    }
    else if (counter_change > 7) {
        reset();
        m_direction = Encoder::Direction::ClockwiseFast;
    }
    else if (counter_change < 0 && counter_change >= -3) {
        reset();
        m_direction = Encoder::Direction::CounterClockwiseSlow;
    }
    else if (counter_change < -3 && counter_change >= -7) {
        reset();
        m_direction = Encoder::Direction::CounterClockwise;
    }
    else if (counter_change < -7) {
        reset();
        m_direction = Encoder::Direction::CounterClockwiseFast;
    }
}

void Ky040::reset()
{
    m_encoder.setCount(0);
    m_counter = 0;
    m_direction = Encoder::Direction::None;
}

Encoder::Direction Ky040::getDirection()
{
    const auto tmp{m_direction};
    reset();
    return tmp;
}

Encoder::Direction Ky040::peekDirection()
{
    return m_direction;
}
