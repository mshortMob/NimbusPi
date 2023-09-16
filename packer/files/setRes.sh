#!/bin/bash


desired_resolutions=( #In order of most to least preffered
'1024x768'
'832x624'
'800x600'
'1152x864'
'1280x1024'
)

primary_output=$(xrandr -display :0.0 --query | grep 'connected primary' | awk -F '\ ' '{print $1}')

for desired_resolution in "${desired_resolutions[@]}"
do
    if xrandr -display :0.0 --query | grep 'connected primary' | grep "$desired_resolution"
    then
        echo 'Display already at desired resolution '"$desired_resolution"
        break
    elif xrandr -display :0.0 --query | grep "$desired_resolution"
    then
        echo 'Display supports '"$desired_resolution"' resolution, updating resolution now'
        xrandr -display :0.0 --output "$primary_output" --mode "$desired_resolution"
        xrefresh -display :0.0
        break
    else
        echo 'Leaving display resolution unchanged'
    fi
done


