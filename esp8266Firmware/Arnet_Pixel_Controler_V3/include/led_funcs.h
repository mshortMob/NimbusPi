#include <FastLED.h>
#define NUM_LEDS 128
#define LED_PIN 15
CRGB leds[NUM_LEDS];
#include <led_synth.h>

void setup_leds(void){
  pinMode(LED_PIN, OUTPUT);
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  FastLED.clear();
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

void handle_leds(int currentMode, bool wifiConnected){
  // chasePattern(colorIndex, colorSpread, cycleTime, trailLength, trailSpread, dir, strobe, brightness); // Normalized to 0-255 Range
  if(!wifiConnected){
    if(epdata.presetTypes[currentMode]==1){
      POVPlayer(currentMode);
    }else{
      chasePattern(epdata.ledPresets[currentMode][0],epdata.ledPresets[currentMode][1],epdata.ledPresets[currentMode][2],epdata.ledPresets[currentMode][3],epdata.ledPresets[currentMode][4],epdata.ledPresets[currentMode][5],epdata.ledPresets[currentMode][6],epdata.ledPresets[currentMode][7]);
    }
  }else{
    if(atoi(epdata.fixtureMode)==2){
      chasePattern(artnetChasePatternState[0],artnetChasePatternState[1],artnetChasePatternState[2],artnetChasePatternState[3],artnetChasePatternState[4],artnetChasePatternState[5],artnetChasePatternState[6],artnetChasePatternState[7]);
    }
  }
}
