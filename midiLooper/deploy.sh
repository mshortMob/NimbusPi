#!/bin/bash

RPI_IP_ADDRESS="$1"

scp mtest.js pi@"$RPI_IP_ADDRESS":/home/pi/mtest.js
scp mpanel.html pi@"$RPI_IP_ADDRESS":/home/pi/mpanel.html
ssh pi@"$RPI_IP_ADDRESS" 'sudo mv /home/pi/mtest.js /root/NimbusPi/midiExperiements/mtest.js && \
sudo mv /home/pi/mpanel.html /root/NimbusPi/midiExperiements/mpanel.html && \
sudo chmod 777 /root/NimbusPi/midiExperiements/mpanel.html && \
sudo chmod 777 /root/NimbusPi/midiExperiements/mtest.js && \
sudo /usr/sbin/service mtest restart && \
/usr/bin/journalctl -u mtest.service -f'
