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

void setLEDSToASingleColor(int r, int g, int b){
  for(int i=0;i<NUM_LEDS;i++){
    leds[i]=CRGB(r,g,b);
  }
  FastLED.show();
}

void handle_leds(int currentMode){
  // chasePattern(colorIndex, colorSpread, cycleTime, trailLength, trailSpread, dir, strobe, brightness); // Normalized to 0-255 Range
  if(epdata.presetTypes[currentMode]==1){
    POVPlayer(currentMode);
  }else{
    chasePattern(epdata.ledPresets[currentMode][0],epdata.ledPresets[currentMode][1],epdata.ledPresets[currentMode][2],epdata.ledPresets[currentMode][3],epdata.ledPresets[currentMode][4],epdata.ledPresets[currentMode][5],epdata.ledPresets[currentMode][6],epdata.ledPresets[currentMode][7]);
  }
}
