/*
 * ======================================================================================================================
 *  MLULP.h - MainLoop code for Ultra Low Power
 * ======================================================================================================================
 */

 /*
 * ======================================================================================================================
 * GetLPStayOnHour() - Check SD card for file, if exists read stay on hour from file
 * ======================================================================================================================
 */
void GetLPStayOnHour() {
  File fp;
  int i=0;
  int soh=0;
  char ch, buf[32]; 

  Output("ML_Init");
  
  // Get Main Loop Control and Interval information from SD card file
  if (SD_exists && SD.exists(SD_LPSOHR_FILE)) {
    // Read first line out of file for value 0-23

    // if error, log message go with defaults
    fp = SD.open(SD_LPSOHR_FILE, FILE_READ); // Open the file for reading, starting at the beginning of the file.

    if (fp) {
      // Deal with invalid file size for format XX,## with possible CR and/or LF
      if (fp.size()>=1 && fp.size()<=4) {
        Output ("GSOH:File Open");
        // Read one line from file
        while (fp.available() && (i <=4 )) {
          ch = fp.read();

          // sprintf (msgbuf, "%02X : %c", ch, ch);
          // Output (msgbuf);

          if ((ch == 0x0A) || (ch == 0x0D) ) {  // newline or linefeed
            break;
          }
          else {
            buf[i++] = ch;
            buf[i] = 0; // make it a string
          }
        }
        fp.close();

        if ((strlen(buf)>0) && (strlen(buf)<=2)) {
          if (isnumeric(buf)) {
            soh = atoi (buf); // Get Stay On Hour
            if ((soh>=0) && (soh<=23)) {
              LowPowerStayOnHour = soh;
            }
            else {
              // Error Invalid hour
              sprintf (Buffer32Bytes, "GSOH:HR[%d] Err", soh);
              Output(Buffer32Bytes);              
            }
          }
          else {
            // Error No Numeric
            sprintf (Buffer32Bytes, "GSOH:HR[%s] Not Numeric", buf);
            Output(Buffer32Bytes);            
          }
        }
        else {
            // Error No Numeric
            sprintf (Buffer32Bytes, "GSOH:HR[%s] Not Numeric", buf);
            Output(Buffer32Bytes); 
        } 
      }
      else {
        fp.close();
        Output ("GSOH:Invalid SZ");       
      }
    }
    else {
      sprintf (Buffer32Bytes, "GSOH:Open[%s] Err", SD_LPSOHR_FILE);          
      Output(Buffer32Bytes);
    }
  }
  else {
    // Going with the defaults MLMode= MLAC
    Output ("GSOH:NF");   
  }
  sprintf (Buffer32Bytes, "GSOH:StayOnHour=%d", LowPowerStayOnHour);
  Output (Buffer32Bytes);
}

 /*
  * ======================================================================================================================
  * DoLowPowerStayOn() - Are we with in the Low Power Stay On Window 
  * ======================================================================================================================
  */
bool DoLowPowerStayOn() {
  if ((LowPowerStayOnHour>=0) && (LowPowerStayOnHour<=23)) { // StayOn hour is set and valid.
    if (Time.hour() == LowPowerStayOnHour) { // Are we in the Stay On hour
      if (Time.minute() < MLOBS_Interval) { // Are we in the first observation window after the hour
        return (true);
      }
    }
  }
  return (false);
}

 /*
  * ======================================================================================================================
  * NetworkDisconnect() - Disconnect from Particle and Turn off modem   
  * ======================================================================================================================
  */
