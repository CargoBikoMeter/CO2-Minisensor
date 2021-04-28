#include "globals.h"
#include "CubeCell_NeoPixel.h"
#include "MHZ19.h"  
#include "SoftwareSerial.h"
#include "cubecell_SH1107Wire.h"
#include "lora.h"

// define user ui language for switch statements
enum eLanguage   // 0=DE ; 1=EN 
{
    DE,
    EN
};
enum eLanguage Language;
//Language_Selector for setting language in setup function is defined in globals.h

#ifdef USE_LORA
// define reference to LoRaWAN appData field, so we can modify field data here
extern uint8_t appData[LORAWAN_APP_DATA_MAX_SIZE];
#endif

// global variable for sending application data via LoRaWAN
uint BatteryVoltage = 0;
uint LowBatteryLevel = 3600; // don't measure below this level 
bool LowBattery = false;     // 

#define timetillsleep   120000 // milliseconds;  2 min:  120000
//#define timetillwakeup 1200000 // milliseconds; 20 min: 1200000  
#define timetillwakeup 120000 // milliseconds; 2 min: 120000 // for test only 
uint32_t TX_INTERVAL = 300000; // miliseconds;  5 min:  300000
uint32_t TX_LAST_SENT_TIME;
bool CO2_STATE_GOOD_SENT = false;
uint32_t currentSeconds;

static TimerEvent_t sleep;
static TimerEvent_t wakeUp;
uint8_t lowpower=0;
bool DEEP_SLEEP = false;      // with active DISPLAY deep sleep is not usefull
bool DEEP_SLEEP_DEACTIVATED = false; // could only be set to true in menu function

int INIT_MODE = 0; // 0 = not initialized, 1 = initialized

// OLED_Display definitions
bool DISPLAY = _DISPLAY; // defined in globals.h 

SH1107Wire  OLED_Display(0x3c, 500000, I2C_NUM_0,GEOMETRY_128_64,GPIO10); // addr , freq , i2c group , resolution , rst
CubeCell_NeoPixel pixels(1, RGB, NEO_GRB + NEO_KHZ800);

// Pin for switching Step-Up module HW-668 ON and OFF 
int VStepUpPin = GPIO14;

int LOOP_CYCLE = 10000;  // defines the loop cycle, in milliseconds

// pin for reading commands, commands depends from the time the button is pressed
// calibration command: button more then 10 seconds pressed 
#define buttonPin USER_KEY     // the number or symbol of the command button pin

int buttonState = HIGH;        // if USER_KEY not pressed: state is HIGH
int buttonread = 0;
int CMD_COUNTER = 0;                       // count the seconds the USER_KEY is pressed
                            
const int DISPLAY_ON_COUNTER = 5;          //  5: switch DISPLAY ON                               
const int DEEP_SLEEP_DISABLE_COUNTER = 10; // 10: disable DEEP_SLEEP
const int CALIBRATION_COUNTER = 20;        // 20: calibrate sensor

// CO2 Sensor definitions
#define BAUDRATE 9600       // MH-Z19 uses 9600 Baud
int WARMUP_TIME = 50000;    // time for sensor to warming up tp provide stable measurements

// any free GPIOs can be TX or RX
SoftwareSerial mySerial(GPIO11 /*RX AB02 - Tx MHZ19*/, GPIO12 /*TX AB02 - Rx MHZ19*/);
MHZ19 myMHZ19;     

// define CO2 levels
int CO2VeryGood            =   500;  // green high density
int CO2Good                =   800;  // green
int CO2Acceptable          =  1000;  // yellow
int CO2Bad                 =  1400;  // red
uint CO2                   =     0;  // will be sent by LoRaWAN


// subfunctions ******************************************************

