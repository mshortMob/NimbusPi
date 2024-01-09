#!/bin/bash

RPI_IP_ADDRESS="$1"

scp osc_streamer.js pi@"$RPI_IP_ADDRESS":/home/pi/osc_streamer.js
scp osc_streamer_gui.html pi@"$RPI_IP_ADDRESS":/home/pi/osc_streamer_gui.html
ssh pi@"$RPI_IP_ADDRESS" 'sudo mv /home/pi/osc_streamer.js /root/osc_streamer.js && \
sudo mv /home/pi/osc_streamer_gui.html /root/osc_streamer_gui.html && \
sudo chmod 777 /root/osc_streamer_gui.html && \
sudo chmod 777 /root/osc_streamer.js && \
sudo /usr/sbin/service osc_streamer restart && \
tail -f /var/log/daemon.log'
