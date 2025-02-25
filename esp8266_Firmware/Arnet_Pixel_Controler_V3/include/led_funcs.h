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
  if(currentMode==0){ chasePattern(0,50,200,4,1,-1,0,55); }
  if(currentMode==1){ chasePattern(50,5,100,16,30,150,0,55); }
  if(currentMode==2){ chasePattern(100,2,70,8,2,255,0,55); }
  if(currentMode==3){ chasePattern(200,150,180,12,50,0,0,55); }
}
