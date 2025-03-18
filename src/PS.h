/*
 * ======================================================================================================================
 * PS.h - Particle Support Funtions
 * ======================================================================================================================
 */

/*
 * ======================================================================================================================
 * Output_CellBatteryInfo() - On OLED display station information
 * ======================================================================================================================
 */
void Output_CellBatteryInfo() {
#if PLATFORM_ID == PLATFORM_ARGON
  WiFiSignal sig = WiFi.RSSI();
  float SignalStrength = sig.getStrength();

  sprintf (Buffer32Bytes, "CS:%d.%02d", 
    (int)SignalStrength, (int)(SignalStrength*100)%100);
  Output(Buffer32Bytes);
#else
  CellularSignal sig = Cellular.RSSI();
  float SignalStrength = sig.getStrength();
  
  int BatteryState = System.batteryState();
  float BatteryPoC = 0.0;                 // Battery Percent of Charge

  // Read battery charge information only if battery is connected. 
  if (BatteryState>0 && BatteryState<6) {
    BatteryPoC = System.batteryCharge();
  }
  
  sprintf (Buffer32Bytes, "CS:%d.%02d B:%d,%d.%02d", 
    (int)SignalStrength, (int)(SignalStrength*100)%100,
    BatteryState, (int)BatteryPoC, (int)(BatteryPoC*100)%100);
  Output(Buffer32Bytes);
#endif
}

#if PLATFORM_ID == PLATFORM_ARGON
/*
 * ======================================================================================================================
 * WiFiChangeCheck() - Check for WIFI.TXT file and set Wireless SSID, Password            
 * ======================================================================================================================
 */
