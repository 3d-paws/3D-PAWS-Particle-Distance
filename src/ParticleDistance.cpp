/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/Users/rjbubon/Documents/Particle/3D-PAWS-Particle-Distance/src/ParticleDistance.ino"
int seconds_to_next_obs();
void firmwareUpdateHandler(system_event_t event, int param);
void setup();
void loop();
#line 1 "/Users/rjbubon/Documents/Particle/3D-PAWS-Particle-Distance/src/ParticleDistance.ino"
PRODUCT_VERSION(1);
#define COPYRIGHT "Copyright [2024] [University Corporation for Atmospheric Research]"
#define VERSION_INFO "ParticleDS-20250305v1"

/*
 *======================================================================================================================
 * ParticleDistanceSensor - Used for Snow or Stream Gauge Monitoring
 *   Board Type : Particle BoronLTE or Argon
 *   Description: Distance sensing for Snow or Stream Gauge Monitoring
 *   Author: Robert Bubon
 *   Date:  2025-02-25 RJB Initial Development - Based on the Snow or Stream Gauge (SSG-ULP) code
 *          2025-02-25 RJB Added support for Always Connected and Low Power mode.
 *                         Changed distance pin to A4 from A3
 *                         Added Baseline distance support
 *                         Added support for various observation intervals
 *                         Added event type SEND and INFO
 *          2025-03-05 RJB Added support when in Low Power to stay connected/powered on, 
 *                         until next OBS period once a day. This is so we can send commands down
 *                         from Particle 
 *
 * NOTES:
 * When there is a successful transmission of an observation any need to send obersavations will be sent. 
 * On transmit a failure of these need to send observations, processing is stopped and the file is deleted.
 * 
 * Distance Sensor Calibration
 * Adding serial console jumper after boot will cause distance gauge to be read every 1 second and value printed.
 * Removing serial console jumper will resume normal operation
 * 
 * Requires Library
 *  SdFat                   by Bill Greiman
 *  RTCLibrary
 *  Adafruit_SSD1306_RK     I2C ADDRESS 0x3C
 *  Adafruit_BM(PE)280      I2C ADDRESS 0x77  (SD0 to GND = 0x76)
 *  adafruit-htu21df        I2C ADDRESS 0x40
 *  Adafruit_BMP3XX         I2C ADDRESS 0x77 and (SD0 to GND = 0x76)
 * 
 * System.batteryState()
 *  0 = BATTERY_STATE_UNKNOWN
 *  1 = BATTERY_STATE_NOT_CHARGING
 *  2 = BATTERY_STATE_CHARGING
 *  3 = BATTERY_STATE_CHARGED
 *  4 = BATTERY_STATE_DISCHARGING
 *  5 = BATTERY_STATE_FAULT
 *  6 = BATTERY_STATE_DISCONNECTED
 * 
 * Publish to Particle
 *  Event Name: SG
 *  Event Variables:
 *   at     timestamp
 *   ds     DS_Median
 *   bp1    bmx_pressure
 *   bt1    bmx_temp
 *   bh1    bmx_humid
 *   bp2    bmx_pressure
 *   bt2    bmx_temp
 *   bh2    bmx_humid
 *   hh1    htu_humid
 *   ht1    htu_temp
 *   bcs    Battery Charger Status
 *   bpc    Battery Percent Charge
 *   cfr    Charger Fault Register
 *   css    Cell Signal Strength
 *   hth    Health 16bits - See System Status Bits in below define statements
 * 
 * Particle Sleep
 * https://docs.particle.io/firmware/low-power/wake-publish-sleep-cellular/
 * 
 * AN002 Device Powerdown
 * https://support.particle.io/hc/en-us/articles/360044252554?input_string=how+to+handle+low+battery+and+recovery
 * 
 * NOTE: Compile Issues
 * If you have compile issues like multiple definations of functions then you need to clean the compile directory out
 *    ~/.particle/toolchains/deviceOS/2.0.1/build/target/user/...
 * 
 * ========================================================
 * Support for 3rd Party Sim 
 * ========================================================
 *   SEE https://support.particle.io/hc/en-us/articles/360039741113-Using-3rd-party-SIM-cards
 *   SEE https://docs.particle.io/cards/firmware/cellular/setcredentials/
 *   Logic
 *     Output how sim is configured (internal or external)
 *     If console is enabled and SD found and SIM.TXT exists at the top level of SD card
 *       Read 1st line from SIM.TXT. Parse line for one of the below patterns
 *        INTERNAL
 *        AUP epc.tmobile.com username passwd
 *        UP username password
 *        APN epc.tmobile.com
 *      Perform appropriate actions to set sim
 *      Rename file to SIMOLD.TXT, so we don't do this on next boot
 *      Output notification to user to reboot then flash board led forever
 *
 * ========================================================
 * Support for Argon WiFi Boards
 * ========================================================
 * At the top level of the SD card make a file called WIFI.TXT
 * Add one line to the file
 * This line has 3 items that are comma separated Example
 * 
 * AuthType,ssid,password
 * 
 * Where AuthType is one of these keywords (WEP WPA WPA2)
 *
 * ========================================================
 * Distance Sensor Type Setup.
 * ========================================================
 * Create SD Card File 5MDIST.TXT for %m sensor and 1.25 multiplier. No file for 10m Sensor and 2.5 multiplier
 * 
 * ======================================================================================================================
 */
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_HTU21DF.h>
#include <Adafruit_MCP9808.h>
#include <Adafruit_SI1145.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_VEML7700.h>
#include <RTClib.h>
#include <SdFat.h>

