#include "button_hal.h"

// Internal state management
static bool last_stable_state = HIGH; // HIGH = Released (Pullup)
static bool last_raw_state = HIGH;
static unsigned long last_debounce_time = 0;
static unsigned long press_start_time = 0;
static bool is_pressed = false;

void button_hal_init() {
    pinMode(PIN_BTN, INPUT_PULLUP);
    Serial.println("Button HAL: Initialized GPIO 7 (INPUT_PULLUP) // [DEBUG]");
}

ButtonEvent button_hal_read() {
    ButtonEvent event = BTN_NONE;
    bool raw_state = digitalRead(PIN_BTN);
    unsigned long now = millis();

    // Reset debounce timer if state changed
    if (raw_state != last_raw_state) {
        last_debounce_time = now;
    }

    // Process stable state
    if ((now - last_debounce_time) > DEBOUNCE_MS) {
        // State has been stable for long enough
        if (raw_state != last_stable_state) {
            last_stable_state = raw_state;

            if (last_stable_state == LOW) {
                // Button just pressed
                press_start_time = now;
                is_pressed = true;
                Serial.println("Button HAL: Pressed // [DEBUG]");
            } 
            else {
                // Button just released
                if (is_pressed) {
                    unsigned long duration = now - press_start_time;
                    is_pressed = false;
                    
                    if (duration >= LONG_HOLD_MIN_MS) {
                        event = BTN_LONG_HOLD;
                        Serial.printf("Button HAL: Long Hold Detected (%d ms) // [DEBUG]\n", duration);
                    } 
                    else if (duration < SHORT_CLICK_MAX_MS) {
                        event = BTN_SHORT_CLICK;
                        Serial.printf("Button HAL: Short Click Detected (%d ms) // [DEBUG]\n", duration);
                    }
                    else {
                        // Duration was in between Short and Long - Spec says return BTN_NONE
                        Serial.printf("Button HAL: Released, duration in deadzone (%d ms) // [DEBUG]\n", duration);
                    }
                }
            }
        }
    }

    last_raw_state = raw_state;
    return event;
}
