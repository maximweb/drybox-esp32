#pragma once

#include <Arduino.h>
#include <OneButton.h>

class Button {
public:
    enum class State {
        Idle = 0,
        Click = 1,
        DoubleClick = 2,
        LongPress = 3,
    };

    Button(uint8_t pin);

    State getState();

    void update();

    void reset();

private:
    void click_event();
    void doubleClick_event();
    void longPress_event();

    uint8_t m_pin;
    State m_state;
    OneButton m_button;
};