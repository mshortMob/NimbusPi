#!/bin/bash

start_listener_script () {
  echo 'searching for device'
  python readAccellSerial.py || sleep 1 && echo 'no accell device found, sleeping before retry' && start_listener_script
}

start_listener_script
