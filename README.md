# SETUP GUIDE

## Authorization

In order to get access, you will first need to create an account at [https://kits.teleagriculture.org/](https://kits.teleagriculture.org). After signing up contact an admin to assign the kit to your account. 

The admin assigns the kit by going to the configuration screen of the kit where a user can be added.

Once the kit is assigned to you, you can view its details and retrieve its API Key. The API Key gives access to exactly one kit, so if you have multiple kits you will have multiple API Keys. You can always get the current API Key in the settings of each kit.

## Arduino

- Download and install the [Arduino IDE](https://www.arduino.cc/en/software).
- Open `teleagriculture_v2.ino`.
- On line 10 where it says: `#define TAC_ID 1001` change `1001` for the kit id number you have been supplied with.
- On line 11 where it says: `#define TAC_API_KEY "CHANGEME"` change `CHANGEME` to the API Key that belongs to your kit. Keep the quotes (`"`) around it.
- **ToDo** Image needs to be updated for added TAC_API_KEY ![change line 11](https://gitlab.com/teleagriculture/community/-/raw/main/doc/images/ard01.png)

- flash the arduino by pressing the **Upload** button.
  ![press upload](https://gitlab.com/teleagriculture/community/-/raw/90a191bf26f24467d258fe4d3be05e8bc58d356a/doc/images/ard02.png)

## Rasperry Pi

- Download and install the [Raspberry Pi Imager](https://www.raspberrypi.com/software/).
- Insert SD card (min. 8GB) in to card reader.
- Open the Raspberry Pi Imager and install the latest Raspberry Pi OS on the SD card. First select the Raspberry pi
- Desktop Operating System, then select you SD card from the storage options, finally press `write`.
  ![rpi imager](https://gitlab.com/teleagriculture/community/-/raw/main/doc/images/rpi_imager.png)
- Insert SD card into the slot on the Raspberry Pi and powerup the Raspberry Pi.
- Once started make sure you have a wifi connection, you can follow [this guide to set it up](https://www.raspberrypi.com/documentation/computers/configuration.html#using-the-desktop).
- Open the terminal and type `sudo apt update && sudo apt upgrade && sudo apt install python3-pip`
  ![install pip](https://gitlab.com/teleagriculture/community/-/raw/main/doc/images/rpi_install_pip.png)
- Then followed up by `pip3 install requests`
- Open a browser and browse to [here](https://gitlab.com/teleagriculture/community) and download the repository as a zipfile.
  ![download repo](https://gitlab.com/teleagriculture/community/-/raw/main/doc/images/download_community_repo.png)
- Unpack the zipfile and put the files in a place where you know the path of.
- Connect the Arduino to the USB port of the Raspberry Pi.
- Open `tacserial_service.service` and and enter the correct path to `tacserial.py` where it says `[ENTER/CORRECT/ADDRESS/HERE]`.
  ![set path](https://gitlab.com/teleagriculture/community/-/raw/main/doc/images/set_path_in_service_file.png)
- In the terminal `cd` to the path where the `tacserial_service.service` file is and execute `sudo cp tacserial_service.service /lib/systemd/system/`.
  ![copy service file](https://gitlab.com/teleagriculture/community/-/raw/main/doc/images/copy_service_file.png)
- Still in the terminal, execute `sudo systemctl enable tacserial_service.service`.
  ![enable service](https://gitlab.com/teleagriculture/community/-/raw/main/doc/images/enable_service.png)
- Finally execute `raspi-config` and go to `System Options -> Boot / Auto Login -> Console Autologin` then press `Finish` and reboot.
  ![start raspi-config](https://gitlab.com/teleagriculture/community/-/raw/main/doc/images/start_raspi_config.png)
  ![raspi-config menu's](https://gitlab.com/teleagriculture/community/-/raw/main/doc/images/raspi_config_consoleboot.png)
- Once booted you should see the tacserial program producing output on the screen.

# API

See [API.md](API.md)
