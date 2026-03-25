#pragma once

typedef enum {
    STATE_WATCHFACE,
    STATE_MENU_HR,
    STATE_EXEC_HR,
    STATE_MENU_AOD,
    STATE_MENU_SLEEP,
    STATE_DEEP_SLEEP_ENTRY // Transition state
} AppState;
