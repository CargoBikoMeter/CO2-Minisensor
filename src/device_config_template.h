#include "globals.h"

/* OTAA parameters*/
// device: YOUR-DEVICE_NAME
uint8_t devEui[] = { TODO };
uint8_t appEui[] = { TODO };
uint8_t appKey[] = { TODO };

/* ABP para*/
uint8_t nwkSKey[] = { TODO };
uint8_t appSKey[] = { TODO };
uint32_t devAddr =  ( uint32_t )0xTODO;

#define _TX_INTERVAL   300000 // how often the data will be send via LoRaWAN in ms