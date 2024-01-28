#!/bin/bash

RPI_IP_ADDRESS="$1"

scp js_controller.js pi@"$RPI_IP_ADDRESS":/home/pi/js_controller.js
scp js_controller_gui.html pi@"$RPI_IP_ADDRESS":/home/pi/js_controller_gui.html
ssh pi@"$RPI_IP_ADDRESS" 'sudo mv /home/pi/js_controller.js /root/js_controller.js && \
sudo mv /home/pi/js_controller_gui.html /root/js_controller_gui.html && \
sudo chmod 777 /root/js_controller_gui.html && \
sudo chmod 777 /root/js_controller.js && \
sudo /usr/sbin/service js_controller restart && \
tail -f /var/log/daemon.log'
