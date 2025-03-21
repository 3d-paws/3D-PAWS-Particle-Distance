# 3D-PAWS-Particle-Distance

## Overview
3D-PAWS-Particle-Distance is designed for Snow and Stream stations, utilizing Particle devices to collect and transmit distance sensor data.

## Features

### Particle Event Type: `DS`
- Observations are transmitted under the Particle Event Type "DS," which stands for Distance Sensor.

### Supported Sensors
- Distance Sensor on pin A4
  - 5-meter sensors (MB7360, MB7369, MB7380, and MB7389)
  - 10-meter sensors (MB7363, MB7366, MB7383, and MB7386)
- I2C Sensors
  - BMP280, BME280, BMP390, BMP388 (Temperature, Pressure & Altitude)
  - HTU21DF (Humidity & Temperature Sensor)
  - MCP9808 (Temperature Sensor)
  - SHT31 (Temperature & Humidity Sensor)
  - HIH8000 (Temperature & Humidity Sensor)
  - SI1145 (UV/IR/Visible Light Sensor)
  - VMEL7700 (LUX)

### Particle Event Type: `INFO`
- Station information is sent to Particle as Event Type "INFO."
- This occurs at boot and can also be triggered via the Particle Cloud console using the `DoAction` function.

### Serial Monitor Support
- Enabling serial output:
  - Connect a jumper wire between Particle pin `D8` and ground before booting.
  - Serial output is configured to 9600 baud.
- Monitoring options:
  - Arduino IDE serial monitor.
  - On macOS with Visual Studio and Particle's Development Environment, use:
    ```sh
    particle serial monitor
    ```
- At boot, with the jumper wire connected:
  - The software will wait 60 seconds for a serial monitor connection.
  - The board LED will flash during this period.
  - After 60 seconds, normal boot continues.
- Example serial output includes initialization details and connected devices.

### 3rd Party SIM Support
- The Particle Boron has an external SIM slot.
- The current SIM state (`INTERNAL` or `EXTERNAL`) is stored in NVRAM.
- Control SIM usage via an SD card file `SIM.TXT`:

#### `SIM.TXT` Configuration Options
1. **APN, Username, and Password**:
   ```
   AUP epc.tmobile.com username password
   ```
2. **Username and Password Only**:
   ```
   UP username password
   ```
3. **APN Only**:
   ```
   APN epc.tmobile.com
   ```
4. **Use Internal SIM**:
   ```
   INTERNAL
   ```
- Upon successful processing, `SIM.TXT` is renamed to `SIMOLD.TXT`.
- Serial console messages indicate SIM status.
- Board LED will flash indefinitely, indicating a required reboot.

📌 **Reference:** [Particle 3rd-Party SIM Guide](https://docs.particle.io/troubleshooting/guides/connectivity-troubleshooting/using-3rd-party-sim-cards/)

### Argon WiFi Board Support
- Initial WiFi setup via the Particle mobile app.
- To modify WiFi credentials:
  - Use the app or create a file `WIFI.TXT` on the SD card.

### `WIFI.TXT` Format
```sh
AuthType,SSID,Password
```
**AuthType Options:**
- `UNSEC`
- `WEP`
- `WPA`
- `WPA2`
- `WPA_ENTERPRISE`
- `WPA2_ENTERPRISE`

📌 **Example Output (Serial Console):**
```
WIFI:Open
WIFI:ATYPE[WPA2]
WIFI:SSID[MySSID]
WIFI:PW[MyPassword]
WIFI:Credentials Cleared
WIFI:Credentials Set [WPA2]
```
- If `WIFI.TXT` is missing, credentials in NVRAM are used.

## Particle Cloud Console: `DoAction` Functions
Accessible in the **FUNCTIONS** section of the Particle Console device view:
- `REBOOT` – Reboots the device.
- `SEND` – Takes an observation and transmits it.
- `INFO` – Sends station information (`INFO` event).
- `MLLP,#` – Sets **Low Power Mode** with observation interval `# = 15, 20, or 30` minutes.
- `BLDS,#` – Sets **BaseLine Distance**. `# = 1-4095` (creates BLDIST.TXT), `#=0` (deletes BLDIST.TXT).
- `MLAC,#` – Sets **Always Connected Mode** with observation interval `# = 1-6, 10, 12, 15, 20, or 30` minutes (creates MAINLOOP.TXT).
- `5MDIST` – Sets distance sensor type to **5M** (creates `5MDIST.TXT`).
- `10MDIST` – Sets distance sensor type to **10M** (deletes `5MDIST.TXT`).
- `SOHR,#` – Stay On Hour `# = 0-23 UTC`
- `SOHR,OFF` – Turn off Stay On Hour

## SD Card Requirements
- **Max Size:** 32GB (Larger cards are not supported)
- **Format Tool:** [SD Card Formatter by TUXERA](https://www.sdcard.org/downloads/formatter/)
- **File System:** MS-DOS (FAT32)

### SD Card Files & Directories
```
/OBS          # Directory for observation files
/OBS/YYYYMMDD.LOG  # Daily observation logs (JSON format)
/N2SOBS.TXT   # Stores unsent observations (auto-deletes when full or sent)
/SIM.TXT      # Controls SIM selection (INTERNAL/EXTERNAL)
/WIFI.TXT     # Stores WiFi credentials for Argon boards
/INFO.TXT     # Stores latest station information (overwritten on each `INFO` call)
/MAINLOOP.TXT # Controls Main Loop behavior
/5MDIST.TXT    # Indicates 5M distance sensor configuration
/BLDIST.TXT    # File used to set distance sensor baseline. If non zero then (distance = baseline - reading) else reading.
/LPSOHR.TXT    # Store the Low Power Stay On Hour Store
```

### `MAINLOOP.TXT` Format
Controls observation timing and connection mode:
```
LP,#  # Low Power Mode - Interval options: 15, 20, 30 minutes
AC,#  # Always Connected Mode - Interval options: 1-6, 10, 12, 15, 20, 30 minutes
```
📌 **Example Timings:**
- `AC,2` → Observations at **0, 2, 4, 6, ...** minutes past the hour.
- `AC,12` → Observations at **0, 12, 24, 36, 48** minutes past the hour.

### `LPSOHR.TXT` Format
Stores the UTC top of the hour value where low power mode will not disconnect but stay on until next obs interval. To turn off this feature, remove the file.
```
File contents is one line. A UTC hour value from 0-23

Example seeting for MST at 10 AM
Mountain Standard Time (MST) is UTC-7. Then a 10 AM MST stay on time would be a UTC time of 17 (5 PM UTC). Place a value of 17 in the file.
```
---
