#pragma once

/**
 * Abstract button encoder interface, i.e. a combination of rotary encoder with
 * builtin button.
 */
class Encoder {
public:
    enum class Direction {
        None = 0,
        ClockwiseSlow = 1,
        Clockwise = 2,
        ClockwiseFast = 3,
        CounterClockwiseSlow = -1,
        CounterClockwise = -2,
        CounterClockwiseFast = -3,
    };

    virtual void begin() = 0;

    /**
     * Update encoder state, ideally in an interrupt.
     */
    virtual void update() = 0;

    virtual void reset() = 0;

    /**
     * Get current direction.
     */
    virtual Direction getDirection() = 0;

    virtual Direction peekDirection() = 0;
};

/**
 * Mock encoder implementation for testing.
 */
class MockEncoder : public Encoder {
public:
    void begin() final {}
    void update() final {}
    void reset() final {}
    Direction getDirection() final { return Direction::None; }
    Direction peekDirection() final { return Direction::None; }
};
