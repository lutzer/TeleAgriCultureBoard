# teleAgriCulture Board V2.1

[<img src="https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/vscode.svg" alt="VS Code logo" width="50" height="50">](https://code.visualstudio.com)   &nbsp;   [<img src="https://cdn.platformio.org/images/platformio-logo-xs.fd6e881d.png" alt="PlatformIO logo" height="50">](https://platformio.org) &nbsp; [<img src="https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/ESP32-S3.png" alt="PlatformIO logo" height="50">](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)

## Getting started

Board Credentials are in `/include/board_credentials.h` (BoardID, API_KEY and LORA credentials)
For defines, GPIOs and implemented Sensors, see `/include/sensor_Board.hpp`
 
### Config Portal Access Point
SSID: TeleAgriCulture Board
pasword: enter123

 
<mark>!! to build this project, take care that board_credentials.h is in the include folder (gets ignored by git)</mark>
 
main() handles Config Accesspoint, WiFi, LoRa, load, save, time and display UI
 
Global vector to store connected Sensor data:
`std::vector<Sensor> sensorVector;`
 
Sensor Data Name: `sensorVector[i].measurements[j].data_name`    --> in order of apperiance (temp, temp1, temp2 ...)
Sensor Value:     `sensorVector[i].measurements[j].value`

## Wishlist

- [x] WPA2 enterprise support
- [x] LoRa regions
- [ ] support for alternative I2C adresses
- [ ] data upload intervall based on UI
 