void WiFiChangeCheck() {
  File fp;
  int i=0;
  char *p, *auth, *ssid, *pw;
  char ch, buf[128];

  if (SD_exists) {
    // Test for file WIFI.TXT
    if (SD.exists(SD_wifi_file)) {
      fp = SD.open(SD_wifi_file, FILE_READ); // Open the file for reading, starting at the beginning of the file.

      if (fp) {
        // Deal with too small or too big of file
        if (fp.size()<=7 || fp.size()>127) {
          fp.close();
          Output ("WIFI:Invalid SZ");
        }
        else {
          Output ("WIFI:Open");
          // Read one line from file
          while (fp.available() && (i < 127 )) {
            ch = fp.read();

            // sprintf (msgbuf, "%02X : %c", ch, ch);
            // Output (msgbuf);

            if ((ch == 0x0A) || (ch == 0x0D) ) {  // newline or linefeed
              break;
            }
            else {
              buf[i++] = ch;
            }
          }
          fp.close();
          Output ("WIFI:Close");

          // At this point we have encountered EOF, CR, or LF
          // Now we need to terminate array with a null to make it a string
          buf[i] = (char) NULL;

          // Parse string for the following
          //   WIFI ssid password
          p = &buf[0];
          auth = strtok_r(p, ",", &p);

          if (auth == NULL) {
            Output("WIFI:ID=Null Err");
          }
          else if ( (strcmp (auth, "WEP") != 0)  &&
                    (strcmp (auth, "WPA") != 0)  &&
                    (strcmp (auth, "WPA2") != 0) ){
            sprintf (msgbuf, "WIFI:ATYPE[%s] Err", auth);          
            Output(msgbuf);
          }
          else {
            ssid = strtok_r(p, ",", &p);
            pw  = strtok_r(p, ",", &p);

            if (ssid == NULL) {
              Output("WIFI:SSID=Null Err");
            }
            else if (pw == NULL) {
              Output("WIFI:PW=Null Err");
            }
            else {
              sprintf (msgbuf, "WIFI:ATYPE[%s]", auth);          
              Output(msgbuf);
              sprintf (msgbuf, "WIFI:SSID[%s]", ssid);
              Output(msgbuf);
              sprintf (msgbuf, "WIFI:PW[%s]", pw);
              Output(msgbuf);

              // Connects to a network secured with WPA2 credentials.
              // https://docs.particle.io/reference/device-os/api/wifi/securitytype-enum/
              if (strcmp (auth, "UNSEC") == 0) {
                Output("WIFI:Credentials Cleared");
                WiFi.clearCredentials();
                Output("WIFI:Credentials Set UNSEC");
                WiFi.setCredentials(ssid, pw, UNSEC);
              }
              else if (strcmp (auth, "WEP") == 0) {
                Output("WIFI:Credentials Cleared");
                WiFi.clearCredentials();
                Output("WIFI:Credentials Set WEP");
                WiFi.setCredentials(ssid, pw, WEP);
              }
              else if (strcmp (auth, "WPA") == 0) {
                Output("WIFI:Credentials Cleared");
                WiFi.clearCredentials();
                Output("WIFI:Credentials Set WPA");
                WiFi.setCredentials(ssid, pw, WPA);
              }
              else if (strcmp (auth, "WPA2") == 0) {
                Output("WIFI:Credentials Cleared");
                WiFi.clearCredentials();
                Output("WIFI:Credentials Set WPA2");
                WiFi.setCredentials(ssid, pw, WPA2);
              }
              else if (strcmp (auth, "WPA_ENTERPRISE") == 0) {
                Output("WIFI:Credentials Cleared");
                WiFi.clearCredentials();
                Output("WIFI:Credentials Set WPAE");
                WiFi.setCredentials(ssid, pw, WPA_ENTERPRISE);
              }
              else if (strcmp (auth, "WPA2_ENTERPRISE") == 0) {
                Output("WIFI:Credentials Cleared");
                WiFi.clearCredentials();
                Output("WIFI:Credentials Set WPAE2");
                WiFi.setCredentials(ssid, pw, WPA2_ENTERPRISE);
              }
              else { 
                Output("WIFI:Credentials NOT Set");
                Output("WIFI:USING NVAUTH");
              }
            }
          }
        }
      }
      else {
        sprintf (msgbuf, "WIFI:Open[%s] Err", SD_wifi_file);          
        Output(msgbuf);
        Output ("WIFI:USING NVAUTH");
      }
    } 
    else {
      Output ("WIFI:NOFILE USING NVAUTH");
    }
  } // SD enabled
  else {
    Output ("WIFI:NOSD USING NVAUTH");
  }
}
#else
/*
 * ======================================================================================================================
 * SimChangeCheck() - Check for SIM.TXT file and set sim based on contents, after rename file to SIMOLD.TXT            
 * ======================================================================================================================
 */
