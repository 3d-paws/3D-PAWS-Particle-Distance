/*
 * ======================================================================================================================
 *  DIST.h - Wind Rain Distance Functions
 * ======================================================================================================================
 */

/*
 * Distance Sensors
 * The 5-meter sensors (MB7360, MB7369, MB7380, and MB7389) use a scale factor of (Vcc/5120) per 1-mm.
 * Particle 12bit resolution (0-4095),  Sensor has a resolution of 0 - 5119mm,  Each unit of the 0-4095 resolution is 1.25mm
 * Feather has 10bit resolution (0-1023), Sensor has a resolution of 0 - 5119mm, Each unit of the 0-1023 resolution is 5mm
 * 
 * The 10-meter sensors (MB7363, MB7366, MB7383, and MB7386) use a scale factor of (Vcc/10240) per 1-mm.
 * Particle 12bit resolution (0-4095), Sensor has a resolution of 0 - 10239mm, Each unit of the 0-4095 resolution is 2.5mm
 * Feather has 10bit resolution (0-1023), Sensor has a resolution of 0 - 10239mm, Each unit of the 0-1023 resolution is 10mm
 */

/*
 * ======================================================================================================================
 * DS_Initialize() - Initize Distance Sensor
 * ======================================================================================================================
 */
void DS_Initialize() {
  File fp;
  int i=0;
  char ch, buf[32]; 

  Output("DS_Init");
  // Get Main Loop Control and Interval information from SD card file
  if (SD_exists && SD.exists(SD_BL_DIST_FILE)) {
    // Read first line out of file, it should be a sting of numeric characters

    // if error, log message go with defaults
    fp = SD.open(SD_BL_DIST_FILE, FILE_READ); // Open the file for reading, starting at the beginning of the file.

    if (fp) {
      // Deal with invalid file size. Pin has 12 bits resolution so 0-4095 
      if (fp.size()>1 && fp.size()<=6) {
        Output ("BLDF:Open");
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

        if (isnumeric(buf)) {
          int bl = atoi (buf);
          if ((bl>=0) && (bl<=4095)) {
            ds_baseline = bl;
          }
          else {
            // Error Invalid Not Numeric
            sprintf (Buffer32Bytes, "BLVAL[%d] ERR", bl);
            Output(Buffer32Bytes);
          } 
        }
        else {
          // Error Invalid Not Numeric
          sprintf (Buffer32Bytes, "BLVAL[%s] !#", buf);
          Output(Buffer32Bytes);          
        }
      }
      else {
        fp.close();
        Output ("BLDF:Invalid SZ");       
      }
    }
    else {
      sprintf (Buffer32Bytes, "BLDF:Open[%s] Err", SD_BL_DIST_FILE);          
      Output(Buffer32Bytes);
    }
  }
  else {
    // Going with the default of 0
    Output ("BLDF:NF");   
  }
  sprintf (Buffer32Bytes, "BLVAL[%d]", ds_baseline);
  Output(Buffer32Bytes);
}


/* 
 *=======================================================================================================================
 * ds_get_median() - this takes 15s to run
 *=======================================================================================================================
 */
unsigned int ds_get_median() {
  int i;

  for (i=0; i<DS_BUCKETS; i++) {
    delay(250);
    ds_buckets[i] = (int) analogRead(DISTANCEGAUGE) * ds_adjustment;;
    // sprintf (Buffer32Bytes, "SG[%02d]:%d", i, od_buckets[i]);
    // OutputNS (Buffer32Bytes);
  }
  
  mysort(ds_buckets, DS_BUCKETS);
  i = (DS_BUCKETS+1) / 2 - 1; // -1 as array indexing in C starts from 0
  
  return (ds_buckets[i]);
}


