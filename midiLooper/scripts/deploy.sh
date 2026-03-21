#!/bin/bash

RPI_IP_ADDRESS="$1"

scp midiLooper.js pi@"$RPI_IP_ADDRESS":/home/pi/midiLooper.js
scp midiLooper.html pi@"$RPI_IP_ADDRESS":/home/pi/midiLooper.html
scp ./scripts/restartRtpMidi.sh pi@"$RPI_IP_ADDRESS":/home/pi/restartRtpMidi.sh
ssh pi@"$RPI_IP_ADDRESS" 'sudo mv /home/pi/midiLooper.js /root/NimbusPi/midiLooper/midiLooper.js && \
sudo mv /home/pi/midiLooper.html /root/NimbusPi/midiLooper/midiLooper.html && \
sudo mv /home/pi/restartRtpMidi.sh /root/NimbusPi/midiLooper/scripts/restartRtpMidi.sh && \
sudo chmod 777 /root/NimbusPi/midiLooper/midiLooper.html && \
sudo chmod 777 /root/NimbusPi/midiLooper/midiLooper.js && \
sudo chmod 777 /root/NimbusPi/midiLooper/scripts/restartRtpMidi.sh && \
sudo /usr/sbin/service midiLooper restart && \
/usr/bin/journalctl -u midiLooper.service -f'
