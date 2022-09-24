from time import sleep
import serial

is_calib = True
start_msg = bytearray([0x01, 0x00, 0x00, 0x00])
end_msg = bytearray([0x01, 0x01])
preheat_msg = bytearray([0x01, 0x02])
calibrating_msg = bytearray([0x01, 0x03])

ser = serial.Serial('/dev/ttyACM0', 9600)

sleep(2)
ser.write(start_msg)
print('starting calibration!\nThis can take a very long time, come back in 30 minutes!')
while is_calib:
    line = ser.read(2)

    if line == end_msg:
        is_calib = False
        print('finished calibrating!')
    elif line == preheat_msg:
        print('preheating...')
    elif line == calibrating_msg:
        print('calibrating...')

ser.close()