void check_pushbutton() 
{
  Serial.println("begin of check_pushbutton function");
  // check if the USER_KEY is pressed, will change from HIGH to LOW
  buttonread = digitalRead(buttonPin);

  Serial.print("buttonread: ");
  Serial.println(buttonread);
  
  if (buttonread == LOW) { //check if button was pressed before and being pressed now
 
    if (buttonState == HIGH)
    {
      // switch on blue pixel for signaling command modus entered
      pixels.clear(); // Set all pixel colors to 'off' 
      pixels.setPixelColor(0, pixels.Color(0, 0, 25)); // blue
      pixels.show();   // Send the updated pixel colors to the hardware.
      buttonState = LOW;
      Serial.println("Button pressed");
      while ( buttonState != HIGH )
      {
        buttonState = digitalRead(buttonPin);
        Serial.println("Button still pressed");
        if ( DISPLAY)
        { 
          if (CMD_COUNTER <= DEEP_SLEEP_DISABLE_COUNTER && DEEP_SLEEP)
          {
            OLED_Display.clear();
            OLED_Display.setTextAlignment(TEXT_ALIGN_CENTER);
            OLED_Display.setFont(ArialMT_Plain_10);
            switch( Language )
            {
              case DE: 
              {
                OLED_Display.drawString(64, 0, "Deaktivierung");
                OLED_Display.drawString(64, 17, "Schlafmodus");
                OLED_Display.drawString(64, 29, "in "+ String(DEEP_SLEEP_DISABLE_COUNTER-CMD_COUNTER) + "Sekunden");
                break;
              }
              case EN: 
              {
                OLED_Display.drawString(64, 0, "Disabling");
                OLED_Display.drawString(64, 17, "sleep mode");
                OLED_Display.drawString(64, 29, "in "+ String(DEEP_SLEEP_DISABLE_COUNTER-CMD_COUNTER) + "seconds");
                break;
              }
            } 
            OLED_Display.setFont(ArialMT_Plain_16);
            OLED_Display.drawString(64, 42, String(CMD_COUNTER));
            OLED_Display.display();
          }
          if (CMD_COUNTER > DEEP_SLEEP_DISABLE_COUNTER)
          {
            OLED_Display.clear();
            OLED_Display.setTextAlignment(TEXT_ALIGN_CENTER);
            OLED_Display.setFont(ArialMT_Plain_10);
            switch( Language )
            {
              case DE: 
              {
                OLED_Display.drawString(64, 0, "Kalibrierung");
                OLED_Display.drawString(64, 17, "nur bei Frischluft");
                OLED_Display.drawString(64, 29, "in "+ String(CALIBRATION_COUNTER-CMD_COUNTER) + "Sekunden");
                break;
              }
              case EN: 
              {
                OLED_Display.drawString(64, 0, "Calibration");
                OLED_Display.drawString(64, 17, "only in fresh air");
                OLED_Display.drawString(64, 29, "in "+ String(CALIBRATION_COUNTER-CMD_COUNTER) + "seconds");
                break;
              }
            } 
            OLED_Display.setFont(ArialMT_Plain_16);
            OLED_Display.drawString(64, 42, String(CMD_COUNTER));
            OLED_Display.display();
          }
        }
        // signal the pressed button to the user
        // clear the pixel LED
        pixels.setPixelColor(0, pixels.Color(50, 20, 0)); // yellow
        pixels.show();   // Send the updated pixel colors to the hardware.
        delay(500);
        pixels.clear();
        pixels.show();   // Send the updated pixel colors to the hardware.
        delay(500);
        CMD_COUNTER++;

        switch ( CMD_COUNTER ) {
        case DISPLAY_ON_COUNTER:
          if ( ! DISPLAY ) 
          {
            // enable DISPLAY
            Serial.println("DISPLAY enabled");
            // flash geen pixel for signaling DISPLAY enabled
            for (int i=0; i<3; i++) {
              // clear the pixel LED
              pixels.setPixelColor(0, pixels.Color(0, 25, 0)); // green
              pixels.show();   // Send the updated pixel colors to the hardware.
              delay(500);
              pixels.clear();
              pixels.show();   // Send the updated pixel colors to the hardware.
              delay(500);
            }
            DISPLAY = true;
            OLED_Display.init();
            OLED_Display.clear();
            OLED_Display.setTextAlignment(TEXT_ALIGN_CENTER);
            OLED_Display.setFont(ArialMT_Plain_16);
            OLED_Display.drawString(64, 0, "Display");
            switch( Language )
            {
              case DE: 
              {
                OLED_Display.drawString(64, 20, "aktiviert");
                break;
              }
              case EN: 
              {
                OLED_Display.drawString(64, 20, "enabled");
                break;
              }
            } 
            OLED_Display.display();
            delay(5000);
            CMD_COUNTER=0; // reset counter
            buttonState = HIGH;
          }  
          break;

        case CALIBRATION_COUNTER: 
          myMHZ19.calibrate(); // start calibration
          if ( DISPLAY )
          { 
            OLED_Display.clear();
            OLED_Display.setTextAlignment(TEXT_ALIGN_CENTER);
            OLED_Display.setFont(ArialMT_Plain_16);
            switch( Language )
            {
              case DE: 
              {
                OLED_Display.drawString(64, 0, "Kalibrierung");
                OLED_Display.drawString(64, 20, "abgeschlossen");
                break;
              }
              case EN: 
              {
                OLED_Display.drawString(64, 0, "Calibration");
                OLED_Display.drawString(64, 20, "finnished");
                break;
              }
            } 
            OLED_Display.display();
          }
          delay(5000);
          CMD_COUNTER=0; // reset counter
          buttonState = HIGH;
          break;

        case DEEP_SLEEP_DISABLE_COUNTER:
          if ( DEEP_SLEEP )
          {
            // disable deep sleep
            DEEP_SLEEP_DEACTIVATED = true;
            Serial.println("DEEP SLEEP disabled");
            // flash geen pixel for signaling DISPLAY enabled
            for (int i=0; i<3; i++) {
              // clear the pixel LED
              pixels.setPixelColor(0, pixels.Color(0, 25, 0)); // green
              pixels.show();   // Send the updated pixel colors to the hardware.
              delay(500);
              pixels.clear();
              pixels.show();   // Send the updated pixel colors to the hardware.
              delay(500);
            }
            if ( DISPLAY )
            { 
              OLED_Display.clear();
              OLED_Display.setTextAlignment(TEXT_ALIGN_CENTER);
              OLED_Display.setFont(ArialMT_Plain_16);
              switch( Language )
              {
                case DE: 
                {
                  OLED_Display.drawString(64, 0, "Schlafmodus");
                  OLED_Display.drawString(64, 20, "deaktiviert");
                  break;
                }
                case EN: 
                {
                  OLED_Display.drawString(64, 0, "sleep mode");
                  OLED_Display.drawString(64, 20, "disabled");
                  break;
                }
              } 
              OLED_Display.display();
            }  
            delay(5000);
            CMD_COUNTER=0; // reset counter
            buttonState = HIGH;
          }
          break;

        default:
          break;
        }
      }  // while loop
      CMD_COUNTER=0; // reset counter
      buttonState = HIGH;
    }
  }
  else {
    if (buttonState == LOW) {
      buttonState = HIGH;
    }
  }

  Serial.println("end of check_pushbutton function");
}

