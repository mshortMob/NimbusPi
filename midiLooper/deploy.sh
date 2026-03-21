#!/bin/bash

RPI_IP_ADDRESS="$1"

scp mtest.js pi@"$RPI_IP_ADDRESS":/home/pi/mtest.js
scp mpanel.html pi@"$RPI_IP_ADDRESS":/home/pi/mpanel.html
scp bounce_rtp.sh pi@"$RPI_IP_ADDRESS":/home/pi/bounce_rtp.sh
ssh pi@"$RPI_IP_ADDRESS" 'sudo mv /home/pi/mtest.js /root/NimbusPi/midiLooper/mtest.js && \
sudo mv /home/pi/mpanel.html /root/NimbusPi/midiLooper/mpanel.html && \
sudo mv /home/pi/bounce_rtp.sh /root/NimbusPi/midiLooper/bounce_rtp.sh && \
sudo chmod 777 /root/NimbusPi/midiLooper/mpanel.html && \
sudo chmod 777 /root/NimbusPi/midiLooper/mtest.js && \
sudo chmod 777 /root/NimbusPi/midiLooper/bounce_rtp.sh && \
sudo /usr/sbin/service mtest restart && \
/usr/bin/journalctl -u mtest.service -f'
