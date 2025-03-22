#!/bin/bash

RPI_IP_ADDRESS="$1"

scp canvas_testbed.html pi@"$RPI_IP_ADDRESS":/home/pi/canvas_testbed.html
ssh pi@"$RPI_IP_ADDRESS" 'sudo mv /home/pi/canvas_testbed.html /var/www/html/canvas_testbed.html && \
sudo chmod 777 /var/www/html/canvas_testbed.html'
