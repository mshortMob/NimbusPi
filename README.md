# NimbusPi

NimbusPi is a collection of projection mapping, VJ, and lighting design tool built for the Raspberry Pi. 

![NimbusPi Demo Screenshot](http://www.nimbuslaboratory.com/static/NimbusPi/NimbusPi_Demo_Screenshot.png) 

## Overview
NimbusPi images currently contains 2 separate applications, ShaderMapper and FlowMapper. You can easily switch between these applications, but only 1 may be run at a time. See the feature sets for each below:

##### ShaderMapper (Note: USB Microphone Recomended)
- Multi object quad and grid warping with masking
- Content generated live by sound reactive GLSL Fragement Shaders (limit: simultaneous playback of 2x shaders, applied to up to 4 objects)
- Ability to record Artnet/DMX and playback at a variable rate or driven by audio amplitude.

##### FlowMapper (Note: USB Webcam Required)
- Multi object quad and grid warping, no masking
- Content generated by applying effects to a local webcam stream
- Effects include time delay, brightness and movement thresholds, kaleidoscoping, and color mixing.
- Works very well for live "Light Painting" installations

## Installation

Just download the latest image [here](http://www.nimbuslaboratory.com/static/NimbusPi/NimbusPi.img) and then burn to an SD card using [Raspberry Pi Imager](https://www.raspberrypi.org/downloads/) or an alternative of your choosing. These images should work on both the Pi 3b+ and Pi 4, although the apps run substantially smoother on a Pi 4.

## Usage Instructions

![NimbusPi User Guide](http://www.nimbuslaboratory.com/static/NimbusPi/NimbusPi%20User%20Guide_v2.jpg) 

### DMX/ArtNet/Wifi Details

#### Broadcasting Wifi and ArtNet
* The NimbusPi image configures the Pi as a standalone hotspot. By default the SSID is `NimbusPi` and password is `NimbusPi123`. 
* ArtNet data is sent via wifi only, not the Pi's ethernet port. 
* 1 full universe of output is supported. By default this is artnet `universe 1` in `subnet 0`. 

#### Recording New Presets
* To record a new preset, navigate to the `DmxEditor` page found at the bottom of the ShaderMapper control panel.
* When recording, you must send data to artnet `universe 2` in `subnet 0`. If you want to live monitor what you are recording, then send your data to both `universe 1` and `2` in `subnet 0`. 
* The DmxEditor page shows raw DMX data in CSV form, rows are dmx frames and columns are dmx channels. You may edit by hand or copy/poste as desired. 
* Be sure to press `Save Preset` once your recording or manual edits are complete.

#### Override Channels
* **Be aware that dmx pattern `0` is a special config file and not a raw DMX file as described above**.
* The data held in pattern `0` is simple CSV list.
    * The first 4 values in this list correspond to the 4 dmx override channels, which discards the listed channels' preset data and replaces it with data from the override controls. (Overrides are labelled `DMX_X1`, `DMX_X2`, `DMX_Y1`, `DMX_Y2` in the control panel)
    * Any subsequent values are channels which should not get scaled by the `Brightness` controls. (dmx mode channels for example) 
* Overrides are usefull for directly controlling things like position, strobe, or macro channels on DMX fixtures.
* If you dont need any override channels (such as with LED strips) then set pattern 0 to a value of `0,0,0,0`

#### Using NimbusPi with NodeMCU / ESP8266 
The Pi's built in hotspot works well to control any number of wifi enabled micro controllers capable of running [ArtNetWifi](https://github.com/rstephan/ArtnetWifi). The example code seen [here](https://github.com/mshortMob/NimbusPi/blob/master/esp8266_Firmware/Arnet_Pixel_Controler_Indiviual_Addressable/featherwing_arnet_w_webUI_for_strips.ino) should work with most generic WS2812b RGB leds, includes a simple WebUI control panel, and projects its own hotspot when it can't connect to the saved network. See the general instructions for using this script below:

- Set the led pin, number of leds, standalone hotspot's SSID and password [here](https://github.com/mshortMob/NimbusPi/blob/master/esp8266_Firmware/Arnet_Pixel_Controler_Indiviual_Addressable/featherwing_arnet_w_webUI_for_strips.ino#L7-L11).

- If you don't wan't to use the WebUI and want to flash your microcontroller to use the default NimbusPi wifi credentials, then simply uncomment the lines found [here](https://github.com/mshortMob/NimbusPi/blob/master/esp8266_Firmware/Arnet_Pixel_Controler_Indiviual_Addressable/featherwing_arnet_w_webUI_for_strips.ino#L65-L66)

- Use the [Arduino IDE](https://www.arduino.cc/en/main/software) or alternative of your choosing to write the code to your microcontroller.

- Once your controller is up and running, the led status codes are as follows:
    - Red = Attempting to connect to external Wifi
    - Blue = Failed to connect to external Wifi, HotSpot + WebUI Active
    - Green = Connnected to external wifi, waiting to receive artnet signal.

- Once connected to the hotspot (as defined [here](https://github.com/mshortMob/NimbusPi/blob/master/esp8266_Firmware/Arnet_Pixel_Controler_Indiviual_Addressable/featherwing_arnet_w_webUI_for_strips.ino#L10-L11)), the Config menu is available in your browser at `http://192.168.4.1/data/`. You may use this menu to set the device's artnet universe, arnet address, external wifi SSID, and external wifi password.


### Development

#### Running Locally

The code based is based around [Three.js](https://threejs.org/) and designed primarily to be run as a local web app. Most features will work on any machine capable of running a simple webserver, php, and chrome. Features related to DMX typically require additional configuration to proxy the [OLA](https://www.openlighting.org/ola/tutorials/ola-on-raspberry-pi/) port (9090) to your web port.

For local development there is no build process, just clone this repo to your webserver's root directoy. But do note that these apps use WebRTC's `getUserMedia`, and thus must either be hosted locally or on a site with TLS/SSH enabled.

#### Building Images

To build your own image, you must be setup to run [packer-builder-arm-image](https://github.com/solo-io/packer-builder-arm-image). Once this is done, simply run a packer build using the [packer template file found here](https://github.com/mshortMob/NimbusPi/blob/master/packer/build.json).

