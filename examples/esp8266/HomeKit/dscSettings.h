/*
 * dscKeybusInterfac-RTOS settings
 */

// Wifi setup
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// Configures the Keybus interface with the specified GPIO or NodeMCU/Wemos-style pins - dscWritePin is
// optional, leaving it out disables the virtual keypad.
#define dscClockPin D1  // GPIO: 5
#define dscReadPin D2   // GPIO: 4
#define dscWritePin D8  // GPIO: 15

