# M5Cardputer-UserDemo-Plus
Official firmware enhanced
 
- add arduino-esp32 as a ESP-IDF component for easier development
  - porting some native but messy code to Arduino lib to avoid crash
- porting @cyberwisk's cardputer WebRadio as an App
  - using a modded version of ESP8266Audio to support https and chunked stream (like qtfm.cn)
  - ability to play radio in background when pressing HOME (G0), press ESC to fully exit
  - more radios (you can modify the radio list at (main/apps/app_radio/M5Cardputer_WebRadio.cpp)
- enhance SCAN, TIMER (renamed to CLOCK), SetWiFi App
- add SCALES and ENV IV App for the mini-scales and ENV IV m5stack sensors
- system enhance
  - WiFi will remain connected in background in default, open SetWiFi App again to turn off
  - use ESP-IDF's automatic sntp
  - enhance system bar
    - show battery voltage and current free heep size
    - add real feature to WiFi icon

# M5Cardputer-UserDemo
M5Cardputer user demo for hardware evaluation.

#### Tool Chains

[ESP-IDF v4.4.6](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/index.html)

#### Build

```bash
git clone https://github.com/m5stack/M5Cardputer-UserDemo
```
```bash
cd M5Cardputer-UserDemo
```
```bash
idf.py build
```