void setSleepTimer() {
  // set SleepTimer to go into lowpower timetillsleep ms later;
  TimerSetValue( &sleep, timetillsleep ); 
  TimerStart( &sleep );
}  

void initializeSensor() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Heltec CubeCell HTCC-AB02 starts ...");
  Serial.println("SRC: CO2-Minisensor");
  Serial.println("Build: " + String(__DATE__) + " " + String(__TIME__));

  // switch Vext pin ON for Pixel-LED and OLED OLED_Display
  digitalWrite(Vext, LOW);
  // switch StepUp module ON - for MH-Z19 Step-Up module
  digitalWrite(VStepUpPin, LOW);
  delay(500);


  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear(); // Set all pixel colors to 'off'

  if ( DISPLAY )
  { 
    // initialize OLED_Display
    OLED_Display.init();
    OLED_Display.setFont(ArialMT_Plain_10);
    OLED_Display.setTextAlignment(TEXT_ALIGN_CENTER);
    OLED_Display.drawString(64, 0, "Adlerkiez-IoT");
    OLED_Display.drawString(64, 11, "CO2-Minisensor");

    switch( Language )
    {
      case DE: 
      {
        OLED_Display.drawString(64, 25, "Initialisierung");
        break;
      }
      case EN: 
      {
        OLED_Display.drawString(64, 25, "Initialization");
        break;
      }
    }
    OLED_Display.display();
  }
  // switch on blue pixel for signaling MHZ19 initialization
  pixels.setPixelColor(0, pixels.Color(0, 0, 25)); // blue
  pixels.show();   // Send the updated pixel colors to the hardware.

  Serial.println("Setup: Hardware Serial init for MHZ19 sensor");
  Serial.println("mySerial begin ..." );
	mySerial.begin(9600);
  delay(500);

  Serial.println("myMHZ19 begin ..." );
  myMHZ19.begin(mySerial);  // *Serial(Stream) reference must be passed to library begin().
  char mhz19_version[10];
  myMHZ19.getVersion(mhz19_version);
  Serial.printf("Setup: MHZ version: %s.\r\n",mhz19_version);

  Serial.println("Setup: sensor self-calibration OFF");
  myMHZ19.autoCalibration(false); 

  if ( DISPLAY )
  { 
    switch( Language )
    {
      case DE: 
      {
        OLED_Display.drawString(64, 37, "Selbstkalibrierung: AUS");
        OLED_Display.drawString(64, 49, "Sensor aufheizen "+String(WARMUP_TIME/1000)+" Sek.");
        break;
      }
      case EN: 
      {
        OLED_Display.drawString(64, 37, "Self-calibration: OFF");
        OLED_Display.drawString(64, 49, "Sensor heating "+String(WARMUP_TIME/1000)+" sec");
        break;
      }
    } 
    OLED_Display.display(); // send the display buffer data to the OLED display
  }

  delay(WARMUP_TIME); // wait 30 seconds until the sensor has warming up
  // sensor successfully initialized, set INIT_MODE to 1 to prevent  
  // initialization in main loop again
  INIT_MODE = 1;

  // set sleep timer
  setSleepTimer();
}