void SimChangeCheck() {
  File fp;
  int i=0;
  char *p, *id, *apn, *un, *pw;
  char ch, buf[128];
  bool changed = false;

  SimType simType = Cellular.getActiveSim();

  if (simType == 1) {
    Output ("SIM:Internal");
  } else if (simType == 2) {
    Output ("SIM:External");
  }
  else {
    sprintf (msgbuf, "SIM:Unknown[%d]", simType);
    Output (msgbuf);
  }

  if (SerialConsoleEnabled && SD_exists) {
    // Test for file SIM.TXT
    if (SD.exists(SD_sim_file)) {
      fp = SD.open(SD_sim_file, FILE_READ); // Open the file for reading, starting at the beginning of the file.

      if (fp) {
        // Deal with too small or too big of file
        if (fp.size()<=7 || fp.size()>127) {
          fp.close();
          Output ("SIMF:Invalid SZ");
          if (SD.remove (SD_sim_file)) {
            Output ("SIMF->Del:OK");
          }
          else {
            Output ("SIMF->Del:Err");
          }
        }
        else {
          Output ("SIMF:Open");
          while (fp.available() && (i < 127 )) {
            ch = fp.read();

            if ((ch == 0x0A) || (ch == 0x0D) ) {  // newline or linefeed
              break;
            }
            else {
              buf[i++] = ch;
            }
          }

          // At this point we have encountered EOF, CR, or LF
          // Now we need to terminate array with a null to make it a string
          buf[i] = (char) NULL;

          // Parse string for the following
          //   INTERNAL
          //   AUP epc.tmobile.com username passwd
          //   UP username password
          //   APN epc.tmobile.com
          p = &buf[0];
          id = strtok_r(p, " ", &p);

          if (id != NULL) {
            sprintf (msgbuf, "SIMF:ID[%s]", id);
            Output(msgbuf);
          }

          if (strcmp (id,"INTERNAL") == 0) {
            Cellular.setActiveSim(INTERNAL_SIM);
            Cellular.clearCredentials();
            changed = true;
          }
          else if (strcmp (id,"APN") == 0) {
            apn = strtok_r(p, " ", &p);

            if (apn == NULL) {
              Output("SIMF:APN=Null Err");
            }
            else {
              Cellular.setActiveSim(EXTERNAL_SIM);
              Output("SIM:Set External-APN");

              // Connects to a cellular network by APN only
              Cellular.setCredentials(apn);
              Output("SIM:Set Credentials");
              sprintf (msgbuf, " APN[%s]", apn);
              Output(msgbuf);
              changed = true;
            }
          }
          else if (strcmp (id," UP") == 0) {
            un  = strtok_r(p, " ", &p);
            pw  = strtok_r(p, " ", &p);

            if (un == NULL) {
              Output("SIMF:Username=Null Err");
            }
            else if (pw == NULL) {
              Output("SIMF:Passwd=Null Err");
            }
            else {
              Cellular.setActiveSim(EXTERNAL_SIM);
              Output("SIM:Set External-UP");

              // Connects to a cellular network with USERNAME and PASSWORD only
              Cellular.setCredentials(un,pw);
              Output("SIM:Set Credentials");
              sprintf (msgbuf, " UN[%s]", un);
              Output(msgbuf);
              sprintf (msgbuf, " PW[%s]", pw);
              Output(msgbuf);
              changed = true;
            }
          }
          else if (strcmp (id,"AUP") == 0) {
            apn = strtok_r(p, " ", &p);
            un  = strtok_r(p, " ", &p);
            pw  = strtok_r(p, " ", &p);

            if (apn == NULL) {
              Output("SIMF:APN=Null Err");
            }
            else if (un == NULL) {
              Output("SIMF:Username=Null Err");
            }
            else if (pw == NULL) {
              Output("SIMF:Passwd=Null Err");
            }
            else {
              Cellular.setActiveSim(EXTERNAL_SIM);
              Output("SIM:Set External-AUP");

              // Connects to a cellular network with a specified APN, USERNAME and PASSWORD
              Cellular.setCredentials(apn,un,pw);
              Output("SIM:Set Credentials");
              sprintf (msgbuf, " APN[%s]", apn);
              Output(msgbuf);
              sprintf (msgbuf, "  UN[%s]", un);
              Output(msgbuf);
              sprintf (msgbuf, "  PW[%s]", pw);
              Output(msgbuf);
              changed = true;
            }
          }
          else {  // Pasrse Error
            sprintf (msgbuf, "SIMF:ID[%s] Err", id);
            Output(msgbuf);
          }
        }

        // No matter what happened with the above, rename file so we don't process again at boot
        // if SIMOLD.TXT exists then remove it before we rename SIM.TXT
        if (SD.exists(SD_simold_file)) {
          if (SD.remove (SD_simold_file)) {
            Output ("SIMF:DEL SIMOLD");
          }
        }

        if (!fp.rename(SD_simold_file)) {
          Output ("SIMF:RENAME ERROR");
        }
        else {
          Output ("SIMF:RENAME OK");
        }
        fp.close();

        // Notify user to reboot blink led forever
        if (changed) {
          Output ("==============");
          Output ("!!! REBOOT !!!");
          Output ("==============");

        }
        else {
          Output ("=====================");
          Output ("!!! SET SIM ERROR !!!");
          Output ("=====================");
        }
        
        while(true) { // wait for Host to open serial port
          Blink(1, 750);
        }
      }
      else {
        Output ("SIMF:OPEN ERROR");
      }
    } 
    else {
      Output ("SIM:NO UPDATE FILE");
    }
  } // Console and SD enabled
}
#endif

