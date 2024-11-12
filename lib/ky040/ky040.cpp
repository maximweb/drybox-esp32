#include "ky040.h"

Ky040::Ky040(uint8_t dt, uint8_t clk)
{
    // Initialize encoder
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    m_encoder.attachSingleEdge(digitalPinToGPIONumber(dt), digitalPinToGPIONumber(clk));
    m_encoder.setCount(0);
}

void Ky040::update()
{
    // Encoder
    const auto counter{m_encoder.getCount()};
    const auto counter_change{counter - m_counter};

    if (counter_change > 0) {
        m_counter = counter;
        m_direction = Encoder::Direction::Clockwise;
    }
    else if (counter_change < 0) {
        m_counter = counter;
        m_direction = Encoder::Direction::CounterClockwise;
    }
}

Encoder::Direction Ky040::direction()
{
    const auto tmp{m_direction};
    m_direction = Encoder::Direction::None;
    return tmp;
}
