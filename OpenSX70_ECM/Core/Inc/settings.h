#ifndef SETTINGS_H
#define SETTINGS_H
#include "main.h"


//      Feature toggles
#define SHUTTERDARKSLIDE 0       //1 Enables feature to not eject dark slide until shutter button is pressed
#define LIGHMETER_HELPER 1       //1 Enables viewfinder light mstruct shutter_speed_timing{
#define EJECT_AFTER_DEPRESSING 1 //1 Enables the user to hold the shutter button to prevent photo ejection
#define FUZZY_MANUAL_MODE 0      //1 Enables alternate manual mode that varies manual speeds according to solenoid speed.

// ------------------DONGLELESS FEATURES (SONAR ONLY)---------------------
// Holding Sonar focus while closing the door will scroll through double exposure (purple), self timer (red), and t-mode (blue). Release on the mode you want
// If counter communication is installed will display "d', "t", "L" for the mode 
// if DONGLELESS_MANUAL_SPEEDS is enabled, holding the shutter button will scroll through manual speed zones (slow, medium, fast) and then holding a half press will scroll through the speeds in that zone. Release on the speed you want.
// Speeds are aranged as follows: slow (1s, 1/2s, 1/4s, 1/8s), medium (1/15s, 1/30s, 1/60s, 1/125s), fast (1/250s, 1/500s, 1/1000s, 1/2000s).
// if DONGLELESS_MANUAL_SPEEDS is enabled ISO swap is selected by half pushing the shutter (sonar focus) and then fully pushing the shutter down while its scrolling through the modes. LED will blink to indicate the ISO chosen (red SX70, blue 600). 

#define DONGLELESS_MANUAL_SPEEDS 0   //1 Enables manual speeds without dongle. 0 disables manual speeds without dongle.

//----------------DONGLE SWITCH FEATURE SELECTION-------------------------
// 1 and 2 values assign features to switch 1 and 2, 0 means unused.
// Example values:
// #define MEXP_MODE 1   : MEXP_MODE on switch 1
// #define SELF_TIMER 2  : SELF_TIMER on switch 2
// #define {whatever} 0  : No switch assigned. 
// DO NOT ASSIGN MULTIPLE THINGS TO THE SAME VALUE (except 0).
// DOING SO WILL BREAK THINGS. YOU CANNOT HAVE MULTIPLE FUNCTIONS ASSIGNED TO THE SAME SWITCH.
// When I have a configuration style dongle set up I will be doing validation on that side.
// Configuration dongle may be difficult due to how flash writing works on the stm32. Have to erase and write a whole flash page.
// Until then, YOU will need to validate that you are not overloading a switch.

#define MEXP_MODE 1
#define SELF_TIMER 2

//----------------END DONGLE SWITCH FEATURE SELECTION---------------------

//---------------MAGIC NUMBERS---------------------------------------------
#define A100 4093 //4093 normally less than max value as watchdog requires exceeding value to trigger
#define A600 1200 // (1200 original - 1500 for my sonar)
#define FD100 2809 //2809 normally
#define FF100 3652 //3652 normally
#define FD600 700
#define FF600 910
//---------------END MAGIC NUMBERS-----------------------------------------

//---------------Shutter Button--------------------------------------------
#define SHUTTER_BUTTON_DEBOUNCE_DELAY 5
#define SIMULTANEOUS_PRESS_DELAY 300
#define SIMULTANEOUS_PRESS_WINDOW 25
//---------------END Shutter Button----------------------------------------

//---------------Flashbar and Dongle Flash---------------------------------
#define Flash_Capture_Delay 4
//---------------End Flash settings----------------------------------------

//---------------METER SETTINGS--------------------------------------------
#define METER_OVERFLOW_THRESHOLD 2
//---------------END METER SETTINGS----------------------------------------

#define PERIPHERAL_TIMEOUT_MS 5

#define DEBOUNCE_DELAY 5

enum setting_type{
    MANUAL_SPEED,
    T_MODE,
    B_MODE,
    AUTO_MODE,
    AUTO_F_MODE
};

extern const uint8_t POWER_DOWN_DELAY;
extern const uint8_t Y_DELAY;

#if FUZZY_MANUAL_MODE
struct fuzzy_shutter_speed_timing{
    uint16_t min_prescaler;
    uint16_t min_period;
    uint16_t max_prescaler;
    uint16_t max_period;
    bool flash_enabled;
    enum setting_type type;
};
#endif
struct shutter_speed_timing{
    uint16_t prescaler;
    uint16_t period;
    bool flash_enabled;
    enum setting_type type;
};

extern struct shutter_speed_timing ShutterSpeedTiming[];
#if FUZZY_MANUAL_MODE
extern struct fuzzy_shutter_speed_timing FuzzyShutterSpeedTiming[];
#endif

#define FLASH_USER_DATA_ADDR  (0x08000000 + 32*1024 - 2048)


#endif