/*
 * ======================================================================================================================
 * DeviceReset() - Kill power to ourselves and do a cold boot
 * ======================================================================================================================
 */
void DeviceReset() {
  digitalWrite(REBOOT_PIN, HIGH);
  delay(5000);
  // Should not get here if relay / watchdog is connected.
  digitalWrite(REBOOT_PIN, LOW);
  delay(2000); 

  // May never get here if relay board / watchdog not connected.

  // Resets the device, just like hitting the reset button or powering down and back up.
  System.reset();
}

/*
 * ======================================================================================================================
 * Function_DoAction() - Handler for Particle Function DoAction     
 * ======================================================================================================================
 */
int Function_DoAction(String s) {
  if (strcmp (s,"REBOOT") == 0) {  // Reboot
    Output("DoAction:REBOOT");

    delay(1000);

    DeviceReset();

    // Never gets here and thus can never send a ack to Particle Portal
    return(0);  
  }

  else if (strcmp (s,"INFO") == 0) {  // Send System Information
    Output("DoAction:INFO");
    SendSystemInformation=true;
    return(0);  
  }

  else if (strcmp (s,"SEND") == 0) {  // Send OBS Now
    Output("DoAction:SEND");
    Time_of_next_obs=0;  // Change timing to force taking and sending an observation
    return(0);  
  }

  else if ((strncmp (s,"MLLP,", 5) == 0) && (strlen(s) == 7)) {
    Output("DoAction:MLAP");
    int mi = s.substring(5).toInt(); // Get Minute Interval after the comma, returns 0 if not all chars are numeric
    if ((mi == 15) || (mi == 20) || (mi == 30)) {
      if (SD_exists) {
        File fp = SD.open(SD_MAIN_LOOP_FILE, FILE_WRITE | O_TRUNC); // Open the file for reading, starting at the beginning of the file.

        if (fp) {
          if (strlen(s.substring(2)) == fp.print(s.substring(2))) { // Skip the ML
            MLMode = MLLP;
            MLOBS_Interval = mi;
            sprintf (Buffer32Bytes, "MLLP,%d Set", mi);
            Output(Buffer32Bytes);
            fp.close();
            return (0);
          }
          else {
            sprintf (Buffer32Bytes, "MLLP,%d WRITE ERR ", mi);
            Output(Buffer32Bytes);
            fp.close();
            return(-3);
          }
        }
        else {
          sprintf (Buffer32Bytes, "MLLP,%d OPEN ERR ", mi);
          Output(Buffer32Bytes); 
          return(-2);           
        }
      }
      else {
        sprintf (Buffer32Bytes, "MLLP,%d SD NF", mi);
        Output(Buffer32Bytes);
        return(-1);
      }
    }
    else {
      // Error Invalid Minute
      sprintf (Buffer32Bytes, "MLLP:Min %d Err", mi);
      Output(Buffer32Bytes);
      return(-4);
    }    
  }

  else if ((strncmp (s,"MLAC,", 5) == 0) && ((strlen(s) == 6) || (strlen(s) == 7))) {
    Output("DoAction:MLAC");
    int mi = s.substring(5).toInt(); // Get Minute Interval after the comma, returns 0 if not all chars are numeric 
    if (((mi >= 1) && (mi <= 6)) || (mi == 10) || (mi == 12) || (mi == 15) || (mi == 20) || (mi == 30)) {
      if (SD_exists) {
        File fp = SD.open(SD_MAIN_LOOP_FILE, FILE_WRITE | O_TRUNC); // Open the file for reading, starting at the beginning of the file.

        if (fp) {
          if (strlen(s.substring(2)) == fp.print(s.substring(2))) { // Skip the ML
            MLMode = MLAC;
            MLOBS_Interval = mi;
            sprintf (Buffer32Bytes, "MLAC,%d Set", mi);
            Output(Buffer32Bytes);
            fp.close();
            return (0);
          }
          else {
            sprintf (Buffer32Bytes, "MLAC,%d WRITE ERR ", mi);
            Output(Buffer32Bytes);
            fp.close();
            return(-3);
          }
        }
        else {
          sprintf (Buffer32Bytes, "MLAC,%d OPEN ERR ", mi);
          Output(Buffer32Bytes); 
          return(-2);           
        }
      }
      else {
        sprintf (Buffer32Bytes, "MLAC,%d SD NF", mi);
        Output(Buffer32Bytes);
        return(-1);
      }
    }
    else {
      // Error Invalid Minute
      sprintf (Buffer32Bytes, "MLAC:Min %d Err", mi);
      Output(Buffer32Bytes);
      return(-4);
    }    
  }

  else if ((strlen(s) == 8) && (strncmp (s, "SOHR,OFF", 8) == 0)) {
    Output("DoAction:SOHR,OFF");
    if (SD_exists) {
      if (SD.exists(SD_LPSOHR_FILE)) {
        if (SD.remove (SD_LPSOHR_FILE)) {
          Output ("SOHR DEL:OK");
          LowPowerStayOnHour = -1;
        }
        else {
          Output ("SOHR DEL:ERR");
          return(-2);
        }            
      }
      else {
        Output ("SOHR FILE NF");  
      }
    }
    else {
      Output("SOHR, SD NF"); 
      return(-1);      
    }
    return(0);
  }

  else if ((strncmp (s,"SOHR,", 5) == 0) && ((strlen(s) == 6) || (strlen(s) == 7))) {
    Output("DoAction:SOHR");
    String sub = s.substring(5);
    if (isnumeric((char *) sub.c_str())) {
      int soh = sub.toInt(); // Get Minute Interval after the comma, returns 0 if not all chars are numeric 
      if ((soh >= 0) && (soh <= 23)) {
        if (SD_exists) {
          File fp = SD.open(SD_LPSOHR_FILE, FILE_WRITE | O_TRUNC); // Open the file for reading, starting at the beginning of the file.

          if (fp) {
            if (sub.length() == fp.print(sub)) {
              Output("SOHR Set");
              sprintf (Buffer32Bytes, "SOHR=%d OK", soh);
              Output(Buffer32Bytes);
              LowPowerStayOnHour = soh;
              fp.close();
              return (0);
            }
            else {
              Output("SOHR WRITE ERR");
              fp.close();
              return(-3);
            }
          }
          else {
            Output("SOHR OPEN ERR"); 
            return(-2);           
          }
        }
        else {
          Output("SOHR SD NF");
          return(-1);
        }
      }
      else {
        // Error Invalid Hour
        sprintf (Buffer32Bytes, "SOHR !HR %d Err", soh);
        Output(Buffer32Bytes);
        return(-4);
      }
    }
    else {
      // Not Numeric
      sprintf (Buffer32Bytes, "SOHR !Numeric %s", sub.c_str());
      Output(Buffer32Bytes);
      return(-4);
    }    
  }
  
  else if (strcmp (s,"5MDIST") == 0) { // Set 5M Distance Sensor State File
    Output("DoAction:5MDIST");
    if (SD_exists) {
      if (SD.exists(SD_5M_DIST_FILE)) {
        Output ("5MDIST, ALREADY EXISTS");      
      }
      else {
        // Touch File
        File fp = SD.open(SD_5M_DIST_FILE, FILE_WRITE);
        if (fp) {
          fp.close();
          ds_adjustment = 1.25;
          Output ("5MDIST SET");
        }
        else {
          Output ("5MDIST OPEN ERR");
          return(-2);
        }
      }
    }
    else {
      Output("5MDIST, SD NF"); 
      return(-1);      
    }
    return(0);
  }

  else if (strcmp (s,"10MDIST") == 0) { // Remove 5M Distance Sensor State File if it exists
    Output("DoAction:10MDIST");
    if (SD_exists) {
      if (SD.exists(SD_5M_DIST_FILE)) {
        if (SD.remove (SD_5M_DIST_FILE)) {
          Output ("5MDIST, DEL:OK");
        }
        else {
          Output ("5MDIST, DEL:ERR");
          return(-2);
        }            
      }
      else {
        Output ("5MDIST, NF");  
      }
    }
    else {
      Output("10MDIST, SD NF"); 
      return(-1);      
    }
    return(0);
  }

  else if ((strncmp (s,"BLDS,", 5) == 0) && ((strlen(s) >= 6) && (strlen(s) <= 9))) { // BLDS,0 - BLDS,4095
    Output("DoAction:BLDS");
    int bl = s.substring(5).toInt(); // Get Minute Interval after the comma, returns 0 if not all chars are numeric
    if (bl == 0) {
      if (SD.remove (SD_BL_DIST_FILE)) {
        Output ("BLDS, DEL:OK");
        ds_baseline = 0;
        return (0);
      }
      else {
        Output ("BLDS, DEL:ERR");
        return(-4);
      }            
    }
    else if ((bl>=1) && (bl<=4095)) {
      if (SD_exists) {
        File fp = SD.open(SD_BL_DIST_FILE, FILE_WRITE | O_TRUNC); // Open the file for reading, starting at the beginning of the file.

        if (fp) {
          if (strlen(s.substring(5)) == fp.print(s.substring(5))) {
            ds_baseline = bl;
            sprintf (Buffer32Bytes, "BLDS SET %d", bl);
            Output(Buffer32Bytes);
            fp.close();
            return (0);
          }
          else {
            Output("BLDS WRITE ERR");
            fp.close();
            return(-3);
          }
        }
        else {
          Output("BLDS OPEN ERR"); 
          return(-2);           
        }
      }
      else {
        Output("BLDS SD NF");
        return(-1);
      }
    }
    else {
      // Error Invalid Distance
      sprintf (Buffer32Bytes, "BLDS VAL %d Err", bl);
      Output(Buffer32Bytes);
      return(-4);
    }    
  }

  else {
    Output("DoAction:UKN"); 
    return(-1);
  }
}