/*
 * ======================================================================================================================
 *  Timers
 * ======================================================================================================================
 */
#define DELAY_NO_RTC              1000*60    // Loop delay when we have no valided RTC
#define CLOUD_CONNECTION_TIMEOUT  90         // Wait for N seconds to connect to the Cell Network

/*
 * ======================================================================================================================
 *  Relay Power Control Pin
 * ======================================================================================================================
 */
#define REBOOT_PIN            A0  // Trigger Watchdog or external relay to cycle power
#define HEARTBEAT_PIN         A1  // Watchdog Heartbeat Keep Alive

/*
 * ======================================================================================================================
 * System Status Bits used for report health of systems - 0 = OK
 * 
 * OFF =   SSB &= ~SSB_PWRON
 * ON =    SSB |= SSB_PWROFF
 * 
 * ======================================================================================================================
 */
#define SSB_PWRON            0x1   // Set at power on, but cleared after first observation
#define SSB_SD               0x2   // Set if SD missing at boot or other SD related issues
#define SSB_RTC              0x4   // Set if RTC missing at boot
#define SSB_OLED             0x8   // Set if OLED missing at boot, but cleared after first observation
#define SSB_N2S             0x10   // Set if Need to Send observation file exists
#define SSB_FROM_N2S        0x20   // Set if observation is from the Need to Send file
#define SSB_AS5600          0x40   // Set if wind direction sensor AS5600 has issues - NOT USED with Distance Gauge                   
#define SSB_BMX_1           0x80   // Set if Barometric Pressure & Altitude Sensor missing
#define SSB_BMX_2          0x100   // Set if Barometric Pressure & Altitude Sensor missing
#define SSB_HTU21DF        0x200   // Set if Humidity & Temp Sensor missing
#define SSB_SI1145         0x400   // Set if UV index & IR & Visible Sensor missing
#define SSB_MCP_1          0x800   // Set if Precision I2C Temperature Sensor missing
#define SSB_MCP_2         0x1000   // Set if Precision I2C Temperature Sensor missing
#define SSB_SHT_1         0x4000   // Set if SHTX1 Sensor missing
#define SSB_SHT_2         0x8000   // Set if SHTX2 Sensor missing
#define SSB_HIH8         0x10000   // Set if HIH8000 Sensor missing
#define SSB_LUX          0x20000   // Set if VEML7700 Sensor missing
#define SSB_PM25AQI      0x40000   // Set if PM25AQI Sensor missing

unsigned int SystemStatusBits = SSB_PWRON; // Set bit 0 for initial value power on. Bit 0 is cleared after first obs
bool JustPoweredOn = true;         // Used to clear SystemStatusBits set during power on device discovery

/*
 * ======================================================================================================================
 *  Globals
 * ======================================================================================================================
 */
