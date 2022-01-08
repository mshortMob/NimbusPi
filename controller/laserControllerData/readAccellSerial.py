#!/usr/bin/python

import serial, string

while True:
    output = ''
    ser = serial.Serial('/dev/ttyUSB0', 115200, 8, 'N', 1, timeout=0.1)
    while output == '':
        output = ser.readline()
    print(output)
    f = open("/var/www/html/controller/laserControllerData/accellData.txt", "w")
    f.write(output)
    f.close()
