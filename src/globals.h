#ifndef APP_GLOBALS_H
#define APP_GLOBALS_H

#include <Arduino.h>

// enable or disable LoRaWAN functions calls in main program code
#define USE_LORA

// we must include LoRa headers because battery function is defined in LoRa
#include "LoRaWan_APP.h"
#include "lora.h"

// define user ui language for switch statements
#define Language_Selector 0 // 0=DE ; 1=EN 

// define if OLED display is enabled as default
#define _DISPLAY true     // false = Default to save energy

#endif