#define MAX_MSGBUF_SIZE 1024
char msgbuf[MAX_MSGBUF_SIZE]; // Used to hold messages
char *msgp;                   // Pointer to message text
char Buffer32Bytes[32];       // General storage

int  LED_PIN = D7;            // Built in LED
bool PostedResults;           // How we did in posting Observation and Need to Send Observations

uint64_t lastOBS = 0;         // time of next observation
int countdown = 120;          // Exit station monitor when reaches 0 - protects against burnt out pin or forgotten jumper

uint64_t NetworkReady = 0;
uint64_t LastTimeUpdate = 0;
uint64_t StartedConnecting = 0;
bool ParticleConnecting = false; 

time32_t Time_of_next_obs = 0;

int  cf_reboot_countdown_timer = 79200; // There is overhead transmitting data so take off 2 hours from 86400s
                                        // Set to 0 to disable feature
int DailyRebootCountDownTimer;

bool SendSystemInformation = true; // Send System Information to Particle Cloud. True means we will send at boot.

bool firmwareUpdateInProgress = false;

#if PLATFORM_ID == PLATFORM_BORON
/*
 * ======================================================================================================================
 *  Power Management IC (bq24195)
 * ======================================================================================================================
 */
PMIC pmic;
#endif

/*
 * ======================================================================================================================
 * International Mobile Subscriber Identity
 * ======================================================================================================================
 */
char imsi[16] = "";
bool imsi_valid = false;
time32_t imsi_next_try = 0;

/*
 * ======================================================================================================================
 *  SD Card
 * ======================================================================================================================
 */
#define SD_ChipSelect D5                // GPIO 10 is Pin 10 on Feather and D5 on Particle Boron Board
SdFat SD;                               // File system object.
File SD_fp;
char SD_obsdir[] = "/OBS";              // Store our obs in this directory. At Power on, it is created if does not exist
bool SD_exists = false;                     // Set to true if SD card found at boot
char SD_n2s_file[] = "N2SOBS.TXT";          // Need To Send Observation file
uint32_t SD_n2s_max_filesz = 200 * 8 * 24;  // Keep a little over 2 days. When it fills, it is deleted and we start over.
uint32_t SD_N2S_POSITION = 0;               // Position in the file past observations that have been sent.  

char SD_sim_file[] = "SIM.TXT";         // File used to set Ineternal or External sim configuration
char SD_simold_file[] = "SIMOLD.TXT";   // SIM.TXT renamed to this after sim configuration set

char SD_wifi_file[] = "WIFI.TXT";       // File used to set WiFi configuration

char SD_INFO_FILE[] = "INFO.TXT";       // Store INFO information in this file. Every INFO call will overwrite content


/*
 * ======================================================================================================================
 *  MainLoop code defines
 * ======================================================================================================================
 */
char SD_MAIN_LOOP_FILE[] = "MAINLOOP.TXT";    // File contents is one line. 
                                              //   LP,# - Low Power, Minute Interval on the hour for observation (15,20,30)
                                              //   AC,# - Always Connected, Minute Interval on the hour for observation
                                              //                            (1-6,10,12,15,20,30)

#define MLLP 0          // Main Loop Low Power Mode
#define MLAC 1          // Main Loop Always Connected
const char *ml_modes[] = {"LP", "AC"};
int MLMode = MLAC;
int MLOBS_Interval = 5;

/*
 * ======================================================================================================================
 *  LowPower StayOn Hour - Top of the UTC hour, low power mode will not disconnect but stay on until next obs interval.
 * ======================================================================================================================
 */
char SD_LPSOHR_FILE[] = "LPSOHR.TXT";    // File contents is one line. A UTC hour value from 0-23
int LowPowerStayOnHour = -1;             // This is in UTC time. So if you want 10am MST and (MST) is UTC-7.
                                         // Then use a value of 17 aka 5PM UTC. 
                                         // A -1 disables this feature

/*
 * =======================================================================================================================
 *  Distance Gauge
 * =======================================================================================================================
 */
