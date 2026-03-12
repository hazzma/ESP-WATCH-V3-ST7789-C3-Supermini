#include "button_hal.h"

static unsigned long pressStartTime = 0;
static bool last_btn_state = HIGH; // Active LOW assumes HIGH as idle
static bool long_hold_triggered = false;

void button_init() {
    pinMode(BTN_PIN, INPUT_PULLUP); // Active LOW uses internal pull-up
}

ButtonEvent button_update() {
    bool current_btn_state = digitalRead(BTN_PIN);
    ButtonEvent event = BTN_NONE;
    unsigned long now = millis();

    // Button Pressed (Active LOW)
    if (current_btn_state == LOW && last_btn_state == HIGH) {
        pressStartTime = now;
        long_hold_triggered = false;
        Serial.println("[BTN] Pressed (LOW)");
    }

    // Identify Long Hold while still pressed
    if (current_btn_state == LOW) {
        if (!long_hold_triggered && (now - pressStartTime >= 1500)) {
            long_hold_triggered = true;
            Serial.println("[BTN] Hold threshold reached (1.5s)");
        }
    }

    // Button Released (HIGH)
    if (current_btn_state == HIGH && last_btn_state == LOW) {
        unsigned long duration = now - pressStartTime;
        Serial.printf("[BTN] Released after %lu ms\n", duration);
        
        if (duration >= 1500) {
            event = BTN_LONG_HOLD;
            Serial.println("[BTN] Long Hold Event Triggered on Release");
        } else if (duration < 400) {
            event = BTN_SHORT_CLICK;
            Serial.println("[BTN] Short Click Event Triggered");
        }
        long_hold_triggered = false;
    }

    last_btn_state = current_btn_state;
    return event;
}

bool button_is_pressed() {
    return digitalRead(BTN_PIN) == LOW; // Active LOW
}
