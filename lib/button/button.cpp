#include "button.h"

Button::Button(uint8_t pin)
: m_pin{pin}
, m_button(pin, true, true)
{
}

void Button::begin()
{
    m_button.setClickMs(150);
    m_button.setPressMs(1000);
    m_button.setDebounceMs(30);
    m_button.attachClick([](void* scope) { ((Button*) scope)->click_event(); }, this);
    m_button.attachDoubleClick([](void* scope) { ((Button*) scope)->doubleClick_event(); }, this);
    m_button.attachLongPressStart([](void* scope) { ((Button*) scope)->longPress_event(); }, this);
}

Button::State Button::getState()
{
    const auto tmp{m_state};
    reset();
    return tmp;
}

Button::State Button::peekState()
{
    return m_state;
}

void Button::update()
{
    m_button.tick();
}

void Button::reset()
{
    m_state = State::Idle;
}

void Button::click_event()
{
    m_state = State::Click;
}

void Button::doubleClick_event()
{
    m_state = State::DoubleClick;
}

void Button::longPress_event()
{
    m_state = State::LongPress;
}
