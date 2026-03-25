#include "button_hal.h"

// Internal structures for dual button management
struct ButtonState {
    int pin;
    bool last_raw_state;
    bool last_stable_state;
    unsigned long last_debounce_time;
    unsigned long press_start_time;
    bool is_pressed;
};

static ButtonState left_btn = { PIN_BTN_LEFT, HIGH, HIGH, 0, 0, false };
static ButtonState right_btn = { PIN_BTN_RIGHT, HIGH, HIGH, 0, 0, false };

void button_hal_init() {
    pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
    pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);
    Serial.println("Button HAL: Dual button initialized (GPIO 7 & 5) // [DEBUG]");
}

static ButtonEvent process_button(ButtonState& btn, bool is_left) {
    ButtonEvent ev = BTN_NONE;
    bool raw = digitalRead(btn.pin);
    unsigned long now = millis();

    if (raw != btn.last_raw_state) {
        btn.last_debounce_time = now;
    }

    if ((now - btn.last_debounce_time) > DEBOUNCE_MS) {
        if (raw != btn.last_stable_state) {
            btn.last_stable_state = raw;

            if (btn.last_stable_state == LOW) {
                btn.press_start_time = now;
                btn.is_pressed = true;
            } else {
                if (btn.is_pressed) {
                    unsigned long duration = now - btn.press_start_time;
                    btn.is_pressed = false;

                    if (duration >= LONG_HOLD_MIN_MS) {
                        ev = is_left ? BTN_LEFT_HOLD : BTN_RIGHT_HOLD;
                    } else if (duration < SHORT_CLICK_MAX_MS) {
                        ev = is_left ? BTN_LEFT_CLICK : BTN_RIGHT_CLICK;
                    }
                }
            }
        }
    }

    btn.last_raw_state = raw;
    return ev;
}

ButtonEvent button_hal_read() {
    // Check both buttons. Priority: Logic might favor one if both released same frame.
    ButtonEvent ev_left = process_button(left_btn, true);
    if (ev_left != BTN_NONE) return ev_left;

    ButtonEvent ev_right = process_button(right_btn, false);
    return ev_right;
}
