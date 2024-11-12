#pragma once

/**
 * Abstract button encoder interface, i.e. a combination of rotary encoder with
 * builtin button.
 */
class Encoder {
public:
    enum class Direction {
        None = 0,
        Clockwise = 1,
        CounterClockwise = -1,
    };

    /**
     * Update encoder state, ideally in an interrupt.
     */
    virtual void update() = 0;

    /**
     * Get current direction.
     */
    virtual Direction direction() = 0;
};

/**
 * Mock encoder implementation for testing.
 */
class MockEncoder : public Encoder {
public:
    void update() final {}
    Direction direction() final { return Direction::None; }
};
