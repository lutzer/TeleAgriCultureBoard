from time import sleep
import serial
import json
import requests
import lightserver
import sys

START_MSG = bytearray([0x02, 0x00, 0x00, 0x00])
RET_SIZE = 28
API_URL = 'https://kits.teleagriculture.org/api/sensors/'

if __name__ == '__main__':
    light_server = lightserver.LightServer()
    light_server.start()

    print('starting TACSERIAL..................................................................................')
    try:
        ser = serial.Serial('/dev/ttyACM0', 9600)
        sleep(30)
        ser.flushInput()
    except serial.serialutil.SerialException:
        print('arduino is not connected, please connect and reboot')
        sys.exit()

    while True:
        color = light_server.get_color()
        print(color)
        clr_msg = bytearray([0x03, color.r, color.g, color.b])
        ser.write(clr_msg)

        ser.write(START_MSG)
        line = ser.readline()

        data = line.decode('ascii').rstrip().split('\t')

        data_dict = {}
        if len(data) == RET_SIZE:
            for i in range(0, len(data), 2):
                if data[i] == 'id':
                    kid = data[i + 1]
                else:
                    data_dict[data[i]] = float(data[i + 1])

            url = '{0}{1}/data'.format(API_URL, kid)
            r = requests.post(url, json=data_dict)
            print(url)
            print(json.dumps(data_dict, indent=4))
            print(r.status_code)

        sleep(55)
