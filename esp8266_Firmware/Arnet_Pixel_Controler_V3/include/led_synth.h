int fixtureNumber=5;

int brightnessCycle=0;
int brightnessArray[9]={100,100,100,100,100,100,100,100,50};
int strobeCycleDebounceTime=1000;
float strobeBuffer[120];
float centerStrobeBuffer=0;
float centerStrobeBuffer2=0;
float desiredBufferR[120];
float desiredBufferG[120];
float desiredBufferB[120];
float pixelBufferR[120];
float pixelBufferG[120];
float pixelBufferB[120];
float filteredBrightness=0.0;
int povPlayerFrame=0;
int povPlayerDebounceTime=0;
int povPlayerCycleTime=5;
int povDirection=-1;

struct {
  int colorIndex = 110;
  int colorSpread = 30;
  int cycleTime = 2000;
  int trailLength = 4;
  int trailSpread = 2;
  int dir = -1;
  int strobe = 0;
  int brightness = 25;
}fixtureValues;

int colorMap[][157] = {{0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,105,110,115,120,125,130,135,140,145,150,155,160,165,170,175,180,185,190,195,200,205,210,215,220,225,230,235,240,245,250,255,255,250,245,240,235,230,225,220,215,210,205,200,195,190,185,180,175,170,165,160,155,150,145,140,135,130,125,120,115,110,105,100,95,90,85,80,75,70,65,60,55,50,45,40,35,30,25,20,15,10,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                       {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,105,110,115,120,125,130,135,140,145,150,155,160,165,170,175,180,185,190,195,200,205,210,215,220,225,230,235,240,245,250,255,255,250,245,240,235,230,225,220,215,210,205,200,195,190,185,180,175,170,165,160,155,150,145,140,135,130,125,120,115,110,105,100,95,90,85,80,75,70,65,60,55,50,45,40,35,30,25,20,15,10,5,0,0},
                       {255,250,245,240,235,230,225,220,215,210,205,200,195,190,185,180,175,170,165,160,155,150,145,140,135,130,125,120,115,110,105,100,95,90,85,80,75,70,65,60,55,50,45,40,35,30,25,20,15,10,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,105,110,115,120,125,130,135,140,145,150,155,160,165,170,175,180,185,190,195,200,205,210,215,220,225,230,235,240,245,250,255,0} };
 
void setDesiredlBuffer(int i, float r, float g, float b){
  desiredBufferR[i]=r;
  desiredBufferG[i]=g;
  desiredBufferB[i]=b;
  
}

void POVPlayer(){
  if( (millis()-povPlayerDebounceTime) > povPlayerCycleTime){
    FastLED.clear();
    for(int i=epdata.pixel_start_offset;i<(epdata.pixel_start_offset+epdata.num_base_leds);i++){
      if((i%16)<8){
        leds[i]=CRGB(colorMap[0][epdata.pixelMap[i%8][povPlayerFrame]], colorMap[1][epdata.pixelMap[i%8][povPlayerFrame]], colorMap[2][epdata.pixelMap[i%8][povPlayerFrame]]);
      }else{
        leds[i]=CRGB(colorMap[0][epdata.pixelMap[7-(i%8)][povPlayerFrame]], colorMap[1][epdata.pixelMap[7-(i%8)][povPlayerFrame]], colorMap[2][epdata.pixelMap[7-(i%8)][povPlayerFrame]]);
      }
    }
    FastLED.setBrightness(50);
    FastLED.show();
    if(povPlayerFrame==7 and povDirection==1){
      povDirection=-1*povDirection;
    }else if(povPlayerFrame==0 and povDirection==-1){
      povDirection=-1*povDirection;
    }
    povPlayerFrame=(povPlayerFrame+povDirection)%8;
    povPlayerDebounceTime=millis();
  }
}

float stepTowardsValue(float current, float desired, float stepSize){
  float nextValue=desired;
  if( (desired-current)>0.0 and abs(desired-current)>3.0 ){
    nextValue=current+((desired-current)/stepSize);
  }else if( (desired-current)<0.0  and abs(desired-current)>3.0 ){
    nextValue=current+((desired-current)/stepSize);
  }else{
    nextValue=desired;
  }
  return nextValue;
}

void setAndFilterPixelBuffer(){
  float tc=1.0;
  float gain=1.0;
  if(floor(fixtureValues.cycleTime)>=188 and floor(fixtureValues.cycleTime)<260){
    tc=(fixtureValues.cycleTime-188)*2.0;
  }
  for(int i =0; i<epdata.num_base_leds; i++){
    float r=stepTowardsValue(pixelBufferR[i],desiredBufferR[i],tc);
    float g=stepTowardsValue(pixelBufferG[i],desiredBufferG[i],tc);
    float b=stepTowardsValue(pixelBufferB[i],desiredBufferB[i],tc);
    pixelBufferR[i]=r;
    pixelBufferG[i]=g;
    pixelBufferB[i]=b;
    
    int centerPixelMapFixture1[][15]={{5,6,7,8,9,10,22,24,25,26,28,29,31,36,36},
                              {44,45,46,47,48,49,50,51,52,53,54,55,56,37,38}};
    int centerPixelMapFixture1Length=15;
    
    int centerPixelMapFixture2[][14]={{4,5,6,7,8,9,10,23,24,25,26,27,28,29},
                         {37,38,39,40,41,42,43,44,45,46,47,48,49,50}};                
    int centerPixelMapFixture2Length=14;
    
    int centerPixelMapFixture3[][15]={{5,6,7,8,9,10,11,23,24,25,26,27,28,29,30},
                         {39,40,41,42,43,44,45,46,47,48,49,50,51,52,53}};
    int centerPixelMapFixture3Length=15;
    
    int centerPixelMapFixture4[][15]={{5,6,7,8,9,10,11,23,24,25,26,27,28,29,30},
                         {39,40,41,42,43,44,45,46,47,48,49,50,51,52,53}};
    int centerPixelMapFixture4Length=15;
    
    // 4-11, 39-45
    // 23-31, 46-53
    bool isCenterPixel=false;
    float centerStrobeModifier=1.0;
    if(fixtureValues.strobe >= 254){
     centerStrobeModifier=1.0;
    }else if(fixtureValues.strobe >= 248){
     centerStrobeModifier=0.0;
    }else if(fixtureValues.strobe >= 240){
     centerStrobeModifier=centerStrobeBuffer;
    }else if(fixtureValues.strobe >= 233){
      centerStrobeModifier=1.0-centerStrobeBuffer;
    }else if(fixtureValues.strobe >= 219){
      centerStrobeModifier=centerStrobeBuffer2;
    }
    if(fixtureNumber==1){
      for(int y=0; y<centerPixelMapFixture1Length; y++){
          if(i==centerPixelMapFixture1[0][y]){
            if( (centerPixelMapFixture1[0][y]!=36) and (((fixtureValues.colorSpread>12) and (fixtureValues.colorSpread<25)) or ((fixtureValues.colorSpread>75) and (fixtureValues.colorSpread<90)) or ((fixtureValues.colorSpread>130) and (fixtureValues.colorSpread<150))) ){
              leds[centerPixelMapFixture1[1][y]]=CRGB( floor(gain*b*centerStrobeModifier), floor(gain*r*centerStrobeModifier), floor(gain*g*centerStrobeModifier) );
            }else{
              leds[centerPixelMapFixture1[1][y]]=CRGB( floor(gain*r*centerStrobeModifier), floor(gain*g*centerStrobeModifier), floor(gain*b*centerStrobeModifier) );
            }
          }
          if(i==centerPixelMapFixture1[1][y] and i!=37 and i!=38){
            isCenterPixel=true;
          }
      }
    }
    if(fixtureNumber==2){
      for(int y=0; y<centerPixelMapFixture2Length; y++){
          if(i==centerPixelMapFixture2[0][y]){
            if( (centerPixelMapFixture2[0][y]!=36) and (((fixtureValues.colorSpread>12) and (fixtureValues.colorSpread<25)) or ((fixtureValues.colorSpread>75) and (fixtureValues.colorSpread<90)) or ((fixtureValues.colorSpread>130) and (fixtureValues.colorSpread<150))) ){
              leds[centerPixelMapFixture2[1][y]]=CRGB( floor(gain*b*centerStrobeModifier), floor(gain*r*centerStrobeModifier), floor(gain*g*centerStrobeModifier) );
            }else{
              leds[centerPixelMapFixture2[1][y]]=CRGB( floor(gain*r*centerStrobeModifier), floor(gain*g*centerStrobeModifier), floor(gain*b*centerStrobeModifier) );
            }
          }
          if(i==centerPixelMapFixture2[1][y]){
            isCenterPixel=true;
          }
      }
    }
    if(fixtureNumber==3){
      for(int y=0; y<centerPixelMapFixture3Length; y++){
          if(i==centerPixelMapFixture3[0][y]){
            if( (true) and (((fixtureValues.colorSpread>12) and (fixtureValues.colorSpread<25)) or ((fixtureValues.colorSpread>75) and (fixtureValues.colorSpread<90)) or ((fixtureValues.colorSpread>130) and (fixtureValues.colorSpread<150))) ){
              leds[centerPixelMapFixture3[1][y]]=CRGB( floor(gain*b*centerStrobeModifier), floor(gain*r*centerStrobeModifier), floor(gain*g*centerStrobeModifier) );
            }else{
              leds[centerPixelMapFixture3[1][y]]=CRGB( floor(gain*r*centerStrobeModifier), floor(gain*g*centerStrobeModifier), floor(gain*b*centerStrobeModifier) );
            }
          }
          if(i==centerPixelMapFixture3[1][y]){
            isCenterPixel=true;
          }
      }
    }
    if(fixtureNumber==4){
      for(int y=0; y<centerPixelMapFixture4Length; y++){
          if(i==centerPixelMapFixture4[0][y]){
            if( (true) and (((fixtureValues.colorSpread>12) and (fixtureValues.colorSpread<25)) or ((fixtureValues.colorSpread>75) and (fixtureValues.colorSpread<90)) or ((fixtureValues.colorSpread>130) and (fixtureValues.colorSpread<150))) ){
              leds[centerPixelMapFixture4[1][y]]=CRGB( floor(gain*b*centerStrobeModifier), floor(gain*r*centerStrobeModifier), floor(gain*g*centerStrobeModifier) );
            }else{
              leds[centerPixelMapFixture4[1][y]]=CRGB( floor(gain*r*centerStrobeModifier), floor(gain*g*centerStrobeModifier), floor(gain*b*centerStrobeModifier) );
            }
          }
          if(i==centerPixelMapFixture4[1][y]){
            isCenterPixel=true;
          }
      }
    }
    if(!isCenterPixel){
      if( (i+epdata.pixel_start_offset) < (NUM_LEDS) ){
        if( fixtureValues.strobe>=200 and fixtureValues.strobe<240 ){
          leds[i+epdata.pixel_start_offset]=CRGB( floor(gain*r*centerStrobeBuffer), floor(gain*g*centerStrobeBuffer), floor(gain*b*centerStrobeBuffer) );
        }else if( fixtureValues.strobe>=248 and fixtureValues.strobe<254 ){
          leds[i+epdata.pixel_start_offset]=CRGB( floor(gain*r), floor(gain*g), floor(gain*b) );
        }else if( fixtureValues.strobe>=254 and fixtureValues.strobe<260 ){
          leds[i+epdata.pixel_start_offset]=CRGB( 0, 0, 0 );
        }else if( fixtureValues.strobe>=240 and fixtureValues.strobe<248 ){
          leds[i+epdata.pixel_start_offset]=CRGB( 0, 0, 0 );
        }else{
          leds[i+epdata.pixel_start_offset]=CRGB( floor(gain*r), floor(gain*g), floor(gain*b) );
        }
      }
    }
  }
}

void chasePatternCore(int colorIndex, int colorSpread, int cycleTime, int trailLength, int trailSpread, int dir, int strobe, int brightness){
//  FastLED.clear();
  for(int i =0; i<epdata.num_base_leds; i++){
    setDesiredlBuffer(i,0,0,0);
  }
  int offsetRotation=17;
  
  int colorLFOCycleTimeInt=20000;
  if(fixtureValues.colorIndex>=200){
    colorLFOCycleTimeInt=23000-floor(1+fixtureValues.colorIndex-200)*400;
  }
  float colorLFOCycle=millis()%colorLFOCycleTimeInt;
  int colorIndexWithLFOModifier=colorIndex;
  if(fixtureValues.colorIndex>=200){
    colorIndexWithLFOModifier=colorIndex+floor(colorLFOCycle/colorLFOCycleTimeInt*150.0);
  }
    
  float cycle=millis()%cycleTime;
  int index=floor((cycle/cycleTime)*epdata.num_base_leds);
  if(dir==1){
    for(int i =0; i<trailLength; i++){
      int tempModulus=floor(epdata.num_base_leds/1);
      int ind=(index+i*trailSpread)%tempModulus;
      setDesiredlBuffer(ind, colorMap[0][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(i*colorSpread))%156] );
    }
  }else if(dir==2){
    for(int i =0; i<trailLength; i++){
      int tempModulus=floor(epdata.num_base_leds/1);
      int ind=(index+i*trailSpread)%tempModulus;
      setDesiredlBuffer(epdata.num_base_leds-1-(ind), colorMap[0][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(i*colorSpread))%156] );
    }  
  }else if(dir==3){
    offsetRotation=17;
    for(int i =0; i<trailLength; i++){
      int tempModulus=floor(epdata.num_base_leds/2);
      int ind=(index+i*trailSpread)%tempModulus;
      int temp=i;
      setDesiredlBuffer((ind+offsetRotation)%epdata.num_base_leds, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
      setDesiredlBuffer((epdata.num_base_leds-1-ind+offsetRotation)%epdata.num_base_leds, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
    }
  }else if(dir==4){
    offsetRotation=35;
    for(int i =0; i<trailLength; i++){
      int tempModulus=floor(epdata.num_base_leds/2);
      int ind=(index+i*trailSpread)%tempModulus;
      int temp=i;
      setDesiredlBuffer((epdata.num_base_leds-1-ind+offsetRotation)%epdata.num_base_leds, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
      setDesiredlBuffer((ind+offsetRotation)%epdata.num_base_leds, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
    }
  }else if(dir==5){
    offsetRotation=17;
    for(int i =0; i<trailLength; i++){
      int ind=(index+i*trailSpread)%(epdata.num_base_leds);
      int temp=i;
      setDesiredlBuffer((ind+offsetRotation)%epdata.num_base_leds, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
      setDesiredlBuffer((epdata.num_base_leds-1-ind+offsetRotation)%epdata.num_base_leds, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
    }
  }else if(dir==6){
    offsetRotation=6;
    for(int i =0; i<trailLength; i++){
      int ind=(index+i*trailSpread)%(epdata.num_base_leds/1);
      int temp=i;
      setDesiredlBuffer((ind+offsetRotation)%epdata.num_base_leds, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
      setDesiredlBuffer((epdata.num_base_leds-1-ind+offsetRotation)%epdata.num_base_leds, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
    }
  }else if(dir==7){
    offsetRotation=6;
    for(int i =0; i<trailLength; i++){
      int ind=(index+i*trailSpread)%(epdata.num_base_leds/2);
      int temp=i;
      setDesiredlBuffer((ind+offsetRotation)%epdata.num_base_leds, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
      setDesiredlBuffer((epdata.num_base_leds-1-ind+offsetRotation)%epdata.num_base_leds, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
    }
  }else if(dir==8){
    offsetRotation=25;
    for(int i =0; i<trailLength; i++){
      int ind=(index+i*trailSpread)%(epdata.num_base_leds/2);
      int temp=i;
      setDesiredlBuffer((ind+offsetRotation)%epdata.num_base_leds, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
      setDesiredlBuffer((epdata.num_base_leds-1-ind+offsetRotation)%epdata.num_base_leds, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
    }
  }else if(dir==9){
    offsetRotation=35;
    for(int i =0; i<trailLength; i++){
      int ind=(index+i*trailSpread)%(epdata.num_base_leds);
      setDesiredlBuffer((ind+offsetRotation)%epdata.num_base_leds, colorMap[0][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(i*colorSpread))%156] );
      setDesiredlBuffer(epdata.num_base_leds-1-ind+offsetRotation, colorMap[0][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(i*colorSpread))%156] );
      setDesiredlBuffer( (epdata.num_base_leds/4+(ind+offsetRotation))%epdata.num_base_leds, colorMap[0][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(i*colorSpread))%156] );
      setDesiredlBuffer( (-epdata.num_base_leds/4+(epdata.num_base_leds-1-(ind)+offsetRotation))%epdata.num_base_leds, colorMap[0][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(i*colorSpread))%156] );
    }   
  }else{
    for(int i =0; i<trailLength; i++){
      int tempModulus=floor(epdata.num_base_leds/1);
      int ind=(index+i*trailSpread)%tempModulus;
      setDesiredlBuffer(ind, colorMap[0][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(i*colorSpread))%156] );
    }
  }

  int strobeFreq=100;
  int strobeTrailLength=4;
  if( (strobe>=0 and strobe<20) or (strobe>=85 and strobe<105) or (strobe>=170 and strobe<190) ){
    strobeTrailLength=3;
  }
  if( (strobe>=20 and strobe<40) or (strobe>=105 and strobe<125)  or (strobe>=190 and strobe<210) ){
    strobeTrailLength=4;
  }
  if( (strobe>=40 and strobe<60) or (strobe>=125 and strobe<145)  or (strobe>=210 and strobe<230) ){
    strobeTrailLength=5;
  }
  if( (strobe>=60 and strobe<85) or (strobe>=145 and strobe<170)  or (strobe>=230 and strobe<=255) ){
    strobeTrailLength=6;
  }
  if(strobe >= 240){
   strobeFreq=floor(10+((strobe%85)*.001));
  }else if(strobe >= 200){
   strobeFreq=floor(10+((strobe%85)*.03));
  }else{
   strobeFreq=floor(10+((strobe%85)*.8));    
  }
  strobeCycleDebounceTime=(strobeCycleDebounceTime+1)%strobeFreq;
  if(strobe==0 or strobe >= 200){
    for(int i=0;i<epdata.num_base_leds;i++){
      strobeBuffer[i]==0.0;
    }   
    if(strobeCycleDebounceTime==0){
      centerStrobeBuffer=random(0,2);
      centerStrobeBuffer2=random(0,2);
    }  
  }else{
    if(strobeCycleDebounceTime==0){
      for(int i=0;i<epdata.num_base_leds;i++){
        strobeBuffer[i]=0;
      }
      for(int i =0; i<strobeTrailLength; i++){
        strobeBuffer[random(0,epdata.num_base_leds)]=1;
      }
    }
    if(strobe<=85){
      for(int i=0;i<epdata.num_base_leds;i++){
        if(strobeBuffer[i]==1.0){
          setDesiredlBuffer(i, 255,255,255 );
        }
      }
    }
   if(strobe>85 and strobe<170){
    for(int i=0;i<epdata.num_base_leds;i++){
      if(strobeBuffer[i]==0.0){
        setDesiredlBuffer(i, 0,0,0 );
      }
    }
   }
   if(strobe>=170){
    for(int i=0;i<epdata.num_base_leds;i++){
      if(strobeBuffer[i]==0.0){
        setDesiredlBuffer(i, 0,0,0 );
      }
      if(strobeBuffer[i]==1.0){
        setDesiredlBuffer(i, 255,255,255 );
      }
    }
   }
  }
  setAndFilterPixelBuffer();
  float tc=1.0;
  float gain=1.0;
  if(floor(fixtureValues.cycleTime)>=150 and floor(fixtureValues.cycleTime)<260){
    tc=(fixtureValues.cycleTime-150)*6.0;
  }
  filteredBrightness=stepTowardsValue(filteredBrightness,brightness,tc);
  FastLED.setBrightness(filteredBrightness);
  FastLED.show();
}

void chasePattern(int colorIndex, int colorSpread, int cycleTime, int trailLength, int trailSpread, int dir, int strobe, int brightness){
  
  fixtureValues.colorIndex=colorIndex;
  fixtureValues.colorSpread=colorSpread;
  fixtureValues.cycleTime=cycleTime;
  fixtureValues.trailLength=trailLength;
  fixtureValues.trailSpread=trailSpread;
  fixtureValues.dir=dir;
  fixtureValues.strobe=strobe;
  fixtureValues.brightness=brightness;
      
  colorIndex=floor((colorIndex/1.3));
  if(floor(colorIndex)<200){
    colorIndex=floor((colorIndex/1.3));
  }
  colorSpread=floor((colorSpread/255.0)*10.0);
  cycleTime=210+floor((cycleTime/255.0)*4200.0);
  trailLength=1+floor((trailLength/255.0)*(epdata.num_base_leds/.75));
  trailSpread=1.0+floor((trailSpread/255.0)*epdata.num_base_leds*.25);
  if(dir>240){
    dir=9;
  } else if(dir>210){
    dir=8;
  } else if(dir>180){
    dir=7;
  } else if(dir>150){
    dir=6;
  } else if(dir>120){
    dir=5;
  } else if(dir>90){
    dir=4;
  } else if(dir>60){
    dir=3;
  } else if(dir>30){
    dir=2;
  }
  chasePatternCore(colorIndex, colorSpread, cycleTime, trailLength, trailSpread, dir, strobe, brightness);
}
