#include "button_hal.h"

// Internal structures for dual button management
struct ButtonState {
    int pin;
    bool last_raw_state;
    bool last_stable_state;
    unsigned long last_debounce_time;
    unsigned long press_start_time;
    unsigned long last_release_time;
    bool is_pressed;
    int click_count;
};

static ButtonState left_btn =  { PIN_BTN_LEFT,  HIGH, HIGH, 0, 0, 0, false, 0 };
static ButtonState right_btn = { PIN_BTN_RIGHT, HIGH, HIGH, 0, 0, 0, false, 0 };
static bool double_click_active = false;

void button_hal_init() {
    pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
    pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);
    double_click_active = false; // Default to FAST mode
    if (Serial) Serial.println("Button HAL: Dual button initialized (Default: ZERO Latency) // [DEBUG]");
}

void button_hal_set_double_click(bool enabled) {
    double_click_active = enabled;
    // Reset counts when switching mode to avoid ghost clicks
    left_btn.click_count = 0;
    right_btn.click_count = 0;
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
                // Button Pressed
                btn.press_start_time = now;
                btn.is_pressed = true;
                btn.click_count++;
            } else {
                // Button Released
                if (btn.is_pressed) {
                    unsigned long duration = now - btn.press_start_time;
                    btn.is_pressed = false;
                    btn.last_release_time = now;

                    if (duration >= LONG_HOLD_MIN_MS) {
                        btn.click_count = 0; // Reset on hold
                        return is_left ? BTN_LEFT_HOLD : BTN_RIGHT_HOLD;
                    }

                    // If double-click is NOT enabled, return CLICK INSTANTLY on release
                    if (!double_click_active) {
                        btn.click_count = 0;
                        return is_left ? BTN_LEFT_CLICK : BTN_RIGHT_CLICK;
                    }
                }
            }
        }
    }

    // Double-click detection gap (ONLY enabled when double_click_active is true)
    if (double_click_active && !btn.is_pressed && btn.click_count > 0) {
        if (now - btn.last_release_time > 250) { // Gap for double click
            if (btn.click_count == 1) ev = is_left ? BTN_LEFT_CLICK : BTN_RIGHT_CLICK;
            else if (btn.click_count >= 2) ev = is_left ? BTN_NONE : BTN_RIGHT_DOUBLE; 
            btn.click_count = 0;
        }
    }

    btn.last_raw_state = raw;
    return ev;
}

ButtonEvent button_hal_read() {
    ButtonEvent ev_left = process_button(left_btn, true);
    if (ev_left != BTN_NONE) return ev_left;

    ButtonEvent ev_right = process_button(right_btn, false);
    return ev_right;
}
