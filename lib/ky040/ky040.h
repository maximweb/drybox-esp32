#pragma once

#include <Arduino.h>
#include <ESP32Encoder.h>

#include "button.h"
#include "encoder.h"

class Ky040 : public Encoder {
public:
    Ky040(uint8_t dt, uint8_t clk);

    void update() final;

    Encoder::Direction direction() final;

private:
    Encoder::Direction m_direction{Encoder::Direction::None};
    ESP32Encoder m_encoder;
    int64_t m_counter{0};
};