#define DISTANCEGAUGE   A4
#define DS_BUCKETS      60
char SD_5M_DIST_FILE[] = "5MDIST.TXT";  // If file exists use adjustment of 1.25. No file, then 10m Sensor is 2.5
char SD_BL_DIST_FILE[] = "BLDIST.TXT";     // File used to set distance sensor baseline  aka distance = baseline - reading
float ds_adjustment = 2.5;              // Default sensor is 10m 
unsigned int ds_baseline = 0;           // if not 0 then we subtract the distance read from this value. Give us height.
unsigned int ds_bucket = 0;             // Distance Buckets
unsigned int ds_buckets[DS_BUCKETS];

/*
 * ======================================================================================================================
 *  Local Code Includes - Do not change the order of the below 
 * ======================================================================================================================
 */
#include "QC.h"                   // Quality Control Min and Max Sensor Values on Surface of the Earth
#include "SF.h"                   // Support Functions
#include "OP.h"                   // OutPut support for OLED and Serial Console
#include "CF.h"                   // Configuration File Variables
#include "TM.h"                   // Time Management
#include "Sensors.h"              // I2C Based Sensors
#include "DIST.h"                 // Distance Suupport
#include "EP.h"                   // EEPROM
#include "SDC.h"                  // SD Card
#include "OBS.h"                  // Do Observation Processing
#include "SM.h"                   // Station Monitor
#include "PS.h"                   // Particle Support Functions
#include "INFO.h"                 // Station Fonformation
#include "ML.h"                   // MainLoop Control and Interval setup
#include "MLLP.h"                 // MainLoop code for Low Power Mode
#include "MLAC.h"                 // MainLoop code for Always Connected Mode

/* 
 *=======================================================================================================================
 * seconds_to_next_obs() - minutes to next observations window
 * 
 * Example for 15m obs interval
 * Time of last obs in ms MOD 900 = Seconds since last period
 * 900 - Seconds since last period = Seconds to next period
 * Seconds to next period + current time in MS = next OBS time
 *=======================================================================================================================
 */
int seconds_to_next_obs() {
  return ((MLOBS_Interval*60) - (Time.now() % (MLOBS_Interval*60)));
}


/*
 * ======================================================================================================================
 * firmwareUpdateHandler() - The firmware update handler sets a global variable when the firmware update starts, 
 *                           so we can defer sleep until it completes (or times out)
 * 
 * SEE https://docs.particle.io/reference/device-os/api/system-events/system-events-reference/#cloud_status-64-parameter-values
 * ======================================================================================================================
 */
void firmwareUpdateHandler(system_event_t event, int param) {
    switch(param) {
        case 0:  // firmware_update_begin:
            firmwareUpdateInProgress = true;
            break;
        case 1:  // firmware_update_complete:
        case -1: // firmware_update_failed:
            firmwareUpdateInProgress = false;
            break;
    }
}

// You must use SEMI_AUTOMATIC or MANUAL mode so the battery is properly reconnected on
// power-up. If you use AUTOMATIC, you may be unable to connect to the cloud, especially
// on a 2G/3G device without the battery.
SYSTEM_MODE(SEMI_AUTOMATIC);

// https://docs.particle.io/cards/firmware/system-thread/system-threading-behavior/
SYSTEM_THREAD(ENABLED);

/*
 * ======================================================================================================================
 * setup() - runs once, when the device is first turned on.
 * ======================================================================================================================
 */