void NetworkDisconnect() {
    if (Particle.connected()) {
      Output ("Disconnect:PC");  
    }
    else {
      Output ("Dissconect:PNC");
    }
  
    // While this function will disconnect from the Cloud, it will keep the connection to the network.
    Particle.disconnect();
    waitFor(Particle.disconnected, 1000);  // Returns true when disconnected from the Cloud.
  
    // Output ("Cell Disconnect");
    #if PLATFORM_ID == PLATFORM_BORON
    Cellular.disconnect();
    Cellular.off();
    #else
    WiFi.disconnect();
    WiFi.off();
    #endif
    delay(1000); // in case of race conditions
  
    StartedConnecting = 0;
    ParticleConnecting = false;
}
  
 /*
 * ======================================================================================================================
 * NetworkConnect() - Turn on and Connect to Particle if not already   
 * 
 * You must not turn off and on cellular more than every 10 minutes (6 times per hour). Your SIM can be blocked by
 * your mobile carrier for aggressive reconnection if you reconnect to cellular very frequently. If you are manually 
 * managing the cellular connection in case of connection failures, you should wait at least 5 minutes before 
 * stopping the connection attempt.         
 * ======================================================================================================================
 */
bool NetworkConnect() {
    if (!Particle.connected()) {
       if(StartedConnecting == 0) {  // Not already connecting
   #if PLATFORM_ID == PLATFORM_BORON
         Cellular.on();
         Output ("Cell Connecting");
         Cellular.connect();
   #else
         WiFi.on();
         Output ("WiFi Connecting");
         WiFi.connect();
   #endif
         StartedConnecting = System.millis();
       }
   #if PLATFORM_ID == PLATFORM_BORON
       else if(Cellular.ready() && !ParticleConnecting) {
   #else
       else if(WiFi.ready() && !ParticleConnecting) {
   #endif
         // Time is synchronized automatically when the device connects to the Cloud.
         // Use NetworkReady to know when to update RTC
         NetworkReady = System.millis();
         Output ("Particle Connecting");
         Particle.connect();
         ParticleConnecting = true;
       }
       else if((System.millis() - StartedConnecting) >= (CLOUD_CONNECTION_TIMEOUT * 1000) ) { 
         //Already connecting - check for time out 60s
         Output ("Connect Timeout");
         NetworkDisconnect();
         return false;   // return false when timmed out trying to connect
       }
     }
     return true; // return true for being connected or in the process of connecting
}
   

/*
 * ======================================================================================================================
 * MainLoopLP() - 
 * ======================================================================================================================
 */
void MainLoopLP() {
  bool PowerDown = false;

  // If Serial Console Pin LOW then read Distance Gauge, Print and Sleep
  // Used for calibrating Distance Gauge at Installation
  // Only stay in this mode for countdown seconds, this protects against burnt out pin or forgotten jumper
  if (countdown && digitalRead(SCE_PIN) == LOW) {
    StationMonitor();

    if (!Particle.connected()) {
      NetworkConnect(); // Need to connect to Particle to get board time set
    }
    // StartedConnecting = System.millis();  // Keep set timer so when we pull jumper we finish connecting and not time out
    countdown--;

    // Taking the Distance measurement is delay enough
  }
  else { // Normal Operation - Main Work
    if (Time.isValid()) {
      if (Particle.connected()) {
        Output ("Particle Connected");

        if (SendSystemInformation) {
          INFO_Do(); // Function sets SendSystemInformation back to false.
        }

        OBS_Do();

        // Shutoff System Status Bits related to initialization after we have logged first observation 
        JPO_ClearBits();
      
        // STC is automatically updated when the device connects to the Cloud
        rtc.adjust(DateTime(Time.year(), Time.month(), Time.day(), Time.hour(), Time.minute(), Time.second() ));
        Output("RTC: Updated");

        PowerDown = true;
      }
      else {
        bool timedOut = NetworkConnect(); // Check on how we are doing connecting to the Cell Network

        if(timedOut == false) { 
          // We failed to connect the Modem is off
          Output ("Connect Failed");
          OBS_Do(); // Particle will not be connected,  this will cause N2S created

          // Shutoff System Status Bits related to initialization after we have logged first observation 
          JPO_ClearBits();

          PowerDown = true;
        }
        else {
          delay (1000); // Waiting on network
        }
      }
    }
    else {
      if (!Particle.connected()) {
        NetworkConnect(); // Need to connect to Particle to get board time set
      }
      stc_timestamp();
      Output(timestamp);
      Output("ERR: No Clock");
      delay (DELAY_NO_RTC);
    }

    // Before we go into ULP mode for 15m, check if we are not connected to a charging source
    // and our battery is at a low level. If so then power down the display and board. 
    // Wait for power to return.
    //
    // We do this at a high enough battery level to avoid the board from powering
    // itself down out of our control. The goal is to leave enough battery for the sensors to
    // chew on and still be able, when power returns, to charge the battery and transmit 
    // with out a current drop causing the board to reset or power down out of our control.
#if PLATFORM_ID == PLATFORM_BORON
    if (PowerDown) {
      if (DoLowPowerStayOn()) {
        Output ("Low Power Stay On");
        Output ("Stay On until Next OBS");
        delay (seconds_to_next_obs()*1000);
      }
      if (firmwareUpdateInProgress) {
        Output ("FW Update In Progress");
        Output ("Stay On until Next OBS");
        delay (seconds_to_next_obs()*1000);
      }
      // We are on battery and have 10% or less percent left then turnoff and wait for power to return
      else if (!pmic.isPowerGood() && (System.batteryCharge() <= 10.0)) {

        Output("Low Power!");

        // Disconnect from the cloud and power down the modem.
        Particle.disconnect();
        Cellular.off();
        delay(5000);

        Output("Powering Down");

        OLED_sleepDisplay();
        delay(5000);

        // Disabling the BATFET disconnects the battery from the PMIC. Since there
        // is no longer external power, this will turn off the device.
        pmic.disableBATFET();

        // This line should not be reached. When power is applied again, the device
        // will cold boot starting with setup().

        // However, there is a potential for power to be re-applied while we were in
        // the process of shutting down so if we're still running, enable the BATFET
        // again and reconnect to the cloud. Wait a bit before doing this so the
        // device has time to actually power off.
        delay(2000);

        OLED_wakeDisplay();   // May need to toggle the Display reset pin.
        delay(2000);
        Output("Power Re-applied");

        pmic.enableBATFET();

        // Start Network
        StartedConnecting = 0; 
        ParticleConnecting = false;
        NetworkConnect();
      }
      else {
        Output ("Going to Sleep");
        // Disconnect from the cloud and power down the modem.
        Particle.disconnect();
        Cellular.off();

        OLED_sleepDisplay();
        delay(2000);        

        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(seconds_to_next_obs()*1000);
        SystemSleepResult result = System.sleep(config);

        // On wake, execution continues after the the System.sleep() command 
        // with all local and global variables intact.
        // System time is still valid.
       
        OLED_wakeDisplay();
        delay(2000);
        Output("Wake Up");

        // Start Network
        StartedConnecting = 0;
        ParticleConnecting = false;
        NetworkConnect();

        // See what sensors we have - any go off line while sleeping?
        I2C_Check_Sensors();
      } // sleep
    } // powerdown
#endif
#if PLATFORM_ID == PLATFORM_ARGON
    if (PowerDown) {
      if (firmwareUpdateInProgress) {
        Output ("FW Update In Progress");
        Output ("Delaying until Next OBS");
        delay (seconds_to_next_obs()*1000);
      }
      else {
        Output ("Going to Sleep");
        // Disconnect from the cloud and power down the modem.
        Particle.disconnect();
        WiFi.off();

        OLED_sleepDisplay();
        delay(2000);        

        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(seconds_to_next_obs()*1000);
        SystemSleepResult result = System.sleep(config);

        // On wake, execution continues after the the System.sleep() command 
        // with all local and global variables intact.
        // System time is still valid.
       
        OLED_wakeDisplay();
        delay(2000);
        Output("Wake Up");

        // Start Network
        StartedConnecting = 0;
        ParticleConnecting = false;
        NetworkConnect();

        // See what sensors we have - any go off line while sleeping?
        I2C_Check_Sensors(); // Make sure Sensors are online
      } // sleep
    } // powerdown
#endif 
  } // normal operation
}