void onSleep()
{
  //TODO: we have to ensure that the loop should be restarted after this interrupt call returns to main program
  Serial.println("DEBUG: function entry: onSleep");

  #ifdef USE_LORA
  // reset the timer if LoRAWAN is not joined
  if ( deviceState != DEVICE_STATE_CYCLE ) {
    // set sleep timer
    setSleepTimer();
    Serial.println("INFO: set sleep timer, device currently not joined");
  }

  // sent data via LoRaWAN if device is joined and TX_INTERVAL is reached
  currentSeconds = (uint32_t) millis();
  // Serial.print("DEBUG: onSleep - last lora transmission ");
  // Serial.print((currentSeconds - TX_LAST_SENT_TIME)/1000);
  // Serial.println(" seconds ago");
  Serial.printf("DEBUG: onSleep - last lora tranmission was %d seconds ago.\r\n",((currentSeconds - TX_LAST_SENT_TIME)/1000));

  if ( (currentSeconds - TX_LAST_SENT_TIME ) >= TX_INTERVAL) {
  if ( deviceState == DEVICE_STATE_CYCLE ) {  
    deviceState = DEVICE_STATE_SEND;

    if ( DISPLAY )
    { 
      // clear the OLED_Display
      OLED_Display.clear();
      OLED_Display.setFont(ArialMT_Plain_16);
      OLED_Display.setTextAlignment(TEXT_ALIGN_CENTER);
      OLED_Display.drawString(64, 0, "CO2-Minisensor");
      switch( Language )
      {
        case DE: 
        {
          OLED_Display.drawString(64, 25, "sendet Daten");
          break;
        }
        case EN: 
        {
          OLED_Display.drawString(64, 25, "send data");
          break;
         }
      }     
      OLED_Display.drawString(64, 43, String(CO2) + " ppm");
      OLED_Display.display(); // send the display buffer data to the OLED display
      delay(5000);
    }
    Serial.println("INFO: onSleep - lora processing ...");
    lora_process();
    TX_LAST_SENT_TIME = (uint32_t) millis();

    // enter deep sleep mode only if the good CO2 state was sent by lora
    if ( DEEP_SLEEP ) {
     CO2_STATE_GOOD_SENT = true;
    }
  } 
  else {
    Serial.println("DEBUG: onSleep - lora processing not allowed yet ...");
    // Serial.print("DEBUG: onSleep -  next tranmission in: ");
    // Serial.print((TX_INTERVAL - TX_LAST_SENT_TIME)/1000);
    // Serial.println(" seconds");
    Serial.printf("DEBUG: onSleep - next lora tranmission in %d seconds.\r\n",((TX_INTERVAL - TX_LAST_SENT_TIME)/1000));
  }

  } 
  #endif
 
  // going into deep sleep only if CO2 value is not red AND LoRaWAN is already joined
  #ifdef USE_LORA
  // going into deep sleep only if CO2 value is not bad AND LoRaWAN is joined AND 
  // last CO2 state sent was good AND battery leel ist good
  if ( DEEP_SLEEP && (deviceState == DEVICE_STATE_CYCLE) && CO2_STATE_GOOD_SENT && ! (LowBattery && DEEP_SLEEP_DEACTIVATED) ) 
  #else
  if ( DEEP_SLEEP && ! DEEP_SLEEP_DEACTIVATED )
  #endif
  {
    Serial.printf("DEBUG: onSleep - Going into lowpower mode, %d ms later wake up.\r\n",timetillwakeup);
    lowpower=1;
    TimerSetValue( &wakeUp, timetillwakeup );
    TimerStart( &wakeUp );
    INIT_MODE = 0;

    if ( DISPLAY )
    { 
      // clear the OLED_Display
      OLED_Display.clear();
      OLED_Display.setFont(ArialMT_Plain_16);
      OLED_Display.setTextAlignment(TEXT_ALIGN_CENTER);
      OLED_Display.drawString(64, 0, "CO2-Minisensor");
      switch( Language )
      {
        case DE: 
        {
          OLED_Display.drawString(64, 25, "geht schlafen");
          break;
        }
        case EN: 
        {
          OLED_Display.drawString(64, 25, "goes to sleep now");
          break;
         }
      }     
      OLED_Display.display(); // send the display buffer data to the OLED display
      delay(5000);

      // clear the OLED_Display
      Serial.println("DEBUG: onSleep - clear OLED_Display");
      OLED_Display.clear();
      OLED_Display.display(); 
      //OLED_Display.displayOff();
    }

    // clear the pixel LED
    Serial.println("DEBUG: onSleep - clear pixel LED");
    pixels.clear();
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(500);

    Serial.println("DEBUG: onSleep - switch Vext pin OFF");
    // switch Vext pin OFF for Pixel-LED and OLED display
    digitalWrite(Vext, HIGH);
    
    // switch StepUp module OFF - for MH-Z19 Step-Up module
    Serial.println("DEBUG: onSleep - switch StepUp module OFF");
    digitalWrite(VStepUpPin, HIGH);
  }
  else {
    // set sleep timer again, so data in TX buffer could be sent via next call to onSleep
    Serial.println("DEBUG: onSleep - set sleep timer again");
    setSleepTimer();
  }
}

