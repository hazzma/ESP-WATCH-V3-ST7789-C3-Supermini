#pragma once

typedef enum {
    STATE_WATCHFACE,
    
    STATE_MENU_HR,
    STATE_EXEC_HR,
    
    STATE_MENU_TIMEOUT,
    STATE_SET_TIMEOUT,
    
    STATE_MENU_AOD,
    
    STATE_MENU_TIMER,
    STATE_SET_TIMER_H,
    STATE_SET_TIMER_M,
    STATE_SET_TIMER_S,
    STATE_EXEC_TIMER,
    
    STATE_MENU_STOPWATCH,
    STATE_EXEC_STOPWATCH,
    
    STATE_MENU_BRIGHTNESS,
    STATE_SET_BRIGHTNESS,
    
    STATE_MENU_SYNC,        // New: Connectivity / Sync Menu
    STATE_SYNCING,          // New: Active Transfer Loading Screen
    
    STATE_MENU_SETTINGS,    // New: Settings main menu entry
    STATE_EXEC_SETTINGS,    // New: Inside settings sub-menu
    STATE_SET_AOD_CONFIG,   // New: AOD advanced config
    STATE_SET_AOD_BRIGHTNESS, // New: AOD brightness sub-editor
    STATE_SET_RAISE2WAKE,   // New: Raise to wake advanced config
    
    STATE_EXEC_STEPS
} AppState;
