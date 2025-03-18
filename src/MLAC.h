/*
 * ======================================================================================================================
 *  MLAC.h - MainLoop code for Always Connected
 * ======================================================================================================================
 */

/*
 * ======================================================================================================================
 * HeartBeat() - 
 * ======================================================================================================================
 */
void HeartBeat() {
    digitalWrite(HEARTBEAT_PIN, HIGH);
    delay(250);
    digitalWrite(HEARTBEAT_PIN, LOW);
  }
  
  /*
   * ======================================================================================================================
   * BackGroundWork() - Take Sensor Reading, Check LoRa for Messages, Delay 1 Second for use as timming delay            
   * ======================================================================================================================
   */
  void BackGroundWork() {
    // Anything that needs sampling every second add below. Example Wind Speed and Direction, StreamGauge
    HeartBeat();  // 250ms
    delay (750);
  }

 /*
 * ======================================================================================================================
 * MainLoopAC() - Always Connected Mode
 * ======================================================================================================================
 */
void MainLoopAC() {
  BackGroundWork(); // Delays 1 second

  if (countdown && (digitalRead(SCE_PIN) == LOW)) {
    StationMonitor();
    countdown--;
  }
  else {
    if (!imsi_valid && Particle.connected() && (Time.now() >= imsi_next_try)) {
      IMSI_Request(); // International Mobile Subscriber Identity
      if (!imsi_valid) {
        imsi_next_try = Time.now() + 3600;
      }
    }

    // This will be invalid if the RTC was bad at poweron and we have not connected to Cell network
    // Upon connection to cell network system Time is set and this becomes valid
    if (Time.isValid()) {  
 
      // Set RTC from Cell network time.
      RTC_UpdateCheck();

      // Perform an Observation, Write to SD, and Transmit observation
      if (Time.now() >= Time_of_next_obs) {
        I2C_Check_Sensors(); // Make sure Sensors are online
        OBS_Do();

        // Given overhead of doing and transmitting an obs, when 1m obs just add 60s to current time.
        Time_of_next_obs = (MLOBS_Interval == 1) ? (Time.now() + 60) : (Time.now() + seconds_to_next_obs());

        // Shutoff System Status Bits related to initialization after we have logged first observation 
        JPO_ClearBits();

        // We are staying connected to the Cell network
        // request time synchronization from the Cell network - Every 4 Hours
        if ((System.millis() - LastTimeUpdate) > (4*3600*1000)) {
          // Note that this function sends a request message to the Cloud and then returns. 
          // The time on the device will not be synchronized until some milliseconds later when 
          // the Cloud responds with the current time between calls to your loop.

          // !!! What if we drop the Cell connection before we get a time update for the Cloud?
          Output ("NW TimeSync REQ");
          Particle.syncTime();
          LastTimeUpdate = System.millis();
        }
      }

      if (SendSystemInformation && Particle.connected()) {
        INFO_Do(); // Function sets SendSystemInformation back to false.
      }

    }
    else {
      stc_timestamp();
      Output(timestamp);
      Output("ERR: No Clock");
      delay (DELAY_NO_RTC);
    }

    // Reboot Boot Every 22+ hours - Not using time but a loop counter.
    if ((cf_reboot_countdown_timer>0) && (--DailyRebootCountDownTimer<=0)) {
      Output ("Daily Reboot");

      // Send an observation, this will then handle any unreported rain. Not needed
      // OBS_Do();

      // Lets not rip the rug out from the modem. Do a graceful shutdown.
      Particle.disconnect();
      waitFor(Particle.disconnected, 1000);  // Returns true when disconnected from the Cloud.

      // Be kind to the cell modem and try to shut it down
      Cellular.disconnect();
      delay(1000);
      Cellular.off();

      Output("Rebooting");  
      delay(1000);
   
      DeviceReset();

      // we should never get here, but just incase 
      Output("I'm Alive! Why?");  

		  Cellular.on();
      delay(1000);

		  Particle.connect();

      DailyRebootCountDownTimer = cf_reboot_countdown_timer; // Reset count incase reboot fails
    }   

    // Check our power status
    // If we are not connected to a charging source and our battery is at a low level
    // then power down the display and board. Wait for power to return.
    // Do this at a high enough battery level to avoid the board from powering
    // itself down out of our control. Also when power returns to be able to charge
    // the battery and transmit with out current drops causing the board to reset or 
    // power down out of our control.
    if (!pmic.isPowerGood() && (System.batteryCharge() <= 10.0)) {

      if (Particle.connected()) {
        INFO_Do(); // Function sets SendSystemInformation back to false.
      }

      Output("Low Power!");

      // While this function will disconnect from the Cloud, it will keep the connection to the network.
      Particle.disconnect();
      waitFor(Particle.disconnected, 1000);  // Returns true when disconnected from the Cloud.
      
      Cellular.disconnect();
      delay(1000);
      Cellular.off();

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

		  Cellular.on();

		  Particle.connect();
    }
  }
}
