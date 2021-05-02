#include "globals.h"
#include "lora.h"
#include "device_config.h"

// BatteryVoltage will be set in main program
extern uint BatteryVoltage;
extern uint CO2;

/*
 * set LoraWan_RGB to Active,the RGB active in loraWan
 * RGB red means sending;
 * RGB purple means joined done;
 * RGB blue means RxWindow1;
 * RGB yellow means RxWindow2;
 * RGB green means received done;
 */

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = LORAWAN_CLASS;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = _TX_INTERVAL;

/*OTAA or ABP*/
bool overTheAirActivation = LORAWAN_NETMODE;

/*ADR enable*/
bool loraWanAdr = LORAWAN_ADR;

/* set LORAWAN_Net_Reserve ON, the node could save the network info to flash, when node reset not need to join again */
bool keepNet = LORAWAN_NET_RESERVE;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = LORAWAN_UPLINKMODE;

/* Application port */
uint8_t appPort = LORA_APP_PORT;
/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 4;

/* Prepares the payload of the frame */
static void prepareTxFrame( uint8_t port )
{
	/*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
	*appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
	*if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
	*if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
	*for example, if use REGION_CN470, 
	*the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
	*/
    //int value1 = APP_DATA_VALUE1;
    appDataSize = LORA_APP_DATA_SIZE;
    appData[0] = BatteryVoltage;
    appData[1] = BatteryVoltage>>8;
    appData[2] = CO2;
    appData[3] = CO2>>8;
}

void lora_setup()
{
	Serial.println("\r\nDEBUG: lora_setup - begin");
    boardInitMcu();
	Serial.begin(115200);
    #if(AT_SUPPORT)
	enableAt();
    #endif
    //LoRaWAN.displayMcuInit();
	deviceState = DEVICE_STATE_INIT;
	LoRaWAN.ifskipjoin();
	Serial.println("DEBUG: lora_setup - end\r\n");
}

void lora_process()
{
	Serial.println("\r\nDEBUG: lora_process - begin");
    switch( deviceState )
	{
		case DEVICE_STATE_INIT:
		{
		  Serial.println("DEBUG: lora_process - DEVICE_STATE_INIT");
          #if(AT_SUPPORT)
			getDevParam();
          #endif
			printDevParam();
			LoRaWAN.init(loraWanClass,loraWanRegion);
			deviceState = DEVICE_STATE_JOIN;
			break;
		}
		case DEVICE_STATE_JOIN:
		{
			Serial.println("DEBUG: lora_process - DEVICE_STATE_JOIN");
            //LoRaWAN.displayJoining();
			LoRaWAN.join();
			break;
		}
		case DEVICE_STATE_SEND:
		{
			Serial.println("DEBUG: lora_process - DEVICE_STATE_SEND");
            //LoRaWAN.displaySending();
			prepareTxFrame( appPort );
			LoRaWAN.send();
			deviceState = DEVICE_STATE_CYCLE;
			break;
		}
		case DEVICE_STATE_CYCLE:
		{
			Serial.println("DEBUG: lora_process - DEVICE_STATE_CYCLE");
			// Schedule next packet transmission
			txDutyCycleTime = appTxDutyCycle + randr( 0, APP_TX_DUTYCYCLE_RND );
			LoRaWAN.cycle(txDutyCycleTime);
			//deviceState = DEVICE_STATE_SLEEP;
			break;
		}
		case DEVICE_STATE_SLEEP:
		{
			Serial.println("DEBUG: lora_process - DEVICE_STATE_SLEEP");
            LoRaWAN.displayAck();
			LoRaWAN.sleep();
			break;
		}
		default:
		{
			Serial.println("DEBUG: lora_process - default");
			deviceState = DEVICE_STATE_INIT;
			break;
		}
	}
	Serial.println("DEBUG: lora_process - end\r\n");
}