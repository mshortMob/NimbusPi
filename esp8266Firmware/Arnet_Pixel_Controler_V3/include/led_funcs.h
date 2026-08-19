#include <FastLED.h>
#define NUM_LEDS 128
#define LED_PIN 15
CRGB leds[NUM_LEDS];
#include <led_synth.h>

void apply_max_brightness(void){
  int percent = epdata.max_brightness_percent;
  if(percent<1){
    percent=1;
  }
  if(percent>100){
    percent=100;
  }
  FastLED.setBrightness(map(percent, 0, 100, 0, 255));
}

void setup_leds(void){
  pinMode(LED_PIN, OUTPUT);
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  FastLED.clear();
  apply_max_brightness();
}

void clear_leds(){
  FastLED.clear();
}

void setLEDSToASingleColor(int r, int g, int b){
  for(int i=0;i<NUM_LEDS;i++){
    leds[i]=CRGB(r,g,b);
  }
  FastLED.show();
}

void setLEDSToDataArray(uint8_t* data, int filterChan){
  for(int i=0;i<NUM_LEDS;i++){
    leds[i]=CRGB(data[filterChan+0+(i*3)], data[filterChan+1+(i*3)], data[filterChan+2+(i*3)]);
  }
  FastLED.show();
}

void setLEDSForArtnet(uint8_t* data, int filterChan, int fixtureMode){
  if(fixtureMode==0){
      setLEDSToASingleColor(data[filterChan+0],data[filterChan+1],data[filterChan+2]);
  }
  if(fixtureMode==1){
    setLEDSToDataArray(data, filterChan);
  }else{
    for(int i=0;i<8;i++){
      artnetChasePatternState[i]=data[filterChan+i];
    }
  }
}

void handle_leds(int currentMode){
  // chasePattern(colorIndex, colorSpread, cycleTime, trailLength, trailSpread, dir, strobe, brightness); // Normalized to 0-255 Range
  if(epdata.control_mode==0){
    if(activePreset.presetType==1){
      POVPlayer(currentMode);
    }else{
      chasePattern(activePreset.ledPresets[0],activePreset.ledPresets[1],activePreset.ledPresets[2],activePreset.ledPresets[3],activePreset.ledPresets[4],activePreset.ledPresets[5],activePreset.ledPresets[6],activePreset.ledPresets[7]);
    }
  }else{
    if(atoi(epdata.fixtureMode)==2){
      chasePattern(artnetChasePatternState[0],artnetChasePatternState[1],artnetChasePatternState[2],artnetChasePatternState[3],artnetChasePatternState[4],artnetChasePatternState[5],artnetChasePatternState[6],artnetChasePatternState[7]);
    }
  }
}