void onWakeUp()
{
  Serial.println("DEBUG: function entry: onWakeUp");
  Serial.printf("Woke up, going %d ms later into lowpower mode.\r\n",timetillsleep);
  lowpower=0;
  // set sleep timer
  setSleepTimer();

  // switch Vext pin ON for Pixel-LED and OLED display
  digitalWrite(Vext, LOW);
  delay(500);
  
  //reset status flags after wakeup
  CO2_STATE_GOOD_SENT = false;
}

void checkBattery()
{
  // get the current battery status
  BatteryVoltage = getBatteryVoltage();
  Serial.println("INFO: BatteryVoltage: " + String(BatteryVoltage) + " mV");
  if (BatteryVoltage <= LowBatteryLevel ) {
    LowBattery = true;
    // set CO2-Level to 10 to signal no CO2 measurement
    CO2 = 10;

    // enable OLED display to show the warning message on display
    DISPLAY = true;
    OLED_Display.init();
    delay(500);
    Serial.printf("WARN: Battery level below %d, to save battery measurement disabled now.\r\n",LowBatteryLevel);

      // clear the OLED_Display
      OLED_Display.clear();
      OLED_Display.setFont(ArialMT_Plain_16);
      OLED_Display.setTextAlignment(TEXT_ALIGN_CENTER);
      OLED_Display.drawString(64, 0, "CO2-Minisensor");
      OLED_Display.setFont(ArialMT_Plain_16);
      switch( Language )
      {
        case DE: 
        {
          OLED_Display.drawString(64, 25, "Batterie schwach");
          break;
        }
        case EN: 
        {
          OLED_Display.drawString(64, 25, "Battery low");
          break;
         }
      }     
      OLED_Display.drawString(64, 43, String(BatteryVoltage)+" mV");

      OLED_Display.display(); // send the display buffer data to the OLED display
      delay(30000);
  }
  else {
    LowBattery = false;
    Serial.printf("INFO: Battery level greather than %d , good\r\n",LowBatteryLevel);
  }
  //delay(1000);
}

