#pragma once

#include <Arduino.h>
#include <ESP32Encoder.h>

#include "button.h"
#include "encoder.h"

class Ky040 : public Encoder {
public:
    Ky040(uint8_t dt, uint8_t clk);

    void begin() final;

    void update() final;

    void reset() final;

    Encoder::Direction getDirection() final;
    Encoder::Direction peekDirection() final;

private:
    uint8_t m_pin_dt;
    uint8_t m_pin_clk;

    Encoder::Direction m_direction{Encoder::Direction::None};
    ESP32Encoder m_encoder;
    int64_t m_counter{0};
};
