#!/bin/bash

RPI_IP_ADDRESS="$1"
current_hash="xxxxx"

while [ true ]
do
    if [ "$current_hash" != "$(cat canvas_testbed.html| shasum)" ]; then
        echo 'found change to deploy'
        current_hash="$(cat canvas_testbed.html| shasum)"
        bash $(pwd)/deploy.sh "$RPI_IP_ADDRESS"
        echo 'ran deploy script'
        echo ''
    fi
    sleep .2
done