void flashPixelLoRaJoin() {
    Serial.println("DEBUG: function flashPixelLoRaJoin called ...");
    // clear the pixel LED
    pixels.clear();
    pixels.show();   // Send the updated pixel colors to the hardware.

    // flash blue pixel for signaling we are now insight the main loop
    for (int i=0; i<3; i++) {
      // clear the pixel LED
      Serial.println("DEBUG: flashPixelLoRaJoin blink blue ...");
      pixels.setPixelColor(0, pixels.Color(0, 0, 25)); // blue
      pixels.show();   // Send the updated pixel colors to the hardware.
      delay(250);
      pixels.clear();
      pixels.show();   // Send the updated pixel colors to the hardware.
      delay(250);
    }
}    

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Setup called ...");

  // define UI language
  switch (Language_Selector)
  {
    case 0:
    {
      Language = DE;
      break;
    }
    case 1:
    {
      Language = EN;
      break;
    }
  }  

  // disable LoRaWAN radio if LoRaWAN should not be used
  #ifndef USE_LORA
  Radio.Sleep();
  #endif

  Serial.println("Heltec CubeCell HTCC-AB02 starts ...");

  pinMode(Vext,OUTPUT);
  // switch Vext pin ON for Pixel-LED and OLED OLED_Display
  digitalWrite(Vext, LOW);
  delay(500);

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

  if ( DISPLAY )
  { 
    Serial.println("INFO: setup - init OLED display ...");
    OLED_Display.init(); // needs 2 mA current in deep sleep
    delay(500);
  }

  #ifdef USE_LORA
  Serial.println("INFO: setup - initialize LoRaWAN module ...");
  flashPixelLoRaJoin();

  // get battery level, so we could send battery data after during LoRa join 
  checkBattery();

  // prepare LoRaWAN TX buffer
  appData[0] = BatteryVoltage;
  appData[1] = BatteryVoltage>>8;

  Serial.println("INFO: setup - call lora_setup() ...");
  lora_setup();
  #endif

  // initialize the pin for P-MOSFET switch for StepUpModul as output
  pinMode(VStepUpPin,OUTPUT);
  // initialize the pushbutton pin as an input
  pinMode(buttonPin, INPUT);

  TimerInit( &sleep, onSleep );
  TimerInit( &wakeUp, onWakeUp );
 
  // set sleep timer
  setSleepTimer();
}

