/*
 * ======================================================================================================================
 *  TM.h - Time Management
 * ====================================================================================================================== 
 */

/*
 * ======================================================================================================================
 *  RTC Setup
 * ======================================================================================================================
 */
RTC_PCF8523 rtc;              // RTC_PCF8523 rtc;
DateTime now;
char timestamp[32];
bool RTC_valid = false;
bool RTC_exists = false;

/* 
 *=======================================================================================================================
 * stc_timestamp() - Read from System Time Clock and set timestamp string
 *=======================================================================================================================
 */
void stc_timestamp() {

  // ISO_8601 Time Format
  sprintf (timestamp, "%d-%02d-%02dT%02d:%02d:%02d", 
    Time.year(), Time.month(), Time.day(),
    Time.hour(), Time.minute(), Time.second());
}

/* 
 *=======================================================================================================================
 * rtc_timestamp() - Read from RTC and set timestamp string
 *=======================================================================================================================
 */
void rtc_timestamp() {
  now = rtc.now(); //get the current date-time

  // ISO_8601 Time Format
  sprintf (timestamp, "%d-%02d-%02dT%02d:%02d:%02d", 
    now.year(), now.month(), now.day(),
    now.hour(), now.minute(), now.second());
}

/* 
 *=======================================================================================================================
 * rtc_initialize()
 *=======================================================================================================================
 */
void rtc_initialize() {

  if (!rtc.begin()) { // Always returns true
     Output("ERR:RTC NOT FOUND");
     SystemStatusBits |= SSB_RTC; // Turn on Bit
     return;
  }
  
  if (!I2C_Device_Exist(PCF8523_ADDRESS)) {
    Output("ERR:RTC-I2C NOTFOUND");
    SystemStatusBits |= SSB_RTC; // Turn on Bit
    delay (5000);
    return;
  }

  RTC_exists = true; // We have a clock hardware connected

  // Particle.io library is 1.2.1 (Old). The below are supported in 1.12.4

  //we don't need the 32K Pin, so disable it
  // rtc.disable32K();

  // set alarm 1, 2 flag to false (so alarm 1, 2 didn't happen so far)
  // if not done, this easily leads to problems, as both register aren't reset on reboot/recompile
  // rtc.clearAlarm(1);
  // rtc.clearAlarm(2);

  // stop oscillating signals at SQW Pin
  // otherwise setAlarm1 will fail
  // rtc.writeSqwPinMode(DS3231_OFF);

  // turn off alarm 1, 2 (in case it isn't off already)
  // again, this isn't done at reboot, so a previously set alarm could easily go overlooked
  // rtc.disableAlarm(1);
  // rtc.disableAlarm(2);
  
  // now = rtc.now(); //get the current date-time - done in timestamp().

  rtc_timestamp();
  sprintf (msgbuf, "%s*", timestamp);
  Output (msgbuf);

  // Do a validation check on the year. 
  // Asumption is: If RTC not set, it will not have the current year.
  
  if (now.year() >= 2021) {
    now = rtc.now();
    Time.setTime(now.unixtime()); // If RTC valid, we set STC.
    RTC_valid = true;
  }
  else {
    Output ("NEED GSM TIME->RTC");
    delay (5000); // Give the user some time to see this problem.
  }
}

/*
 * ======================================================================================================================
 * RTC_UpdateCheck() - Check if we need to Set or Update the RTC clock from the Cell Network   
 * ======================================================================================================================
 */
void RTC_UpdateCheck() {
  if (RTC_exists && NetworkReady) { 
    // We have a RTC and We have connected to the Cell network at some point
    if (!RTC_valid) {
      // Set Uninitialized RTC from STC. Which has been set from the Cloud
      rtc.adjust(DateTime(Time.year(), Time.month(), Time.day(), Time.hour(), Time.minute(), Time.second() ));
      Output("RTC: Set");
      RTC_valid = true;
    }
    else if (LastTimeUpdate == 0){
      // We have never updated RTC from Cell network time 
      rtc.adjust(DateTime(Time.year(), Time.month(), Time.day(), Time.hour(), Time.minute(), Time.second() ));

      Output("RTC: 1ST SYNC");
      rtc_timestamp();
      sprintf (msgbuf, "%s*", timestamp);
      Output (msgbuf);
      LastTimeUpdate = System.millis();
    }
    else if ((System.millis() - LastTimeUpdate) >= 2*3600*1000) {  // It's been 2 hours since last RTC update
      rtc.adjust(DateTime(Time.year(), Time.month(), Time.day(), Time.hour(), Time.minute(), Time.second() ));

      Output("RTC: 2HR SYNC");
      rtc_timestamp();
      sprintf (msgbuf, "%s*", timestamp);
      Output (msgbuf);
      LastTimeUpdate = System.millis();
    }
  }
}

