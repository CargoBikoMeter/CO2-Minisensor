# CO2-Minisensor
A small device based on MH-Z19B NDIR CO2 sensor and Heltec CubeCell HTCC-AB02

![alt text](https://github.com/CargoBikoMeter/CO2-Minisensor/blob/master/images/CO2-Minisensor_Working-Mode.jpg)

## Table of contents
* [Overview](#overview)
* [Features](#features)
* [Development Setup](#development-setup)
* [Operation](#operation)

## Overview
CO2-Minisensor is an Arduino application in a little box, so you can carry this small device with you and measure the CO2 concentration in rooms where you are. 

## Features
 * Measures CO2 concentration every twenty minutes
 * Displays the measured CO2 concentration in three levels via the Neopixel LED or as numeric values on the OLED display
   * green:  <= 800 ppm
   * yellow: > 800 and <= 1400 ppm
   * red:    > 1400 ppm
 * Sleep-mode for low energy consumption
 * Integrated battery which can be charged by an external USB power supply or an attached small solar modul
 * Menu functions via on-board USER button
   * enable OLED display
   * sensor calibration
   * sleep mode deactivation
 * Data transmission via [The Things Network](https://thethingsnetwork.org) (TTN), a community-driven LoRaWAN network

## Development Setup
Device components:
 * MH-Z19B NDIR CO2 sensor 0-5000 ppm
 * Heltec CubeCell HTCC AB02 with integrated Neopixel LED, OLED display and LoRaWAN
 * StepUp-Modul Midi (SX-1308) DC/DC converter for powering the MH-Z19B sensor
 * AO 3415A P-Channel MOSFET (SMD) - power switch for StepUp-Modul in sleep-mode to save battery power
 * LP-503035 LiPo battery 500mAh
 * battery power switch SS-330
 * experimental board 4x6cm
 * cylinder head screw, slotted, M2, 8mm

Used Libraries:
 * MH-Z19 by Jonathan Dempsey
 * Arduino-based libraries for CubeCell devices provided by Heltec

Development environment:
 * PlatformIO IDE
 * Schematic: [KiCAD](https://github.com/CargoBikoMeter/CO2-Minisensor/blob/master/images/CO2-Minisensor--Schaltplan.svg)

How to configure the device:
In your local development environment rename the file device_config_template.h to device_config.h and copy/paste the requiered keys from TTN console into the file.

You must fill the following section with your private data for TTN device mode OTAA:
```
/* OTAA parameters*/
// device: YOUR-DEVICE_NAME
uint8_t devEui[] = { TODO };
uint8_t appEui[] = { TODO };
uint8_t appKey[] = { TODO };
```
If you want to use ABP mode, then fill the next section:
```
/* ABP para*/
uint8_t nwkSKey[] = { TODO };
uint8_t appSKey[] = { TODO };
uint32_t devAddr =  ( uint32_t )0xTODO;
```

With the global configuration file globals.h you can define the following parameter:

Enable/disable the LoRaWAN functionality:
```
#define USE_LORA  // default is on
```
Select the language for OLED display:
```
#define Language_Selector 0 // 0=DE ; 1=EN
```
Enable the OLED display as default after device startup:
```
#define _DISPLAY true     // false = Default to save energy
```

## Operation
First, open the plastic cap of the little box and move the MH-Z19B sensor gently outside the box. 

After uploading the image to the device with PlatformIO, the device initalizes, calls the setup function and then the main loop. If LoRaWAN is enabled, the device performs the TTN joining process. The start of the joining process will be signalled after powering the device by the Neopixel LED with a triple purple flashlight. During the joinig process no further information will be presented by the Neopixel LED.  

If you want to disable LoRaWAN functionality, press the "USER" button after the device was powered on for the first time or after a reset by the "RST" button and after a white flashlight appears. The LoRaWAN deactivation will be signalled with one yellow and a triple white flashlight. If LoRaWAN was disabled during compile time, no white flashlight will be shown during startup. 

After the TTN joining process has been completed successfully, the Step-Up module and the attached sensor will be switched on by the MOSFET. After the warm-up time (60 seconds and indicated by a permanent blue Neopixel LED) the CO2 measurement starts and, according to the measured value, the green, yellow or red Neopixel LED flashes. Alternatively, if the OLED display is enabled, the values will be displayed there. After first startup the OLED display is disabled by default to save battery power.

If LoRaWAN is enabled the device sends the first measured CO2 value and the battery voltage to TTN.

The beginning of each measurement cycle is indicated by a triple blue flashlight. After displaying the measurement, the program waits for ten seconds and starts a new cycle. 

To enable the OLED display, press the "USER" button (the left button on the CubeCell device in the picture above) as long as the blue Neopixel LED flashes. Then hold the "USER" button still pressed for 5 seconds, which will be signalled every second by a yellow flashligt. The OLED display activation will be acknowledged by a triple green flashlight.

If the OLED display is enabled, the following menu functions are available:
 * disable the sleep mode by pressing the "USER" button for 10 seconds
 * sensor calibration by pressing the "USER" button for 20 seconds

In normal operation, sleep mode will be activated 120 seconds after the first measurement. If the CO2 concentration is above 1400 ppm, the sleep mode will be temporarily disabled until the concentration is below 800 ppm and the last measurement was sent to TTN. During this period, the device sends the measured data via LoRaWAN every 300 seconds. 

The CO2-Minisensor wakes up again after 20 minutes.

If the battery voltage falls below 3.6 volt, the measurement stops and a warning message will be displayed on the OLED display. If the OLED display is currently not enabled, it will be enabled. If LoRaWAN is enabled, the device sends a CO2 measurement value of 10 ppm, which can be used as an indicator for a low battery level. The measurement starts again, when the battery voltage is above 3.6 volt even if the device is powered by an external USB power supply. 

If the device is powered by an external USB power supply and the battery switch is off, CO2 measurement is enabled, because the measured voltage is based on the USB power.  


Now have fun with the CO2-Minisensor!
