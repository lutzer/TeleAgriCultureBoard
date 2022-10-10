# SETUP GUIDE

## Arduino

* Download and install the [Arduino IDE](https://www.arduino.cc/en/software).
* Open `teleagriculture_v2.ino`.
* On line 11 where it says: `#define TAC_ID 1001` change `1001` for the kit id number you have been supplied with.
![change line 11](https://gitlab.com/teleagriculture/community/-/raw/main/doc/images/ard01.png)
* flash the arduino by pressing the **Upload** button.
![press upload](https://gitlab.com/teleagriculture/community/-/raw/90a191bf26f24467d258fe4d3be05e8bc58d356a/doc/images/ard02.png)


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

# API

## Endpoints for making GET request to the database

#### Get latest sensor data:
>`https://kits.teleagriculture.org/api/sensors/[KIT_ID]/data`
>
>`[KIT_ID]`: the kit id you are targeting.

return body example:

	{
	    "id": "35154",
	    "kit_id": "9999",
	    "timestamp": "2022-08-11 15:19:03",
	    "ftTemp": "2.00",
	    "gbHum": "0.00",
	    "gbTemp": "0.00",
	    "Moist": "0.00",
	    "co": "0.00",
	    "no2": "0.00",
	    "nh3": "0.0000",
	    "c3h8": "0.00",
	    "c4h10": "0.00",
	    "ch4": "0.00",
	    "h2": "0.00",
	    "c2h5oh": "0.00",
	    "pH": "0.00",
	    "no3": "2",
	    "no2_aq": "0",
	    "kh": "0",
	    "cl2": "0",
	    "nh4": "0",
	    "nh3_aq": "0",
	    "po4": "0",
	    "cu": "0",
	    "salinity": "0"
	}

#### Get latest strip data:
>`https://kits.teleagriculture.org/api/strip/[KIT_ID]/data`
>
>`[KIT_ID]`: the kit id you are targeting.

return body example:

	{
	    "id": "13",
	    "kit_id": "9999",
	    "timestamp": "2022-08-12 14:42:55",
	    "no3": "0.00",
	    "no2_strip": "0.00",
	    "gh": "-1.00",
	    "kh": "-1.00",
	    "ph_strip": "-1.00",
	    "cl2": "0.00"
	}

#### Get sensor history:
>`https://kits.teleagriculture.org/api/history/[KIT_ID]/[SENSOR_NAME]`
>
>Gets the gets the latest year of data.
>
>`[KIT_ID]`: the kit id you are targeting.
>`[SENSOR_NAME]`: the name of the sensor you want the history for.

return body example:

	[
	    {
	        "timestamp": "2021-08-26 09:51:56",
	        "ftTemp": "27.521250"
	    },
	    {
	        "timestamp": "2021-08-26 10:00:08",
	        "ftTemp": "28.406949"
	    },
	    {
	        "timestamp": "2021-08-26 11:00:40",
	        "ftTemp": "28.411379"
	    },
	    {
	        "timestamp": "2021-08-26 12:00:21",
	        "ftTemp": "28.072069"
	    },

	    ...

	]