void setup() {
    // The device has booted, reconnect the battery.
#if PLATFORM_ID == PLATFORM_BORON
	pmic.enableBATFET();
#endif

  // Set Default Time Format
  Time.setFormat(TIME_FORMAT_ISO8601_FULL);

  // Put initialization like pinMode and begin functions here.
  pinMode (LED_PIN, OUTPUT);
  Output_Initialize();
  delay(2000); // Prevents usb driver crash on startup, Arduino needed this so keeping for Particle

  // Set up Distance gauge pin for reading 
  pinMode(DISTANCEGAUGE, INPUT);

  Serial_write(COPYRIGHT);
  Output (VERSION_INFO); // Doing it one more time for the OLED
  delay(4000);

  // The System.on() function is used to subscribe to system-level events and configure 
  // how the device should behave when these events occur.
  // Firmware update handler sets a global variable when the firmware update starts.
  // We do not want to go into low power mode during this update
  // System.on(firmware_update, firmwareUpdateHandler);

   // Set Daily Reboot Timer
  DailyRebootCountDownTimer = cf_reboot_countdown_timer;  

  // Initialize SD card if we have one.
  SD_initialize();

  // Report if we have Need to Send Observations
  if (SD_exists && SD.exists(SD_n2s_file)) {
    SystemStatusBits |= SSB_N2S; // Turn on Bit
    Output("N2S:Exists");
  }
  else {
    SystemStatusBits &= ~SSB_N2S; // Turn Off Bit
    Output("N2S:NF");
  }

  // Display EEPROM Information 
  EEPROM_Dump();

  // Check if correct time has been maintained by RTC
  // Uninitialized clock would be 2000-01-00T00:00:00
  stc_timestamp();
  sprintf (msgbuf, "%s+", timestamp);
  Output(msgbuf);

  // Read RTC and set system clock if RTC clock valid
  rtc_initialize();

  if (Time.isValid()) {
    Output("STC: Valid");
  }
  else {
    Output("STC: Not Valid");
  }

  stc_timestamp();
  sprintf (msgbuf, "%s=", timestamp);
  Output(msgbuf);

  DS_Initialize();

  // Adafruit i2c Sensors
  bmx_initialize();
  htu21d_initialize();
  mcp9808_initialize();
  sht_initialize();
  hih8_initialize();
  si1145_initialize();
  lux_initialize();

  if (SD.exists(SD_5M_DIST_FILE)) {
    ds_adjustment = 1.25;
    Output ("DIST=5M");
  }
  else {
    ds_adjustment = 2.5;
    Output ("DIST=10M");
  }

  MainLoopInitialize(); // Get Main Loop Control and Interval information

// !!! Remove below for production, added for testing
//  if (MLMode == MLLP) {
    GetLPStayOnHour(); // Get Low Power Stay on Hour 
//  }

#if PLATFORM_ID == PLATFORM_ARGON
  //==================================================
  // Check if we need to program for WiFi change
  //==================================================
  WiFiChangeCheck();
#else
  //==================================================
  // Check if we need to program for Sim change
  //==================================================
  SimChangeCheck();
#endif

  // Graceful Disconnect: By setting .graceful(true), the device ensures that any pending events or messages are sent 
  //                      to the cloud before disconnecting. This is important for maintaining data integrity and 
  //                      ensuring that the cloud is notified of the disconnect.
  // Timeout: The .timeout(5s) parameter specifies that the device should wait up to 5 seconds for the pending messages
  //          to be acknowledged by the cloud before proceeding with the disconnection. If the timeout is reached, the 
  //          disconnection will occur regardless of whether all messages have been acknowledged.
  Particle.setDisconnectOptions(CloudDisconnectOptions().graceful(true).timeout(5s));

  if (MLMode == MLLP) {
    // Low Power Mode
    // We Handle and maintain the connection since we shutdown the connection and go to sleep
    NetworkConnect(); // This function calls Particle.connect()
  }
  else {
    // Always Connected Mode

    // Connect the device to Particle Cloud. 
    //   This will automatically activate the cellular connection and attempt to connect to the Particle cloud if the 
    //   device is not already connected to the cloud. Upon connection to cloud, time is synced, aka Particle.syncTime()

    // Blocking Behavior: In most cases, calling Particle.connect() will block the main loop while the device attempts 
    //                    to connect to the Particle Cloud. This means that if the device is not truly connected to the 
    //                    cellular network, the execution of your code will halt at that point until it either successfully 
    //                    connects or fails after a timeout period
    Particle.connect();
  
    // Setup Remote Function to DoAction, Expects a parameter to be passed from Particle to control what action
    if (Particle.function("DoAction", Function_DoAction)) {
     Output ("DoAction:OK");
    }
    else {
      Output ("DoAction:ERR");
    }    
  }
}

/*
 * ======================================================================================================================
 * loop() runs over and over again, as quickly as it can execute.
 * ======================================================================================================================
 */
void loop() {
  if (MLMode == MLLP) {
    MainLoopLP(); // Low Power Mode
  }
  else {
    MainLoopAC(); // Always Connected Mode
  }
} // loop
