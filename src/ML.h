/*
 * ======================================================================================================================
 *  ML.h - MainLoop code
 * ======================================================================================================================
 */


/*
 * ======================================================================================================================
 * MainLoopInitialize() - Initialize Main Loop Control and Interval information from SD card file
 * ======================================================================================================================
 */
void MainLoopInitialize() {
  File fp;
  int i=0;
  int mi=0;
  char ch, buf[32]; 

  Output("ML_Init");
  
  // Get Main Loop Control and Interval information from SD card file
  if (SD_exists && SD.exists(SD_MAIN_LOOP_FILE)) {
    // Read first line out of file
    // LP,# - Low Power, Minute Interval on the hour for observation (15,20,30)
    // AC,# - Always Connected, Minute Interval on the hour for observation (1-6,10,12,15,20,30)

    // if error, log message go with defaults
    fp = SD.open(SD_MAIN_LOOP_FILE, FILE_READ); // Open the file for reading, starting at the beginning of the file.

    if (fp) {
      // Deal with invalid file size for format XX,## with possible CR and/or LF
      if (fp.size()>=4 && fp.size()<=7) {
        Output ("MLF:Open");
        // Read one line from file
        while (fp.available() && (i < 31 )) {
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

        // Process line get Mode
        if (buf[2] == ',') {
          mi = atoi (&buf[3]); // Get Minute Interval
          if (buf[0] == 'L' && buf[1] == 'P') {
            if ((mi == 15) || (mi == 20) || (mi == 30)) {
              
              MLMode = MLLP;
              MLOBS_Interval = mi;
            }
            else {
              // Error Invalid Minute
              sprintf (Buffer32Bytes, "MLLP:Min %d Err", mi);
              Output(Buffer32Bytes);
            }
          }
          else if (buf[0] == 'A' && buf[1] == 'C') {
            if (((mi >= 1) && (mi <= 6)) || (mi == 10) || (mi == 12) || (mi == 15) || (mi == 20) || (mi == 30)) {
              MLMode = MLAC;
              MLOBS_Interval = mi;
            }
            else {
              // Error Invalid Minute
              sprintf (Buffer32Bytes, "MLAC:Min %d Err", mi);
              Output(Buffer32Bytes);
            }
          }
          else {
            // Error Unknown Mode
            sprintf (Buffer32Bytes, "MLF:Mode [%c%c] Err", buf[0], buf[1]);
            Output(Buffer32Bytes);
          }
        }
        else {
          // Error Format
          Output ("MLR:Format Err");
        }
      }
      else {
        fp.close();
        Output ("MLF:Invalid SZ");       
      }
    }
    else {
      sprintf (Buffer32Bytes, "MLF:Open[%s] Err", SD_MAIN_LOOP_FILE);          
      Output(Buffer32Bytes);
    }
  }
  else {
    // Going with the defaults MLMode= MLAC
    Output ("MLF:NF");   
  }
  sprintf (Buffer32Bytes, "MLI:%s:%d", ml_modes[MLMode], MLOBS_Interval);
  Output (Buffer32Bytes);
}  