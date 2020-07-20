# ShaderMapper

ShaderMapper is a basic projection mapping, VJ, and lighting design tool based in Three.js and designed primarily to be run as a local web app on the Raspberry Pi.

#### Mapping and VJ Features
- Multi object quad and grid warping of content written in GLSL fragment shaders. *(video playback not supported at this time due to limitations of the pi)*
- Simultanenos playback of 2 shaders which can be dynamically assigned to up to four objects *(althought he pi could probably handle a good but more than this if desired)*
- Can cycle through a bank of pre-defined fragment shaders and select from a bank of still images which can be used for masking or other manipulation within the shaders.
- Mouse and audio data are available to the fragment shaders as uniform inputs. 
- Controls are available both onscreen and via keyboard shortcuts *(can be critical in situations where your only monitori is a projector)*
- All setting can be stored and recalled as presets, allowing for the easy creation and playback of VJ type scenes.

#### DMX/ArtNet/Lighting Features
- DMX Features are just beyond proof of concept stage.
- Adding fixture, scene, and patching controls will be required before this is ready for public use.
- This is basically a custom UI on top of OLA, current code will not attempt to send DMX if OLA isn't setup (and reverse proxied) on your system.


## Running Locally

These instructions are specific to raspberry pi, but anyone familiar with linux should be able to exprapolate to other systems: 

1. install apache and php: `apt install apache2 php -y`
1. clone this repo in the webroot directory: `cd /var/www/html/ && git clone https://github.com/mshortMob/PoiPainter.git`
1. move the repo contents up into the web root directory: `mv PoiPainter/* ./ && mv  PoiPainter/.git ./`
1. make sure apache is running: `service apache2 start && service apache2 status`
1. make sure your running the latest version of chromium, and then just navigate to `http:/localhost`