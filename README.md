# SETUP GUIDE

## Arduino

* Download and install the [Arduino IDE](https://www.arduino.cc/en/software).
* On line 11 where it says: `#define TAC_ID 1001` change `1001` for the kit id number you have been supplied with.
* Open `teleagriculture_v2.ino` and flash the arduino by pressing the **Upload** button.


## Rasperry Pi

* Download and install the [Raspberry Pi Imager](https://www.raspberrypi.com/software/).
* Insert SD card (min. 8GB) in to card reader.
* Open the Raspberry Pi Imager and install the latest Raspberry Pi OS on the SD card.
* Insert SD card into the slot on the Raspberry Pi and powerup the Raspberry Pi.
* Once started make sure you have a wifi connection, you can follow [this guide to set it up](https://www.raspberrypi.com/documentation/computers/configuration.html#using-the-desktop).
* Open the terminal and type `sudo apt update && sudo apt upgrade && sudo apt install python3-pip`
* Then followed up by `pip3 install requests`
* Open a browser and browse to [here](https://gitlab.com/teleagriculture/iot-v2) and download the repository as a zipfile.
* Unpack the zipfile and put the files in a place where you know the path of.
* Connect the Arduino to the USB port of the Raspberry Pi.
* Open `tacserial_service.service` and and enter the correct path to `tacserial.py` where it says `[ENTER/CORRECT/ADDRESS/HERE]`.
* In the terminal `cd` to the path where the `tacserial_service.service` file is and execute `sudo cp tacserial_service.service /lib/systemd/system/`.
* Still in the terminal, execute `sudo systemctl enable tacserial_service.service`.
* Finally execute `raspi-config` and go to `System Options -> Boot / Auto Login -> Console Autologin` then press `Finish` and reboot.
* Once booted you should see the tacserial program producing output on the screen.