void measure() {
    // get the current battery level
    checkBattery();

    Serial.print("DEBUG: measure - DEEP_SLEEP= ");
    Serial.println(DEEP_SLEEP);

    Serial.print("DEBUG: measure - INIT_MODE= ");
    Serial.println(INIT_MODE);
    switch( INIT_MODE )
    {
      case 0: 
      {
        Serial.println("case 0 switch");
        initializeSensor();
        break;
      }
    } 

    // clear the pixel LED
    pixels.clear();
    pixels.show();   // Send the updated pixel colors to the hardware.

    // flash blue pixel for signaling we are now insight the main loop
    for (int i=0; i<3; i++) {
      // clear the pixel LED
      pixels.setPixelColor(0, pixels.Color(0, 0, 25)); // blue
      pixels.show();   // Send the updated pixel colors to the hardware.
      delay(500);
      pixels.clear();
      pixels.show();   // Send the updated pixel colors to the hardware.
      delay(500);
    }

    // read the state of the pushbutton value:
    check_pushbutton();

    if ( DISPLAY && ! lowpower )
    { 
      // clear the OLED_Display
      OLED_Display.clear();
      OLED_Display.setFont(ArialMT_Plain_16);
      OLED_Display.setTextAlignment(TEXT_ALIGN_CENTER);
      OLED_Display.drawString(64, 0, "CO2-Minisensor");
      OLED_Display.setFont(ArialMT_Plain_16);
      switch( Language )
      {
        case DE: 
        {
          OLED_Display.drawString(64, 25, "Batterie");
          break;
        }
        case EN: 
        {
          OLED_Display.drawString(64, 25, "Battery");
          break;
         }
      }     
      OLED_Display.drawString(64, 43, String(BatteryVoltage)+" mV");

      OLED_Display.display(); // send the display buffer data to the OLED display
      delay(5000);

      OLED_Display.clear();
      OLED_Display.setFont(ArialMT_Plain_16);
      OLED_Display.setTextAlignment(TEXT_ALIGN_CENTER);
      OLED_Display.drawString(64, 0, "CO2-Minisensor");
      OLED_Display.display(); // send the display buffer data to the OLED display
    }  

    // turn the LED on (HIGH is the voltage level)
    Serial.println("DEBUG: measure - reading MHZ19 CO2 value ...");
   
    CO2 = myMHZ19.getCO2();    //CO2 in ppm messen
    Serial.print("DEBUG: measure - CO2 value: ");
    Serial.println(CO2);

    if ( DISPLAY )
    { 
      OLED_Display.setFont(ArialMT_Plain_16);
      OLED_Display.setTextAlignment(TEXT_ALIGN_CENTER);
      OLED_Display.drawString(64, 25, String(CO2) + " ppm");
    }

    if ( ( CO2 < 400 ) ) {
      pixels.setPixelColor(0, pixels.Color(0, 0, 25)); // blue

      // disable DEEP_SLEEP until CO2 is good
      Serial.println("DEBUG: CO2 <= 400 - Deep Sleep disabled");
      DEEP_SLEEP = false;

      if ( DISPLAY )
      { 
        switch( Language )
        {
          case DE: 
          {
            OLED_Display.drawString(64, 43, "Sensor prüfen!");
            break;
          }
          case EN: 
          {
            OLED_Display.drawString(64, 43, "Check sensor!");
            break;
          }
        } 
      }
    }    
    
    if ( ( CO2 >= 400 ) && ( CO2 <= CO2VeryGood ) ) {
      pixels.setPixelColor(0, pixels.Color(0, 100, 0)); // green high intensity
      
      if ( ! DEEP_SLEEP_DEACTIVATED ) {
        Serial.println("DEBUG: CO2 very good - Deep Sleep enabled");
        DEEP_SLEEP = true;
      }

      if ( DISPLAY )
      { 
        switch( Language )
        {
          case DE: 
          {
            OLED_Display.drawString(64, 43, "sehr gut");
            break;
          }
          case EN: 
          {
            OLED_Display.drawString(64, 43, "very good");
            break;
          }
        } 
      }
    }

    if ( ( CO2 > CO2VeryGood ) && ( CO2 <= CO2Good ) ) {
      pixels.setPixelColor(0, pixels.Color(0, 25, 0)); // green
      
      if ( ! DEEP_SLEEP_DEACTIVATED ) {
        Serial.println("DEBUG: CO2 good - Deep Sleep enabled");
        DEEP_SLEEP = true;
      }
      
      if ( DISPLAY )
      { 
        switch( Language )
        {
          case DE: 
          {
            OLED_Display.drawString(64, 43, "gut");
            break;
          }
          case EN: 
          {
            OLED_Display.drawString(64, 43, "good");
            break;
          }
        } 
      }
    }

    if ( ( CO2 > CO2Good ) && ( CO2 <= CO2Bad ) ) {
      pixels.setPixelColor(0, pixels.Color(50, 20, 0)); // yellow

      // disable DEEP_SLEEP until CO2 is good
      Serial.println("DEBUG: CO2 sufficient - Deep Sleep disabled");
      DEEP_SLEEP = false;

      if ( DISPLAY )
      { 
        switch( Language )
        {
          case DE: 
          {
            OLED_Display.drawString(64, 43, "ausreichend");
            break;
          }
          case EN: 
          {
            OLED_Display.drawString(64, 43, "sufficient");
            break;
          }
        } 
      }
    }
      
    if ( ( CO2 > CO2Bad ) ) {
      pixels.setPixelColor(0, pixels.Color(25, 0, 0)); // red

      // disable DEEP_SLEEP until CO2 is good
      Serial.println("DEBUG: CO2 bad - Deep Sleep disabled");
      DEEP_SLEEP = false;

      if ( DISPLAY )
      { 
        switch( Language )
        {
          case DE: 
          {
            OLED_Display.drawString(64, 43, "schlecht");
            break;
          }
          case EN: 
          {
            OLED_Display.drawString(64, 43, "bad");
            break;
          }
        } 
      }
    }
    
    pixels.show();   // Send the updated pixel colors to the hardware.
    
    if ( DISPLAY && ! lowpower )
    {
      OLED_Display.display(); // send the display buffer data to the OLED display
    }

    // wait some time to read the display
    Serial.println("DEBUG: measure - delay before next pass through main loop");
    delay(LOOP_CYCLE); // delay before next pass through main loop

    // clear the pixel LED
    pixels.clear();
    pixels.show();   // Send the updated pixel colors to the hardware.

    // now clear the display
    if ( DISPLAY )
    {
      Serial.println("DEBUG: measure - clear OLED display before leaving function");
      OLED_Display.clear();
      OLED_Display.display(); 
    }
}

void loop() {

  if( lowpower ) 
  {
    //note that lowPowerHandler() runs six times before the mcu goes into lowpower mode;
    Serial.println("loop: Enter low power modus");
    lowPowerHandler();
  }
  else {
    #ifdef USE_LORA
    // process main functions only if device is joined
    if ( deviceState == DEVICE_STATE_CYCLE ) {
    #endif
      // process main functions
      Serial.println("INFO: process main functions ...");
      if (! LowBattery) {
        measure();
      }
      else {
        // get the current battery level
        checkBattery();
      }
      
    #ifdef USE_LORA
    }
    #endif
  }
  #ifdef USE_LORA
  // process lora subfunctions
	lora_process();
  Serial.println("");
  Serial.println("INFO: deviceState= " + String(deviceState));
  // flash Pixel LED during joining 
  if ( deviceState != DEVICE_STATE_CYCLE) {
    Serial.println("INFO: joining ... ");
    //RR: no join with that line active: flashPixelLoRaJoin();
    //flashPixelLoRaJoin();
  }
  #endif
  delay(1000);
}