/*
 * ======================================================================================================================
 * callback_cimi() - Callback for International Mobile Subscriber Identity 
 * 
 * COMMAND: AT+CIMI   Note: Each line returned is a call to this callback function
 * SEE https://docs.particle.io/reference/device-os/api/cellular/command/
 * ======================================================================================================================
 */
int IMSI_Callback(int type, const char* buf, int len, char* cimi) {
  // sprintf (msgbuf, "AT+CIMI:%X [%s]", type, buf); 
  // Output (msgbuf);     

  if ((type == TYPE_UNKNOWN) && cimi) {
    if (sscanf(buf, "\r\n%[^\r]\r\n", cimi) == 1)
      /*nothing*/;
  }

  if (type == TYPE_OK) {
    return (RESP_OK);
  }
  return (WAIT);
}

/*
 * ======================================================================================================================
 * IMSI_Request() - Request modem to provided International Mobile Subscriber Identity number            
 * ======================================================================================================================
 */
void IMSI_Request() {
  // Get International Mobile Subscriber Identity
  if ((RESP_OK == Cellular.command(IMSI_Callback, imsi, 10000, "AT+CIMI\r\n")) && (strcmp(imsi,"") != 0)) {
    sprintf (msgbuf, "IMSI:%s", imsi);
    Output (msgbuf);
    imsi_valid = true;
  }
  else {
    Output("IMSI:NF");
  }
}