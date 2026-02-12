#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include <ArtnetWifi.h>
#include<Wire.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>

int led_pin=15;
int mode_button_pin=13;
int num_leds=84;
int num_leds_base=53;
int fixtureNumber=1;
bool writeEEPROM=false;
const char *ssid = "MandalaCircle1"; //HotSpot Password, use web UI to setup local wifi
const char *password = "NimbusPi123"; // must be different than your local wifi

struct {
  char ssid[256] = "NimbusPi-ApcMini"; // This is what gets stored in EEPROM if you uncomment 
  char password[256] = "NimbusPi123"; //  EEPROM.put and EEPROM.commit in setup
  char universe[16] = "1";
  char startChan[32] = "1";
  char fixtureMode[16] = "2";   //fixtureMode 0= 3chan ; fixtureMode 1= indiviudal addressable
  char oscAddressX[512] = "/accell/x";
  char oscAddressY[512] = "/accell/y";
  char oscAddressZ[512] = "/accell/z";
  char oscTargetIP[256] = "192.168.4.1";  
  char oscPort[64] = "10001";
}epdata;

Adafruit_NeoPixel leds = Adafruit_NeoPixel(num_leds, led_pin, NEO_GRB + NEO_KHZ800);
const unsigned int localPort = 8889;        // local port to listen for OSC packets (actually not used for sending)
const int MPU=0x68; 
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
WiFiUDP Udp;                                // A UDP instance to let us send and receive packets over UDP
bool startupMode=true;
bool wifiConnected=false;
ESP8266WebServer server(80);
ArtnetWifi artnet;
char fullResponsePayload[7000];
int previousSwitchValue=1;
int pattern = 0;
int numPatterns=5;
int frame=0;
int brightnessCycle=0;
int autoPattern=0;
int autoPatternTimer=0;
int pixel_offset_correction = 0;
float valx;
float valy;
float valz;
float lastx=0.0;
float lasty=0.0;
float lastz=0.0;
float diffx=0.0;
float diffy=0.0;
float diffz=0.0;
int noisePatternMode=0;
int brightnessArray[9]={100,100,100,100,100,100,100,100,50};
bool POVMode=false;
float emblemBlinderDebounceTime=0;
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
long rainbowFirstPixelHue=0;

struct {
  int colorIndex = 110;
  int colorSpread = 30;
  int cycleTime = 100;
  int trailLength = 30;
  int trailSpread = 10;
  int dir = -1;
  int strobe = 0;
  int brightness = 25;
}fixtureValues;

int colorMap[][156] = {{0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,105,110,115,120,125,130,135,140,145,150,155,160,165,170,175,180,185,190,195,200,205,210,215,220,225,230,235,240,245,250,255,255,250,245,240,235,230,225,220,215,210,205,200,195,190,185,180,175,170,165,160,155,150,145,140,135,130,125,120,115,110,105,100,95,90,85,80,75,70,65,60,55,50,45,40,35,30,25,20,15,10,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                       {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,105,110,115,120,125,130,135,140,145,150,155,160,165,170,175,180,185,190,195,200,205,210,215,220,225,230,235,240,245,250,255,255,250,245,240,235,230,225,220,215,210,205,200,195,190,185,180,175,170,165,160,155,150,145,140,135,130,125,120,115,110,105,100,95,90,85,80,75,70,65,60,55,50,45,40,35,30,25,20,15,10,5,0},
                       {255,250,245,240,235,230,225,220,215,210,205,200,195,190,185,180,175,170,165,160,155,150,145,140,135,130,125,120,115,110,105,100,95,90,85,80,75,70,65,60,55,50,45,40,35,30,25,20,15,10,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,105,110,115,120,125,130,135,140,145,150,155,160,165,170,175,180,185,190,195,200,205,210,215,220,225,230,235,240,245,250,255} };

void setup() {
  Serial.begin(115200);
  pinMode(led_pin, OUTPUT);
  pinMode(mode_button_pin, INPUT_PULLUP); 
  digitalWrite(mode_button_pin, HIGH); 
  previousSwitchValue=digitalRead(mode_button_pin);
  Serial.println("");
  Serial.println("");
  EEPROM.begin(2944);
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B); 
  Wire.write(0);    
  Wire.endTransmission(true);
  leds.begin();
  leds.clear();
  leds.show();


//  leds.setBrightness(25);
//  for(int x=0; x<49; x++){
//    Serial.println(x);
//    leds.setPixelColor(x, 255, 0, 0);
//  }
//  for(int x=49; x<57; x++){
//    Serial.println(x);
//    leds.setPixelColor(x, 0, 255, 0);
//  }
//  for(int x=57; x<num_leds; x++){
//    Serial.println(x);
//    leds.setPixelColor(x, 0, 0, 255);
//  }
//  leds.show();
//  delay(300000);

  
  delay(1000);
  if(writeEEPROM){
    EEPROM.put(0,epdata); //usefull if you want to flash it with the correct wifi data already
    EEPROM.commit();
  }
  EEPROM.get(0,epdata);
  delay(1000);
  Serial.println("EEPROM READ:");
  Serial.print("ssid: ");
  Serial.println(epdata.ssid);
  Serial.print("password: ");
  Serial.println(epdata.password);
  Serial.print("universe: ");
  Serial.println(epdata.universe);
  Serial.print("startChan: ");
  Serial.println(epdata.startChan);
  Serial.print("fixtureMode: ");
  Serial.println(epdata.fixtureMode);
  Serial.print("oscAddressX: ");
  Serial.println(epdata.oscAddressX);
  Serial.print("oscAddressY: ");
  Serial.println(epdata.oscAddressY);
  Serial.print("oscAddressZ: ");
  Serial.println(epdata.oscAddressZ);
  Serial.print("oscTargetIP: ");
  Serial.println(epdata.oscTargetIP);
  Serial.print("oscPort: ");
  Serial.println(epdata.oscPort);
  getResponsePayoad();
  wifiConnected = connectToSavedWifi();
  if(wifiConnected){
    startupMode=false;
    artnet.begin();
    artnet.setArtDmxCallback(onDmxFrame);
  }else{
    startStandaloneHotspot();
  }
  for(int i=0;i<120;i++){
    strobeBuffer[i]=0;
  }
}

void loop() {
  if(previousSwitchValue!=digitalRead(mode_button_pin)){
    previousSwitchValue=digitalRead(mode_button_pin);
    if(digitalRead(mode_button_pin)==0){
      if(POVMode==true){
        pattern=(pattern+1)%numPatterns;
        Serial.print("pattern:");
        Serial.println(pattern);
      }
      if(POVMode==false and startupMode==false and wifiConnected==true ){
        startupMode=false;
        POVMode=true;      
        wifiConnected=false;
      }
      if(startupMode==true){
        startupMode=false;
        POVMode=true;
        Serial.println("User button press, switching to POV mode. ");
          for(int i=0;i<num_leds;i++){
            leds.setPixelColor(i, 0, 0, 0);  
          }
          leds.setBrightness(2);
          leds.show();
      }
    }
  }
  if(startupMode){
    pattern=2;
    drawPOVFrame(); 
    server.handleClient(); 
  }else {
    if(POVMode==true){
      drawPOVFrame();
    }else{
      if(!wifiConnected){
        stopStandaloneHotspot();
      }else{
        artnet.read();
        int fixtureMode=atoi(epdata.fixtureMode);
        if(fixtureMode==2){
          int colorIndex=floor((fixtureValues.colorIndex/1.3));
          if(floor(fixtureValues.colorIndex)<200){
            colorIndex=floor((fixtureValues.colorIndex/1.3));
          }else{
            colorIndex=153;
          }
          int colorSpread=floor((fixtureValues.colorSpread/255.0)*10.0);
          int cycleTime=210+floor((fixtureValues.cycleTime/255.0)*4200.0);
          int trailLength=1+floor((fixtureValues.trailLength/255.0)*(num_leds/.75));
          int trailSpread=1.0+floor((fixtureValues.trailSpread/255.0)*num_leds*.25);
          int dir=1;
          if(fixtureValues.dir>240){
            dir=9;
          } else if(fixtureValues.dir>210){
            dir=8;
          } else if(fixtureValues.dir>180){
            dir=7;
          } else if(fixtureValues.dir>150){
            dir=6;
          } else if(fixtureValues.dir>120){
            dir=5;
          } else if(fixtureValues.dir>90){
            dir=4;
          } else if(fixtureValues.dir>60){
            dir=3;
          } else if(fixtureValues.dir>30){
            dir=2;
          }
          chasePattern1(colorIndex, colorSpread, cycleTime, trailLength, trailSpread, dir, fixtureValues.strobe, fixtureValues.brightness);
        }
      } 
    }
  }
//  if(wifiConnected){
//    scrapeAccellerometer(false, true);
//  }else{
//    scrapeAccellerometer(false, false);   
//  }
}

void drawPOVFrame() {
  if(pattern==0){
    noisePattern5();
  }else if(pattern==1){
    noisePattern6();
  }else if(pattern==2){
    fixtureValues.colorSpread=15;
    fixtureValues.colorIndex=206;
    chasePattern1(75, 1, 3000, 10, 1, 4, 0, 75);
  }else if(pattern==3){
    rainbow(5);
  }else if(pattern==4){
    int cycleTime=40000;
    float cycle=millis()%cycleTime;
    int colorIndex=floor((cycle/cycleTime)*150);
    chasePattern1(colorIndex, 8, 3250, 10, 1, 4, 0, 255);
  }
  frame=(frame+1)%16;
}

void setDesiredlBuffer(int i, float r, float g, float b){
  desiredBufferR[i]=r;
  desiredBufferG[i]=g;
  desiredBufferB[i]=b;
  
}

void setAndFilterPixelBuffer(){
  float tc=1.0;
  float gain=1.0;
  if(floor(fixtureValues.cycleTime)>=188 and floor(fixtureValues.cycleTime)<260){
    tc=(fixtureValues.cycleTime-188)*2.0;
  }
  for(int i =0; i<num_leds; i++){
    float r=stepTowardsValue(pixelBufferR[i],desiredBufferR[i],tc);
    float g=stepTowardsValue(pixelBufferG[i],desiredBufferG[i],tc);
    float b=stepTowardsValue(pixelBufferB[i],desiredBufferB[i],tc);
    pixelBufferR[i]=r;
    pixelBufferG[i]=g;
    pixelBufferB[i]=b;
    
//    int centerPixelMapFixture1[][27]={{12,13,15,18,20,23,26,25,31,34,36,39,42,45,47,49,52,55,58,60,62,64,66,1,4,8,11},
//                                      {57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83}};
//    int centerPixelMapFixture1Length=27;
    
    int centerPixelMapFixture1[][26]={{32,33,34,35,36,37,38,39,40,41,42,43,44,45, 7, 8, 9,10,11,12,13,14,15,16,17,18},
                                      {64,65,66,67,68,69,70,71,72,73,74,75,76,78,79,80,81,82,83,57,58,59,60,61,62,63,}};
    int centerPixelMapFixture1Length=26;           
    
    int centerPixelMapFixture2[][14]={{4,5,6,7,8,9,10,23,24,25,26,27,28,29},
                         {15,19,23,27,31,35,39,43,47,51,53,58,62,65}};                
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
            if( (((fixtureValues.colorSpread>12) and (fixtureValues.colorSpread<25)) or ((fixtureValues.colorSpread>75) and (fixtureValues.colorSpread<90)) or ((fixtureValues.colorSpread>130) and (fixtureValues.colorSpread<150))) ){
              leds.setPixelColor(centerPixelMapFixture1[1][y],leds.Color( floor(gain*b*centerStrobeModifier), floor(gain*r*centerStrobeModifier), floor(gain*g*centerStrobeModifier) ));
            }else{
              leds.setPixelColor(centerPixelMapFixture1[1][y],leds.Color( floor(gain*r*centerStrobeModifier), floor(gain*g*centerStrobeModifier), floor(gain*b*centerStrobeModifier) ));
            }
          }
          if(i==centerPixelMapFixture1[1][y]){
            isCenterPixel=true;
          }
      }
    }
    if(fixtureNumber==2){
      for(int y=0; y<centerPixelMapFixture2Length; y++){
          if(i==centerPixelMapFixture2[0][y]){
            if( (centerPixelMapFixture2[0][y]!=36) and (((fixtureValues.colorSpread>12) and (fixtureValues.colorSpread<25)) or ((fixtureValues.colorSpread>75) and (fixtureValues.colorSpread<90)) or ((fixtureValues.colorSpread>130) and (fixtureValues.colorSpread<150))) ){
              leds.setPixelColor(centerPixelMapFixture2[1][y],leds.Color( floor(gain*b*centerStrobeModifier), floor(gain*r*centerStrobeModifier), floor(gain*g*centerStrobeModifier) ));
            }else{
              leds.setPixelColor(centerPixelMapFixture2[1][y],leds.Color( floor(gain*r*centerStrobeModifier), floor(gain*g*centerStrobeModifier), floor(gain*b*centerStrobeModifier) ));
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
              leds.setPixelColor(centerPixelMapFixture3[1][y],leds.Color( floor(gain*b*centerStrobeModifier), floor(gain*r*centerStrobeModifier), floor(gain*g*centerStrobeModifier) ));
            }else{
              leds.setPixelColor(centerPixelMapFixture3[1][y],leds.Color( floor(gain*r*centerStrobeModifier), floor(gain*g*centerStrobeModifier), floor(gain*b*centerStrobeModifier) ));
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
              leds.setPixelColor(centerPixelMapFixture4[1][y],leds.Color( floor(gain*b*centerStrobeModifier), floor(gain*r*centerStrobeModifier), floor(gain*g*centerStrobeModifier) ));
            }else{
              leds.setPixelColor(centerPixelMapFixture4[1][y],leds.Color( floor(gain*r*centerStrobeModifier), floor(gain*g*centerStrobeModifier), floor(gain*b*centerStrobeModifier) ));
            }
          }
          if(i==centerPixelMapFixture4[1][y]){
            isCenterPixel=true;
          }
      }
    }
    if(!isCenterPixel){
      if( fixtureValues.strobe>=200 and fixtureValues.strobe<240 ){
        leds.setPixelColor(i,leds.Color( floor(gain*r*centerStrobeBuffer), floor(gain*g*centerStrobeBuffer), floor(gain*b*centerStrobeBuffer) ));
      }else if( fixtureValues.strobe>=248 and fixtureValues.strobe<254 ){
        leds.setPixelColor(i,leds.Color( floor(gain*r), floor(gain*g), floor(gain*b) ));
      }else if( fixtureValues.strobe>=254 and fixtureValues.strobe<260 ){
        leds.setPixelColor(i,leds.Color( 0, 0, 0 ));
      }else if( fixtureValues.strobe>=240 and fixtureValues.strobe<248 ){
        leds.setPixelColor(i,leds.Color( 0, 0, 0 ));
      }else{
        leds.setPixelColor(i,leds.Color( floor(gain*r), floor(gain*g), floor(gain*b) ));
      }
    }
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

void chasePattern1(int colorIndex, int colorSpread, int cycleTime, int trailLength, int trailSpread, int dir, int strobe, int brightness){
//  leds.clear();
  for(int i =0; i<num_leds; i++){
    setDesiredlBuffer(i,0,0,0);
  }
  int offsetRotation=25;
  
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
  int index=floor((cycle/cycleTime)*num_leds_base);
  if(dir==1){
    for(int i =0; i<trailLength; i++){
      int tempModulus=floor(num_leds_base/1);
      int ind=(index+i*trailSpread)%tempModulus;
      setDesiredlBuffer(ind, colorMap[0][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(i*colorSpread))%156] );
    }
  }else if(dir==2){
    for(int i =0; i<trailLength; i++){
      int tempModulus=floor(num_leds_base/1);
      int ind=(index+i*trailSpread)%tempModulus;
      setDesiredlBuffer(num_leds_base-1-(ind), colorMap[0][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(i*colorSpread))%156] );
    }  
  }else if(dir==3){
    offsetRotation=25;
    for(int i =0; i<trailLength; i++){
      int tempModulus=floor(num_leds_base/2);
      int ind=(index+i*trailSpread)%tempModulus;
      int temp=i;
      setDesiredlBuffer((ind+offsetRotation)%num_leds_base, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
      setDesiredlBuffer((num_leds_base-1-ind+offsetRotation)%num_leds_base, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
    }
  }else if(dir==4){
    offsetRotation=25;
    for(int i =0; i<trailLength; i++){
      int tempModulus=floor(num_leds_base/2);
      int ind=(index+i*trailSpread)%tempModulus;
      int temp=i;
      setDesiredlBuffer((num_leds_base-1-ind+offsetRotation)%num_leds_base, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
      setDesiredlBuffer((ind+offsetRotation)%num_leds_base, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
    }
  }else if(dir==5){
    offsetRotation=25;
    for(int i =0; i<trailLength; i++){
      int ind=(index+i*trailSpread)%(num_leds_base);
      int temp=i;
      setDesiredlBuffer((ind+offsetRotation)%num_leds_base, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
      setDesiredlBuffer((num_leds_base-1-ind+offsetRotation)%num_leds_base, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
    }
  }else if(dir==6){
    offsetRotation=13;
    for(int i =0; i<trailLength; i++){
      int ind=(index+i*trailSpread)%(num_leds_base/1);
      int temp=i;
      setDesiredlBuffer((ind+offsetRotation)%num_leds_base, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
      setDesiredlBuffer((num_leds_base-1-ind+offsetRotation)%num_leds_base, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
    }
  }else if(dir==7){
    offsetRotation=13;
    for(int i =0; i<trailLength; i++){
      int ind=(index+i*trailSpread)%(num_leds_base/2);
      int temp=i;
      setDesiredlBuffer((ind+offsetRotation)%num_leds_base, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
      setDesiredlBuffer((num_leds_base-1-ind+offsetRotation)%num_leds_base, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
    }
  }else if(dir==8){
    offsetRotation=25;
    for(int i =0; i<trailLength; i++){
      int ind=(index+i*trailSpread)%(num_leds_base/2);
      int temp=i;
      setDesiredlBuffer((ind+offsetRotation)%num_leds_base, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
      setDesiredlBuffer((num_leds_base-1-ind+offsetRotation)%num_leds_base, colorMap[0][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(temp*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(temp*colorSpread))%156] );
    }
  }else if(dir==9){
    offsetRotation=25;
    for(int i =0; i<trailLength; i++){
      int ind=(index+i*trailSpread)%(num_leds_base);
      setDesiredlBuffer((ind+offsetRotation)%num_leds_base, colorMap[0][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(i*colorSpread))%156] );
      setDesiredlBuffer(num_leds_base-1-ind+offsetRotation, colorMap[0][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(i*colorSpread))%156] );
      setDesiredlBuffer( (num_leds_base/4+(ind+offsetRotation))%num_leds_base, colorMap[0][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(i*colorSpread))%156] );
      setDesiredlBuffer( (-num_leds_base/4+(num_leds_base-1-(ind)+offsetRotation))%num_leds_base, colorMap[0][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[1][(colorIndexWithLFOModifier+(i*colorSpread))%156], colorMap[2][(colorIndexWithLFOModifier+(i*colorSpread))%156] );
    }   
  }else{
    for(int i =0; i<trailLength; i++){
      int tempModulus=floor(num_leds_base/1);
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
    for(int i=0;i<num_leds_base;i++){
      strobeBuffer[i]==0.0;
    }   
    if(strobeCycleDebounceTime==0){
      centerStrobeBuffer=random(0,2);
      centerStrobeBuffer2=random(0,2);
    }  
  }else{
    if(strobeCycleDebounceTime==0){
      for(int i=0;i<num_leds_base;i++){
        strobeBuffer[i]=0;
      }
      for(int i =0; i<strobeTrailLength; i++){
        strobeBuffer[random(0,num_leds_base)]=1;
      }
    }
    if(strobe<=85){
      for(int i=0;i<num_leds_base;i++){
        if(strobeBuffer[i]==1.0){
          setDesiredlBuffer(i, 255,255,255 );
        }
      }
    }
   if(strobe>85 and strobe<170){
    for(int i=0;i<num_leds_base;i++){
      if(strobeBuffer[i]==0.0){
        setDesiredlBuffer(i, 0,0,0 );
      }
    }
   }
   if(strobe>=170){
    for(int i=0;i<num_leds_base;i++){
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
  leds.setBrightness(filteredBrightness);
  leds.show();
}

void noisePattern5(void){
  if( abs(emblemBlinderDebounceTime-millis())>2000 ){
     emblemBlinderDebounceTime=millis();
    noisePatternMode=(noisePatternMode+1)%2;
  }
  leds.clear();
  if(noisePatternMode==0){
    for(int i=0; i<4; i++) {
      float tempRandom=floor(random(0,num_leds_base));
      leds.setPixelColor(tempRandom, leds.Color( random(0,255), 0, 255 ) );
      if(tempRandom>11){
        leds.setPixelColor(num_leds_base-tempRandom, leds.Color( random(0,255), 0, 255 ) );
      }
    }
    for(int i=0; i<1; i++) {
      float tempRandom=floor(random(0,num_leds_base));
      leds.setPixelColor(tempRandom, leds.Color( 255, 255, 255 ) );
    }
    leds.setBrightness(100);
    leds.show();
    delay(110);
   }else{
    for(int i=0; i<4; i++) {
      float tempRandom=floor(random(0,num_leds_base));
      leds.setPixelColor(tempRandom, leds.Color( 0, 255, random(0,144) ) );
    }
    for(int i=0; i<1; i++) {
      float tempRandom=floor(random(0,num_leds_base));
      leds.setPixelColor(tempRandom, leds.Color( 255, 255, 255 ) );
    }
    leds.setBrightness(100);
    leds.show();
    delay(90);
  }
}

void noisePattern6(void){
  leds.clear();
  for(int i=0; i<4; i++) {
    float tempRandom=floor(random(0,num_leds));
    leds.setPixelColor(tempRandom, leds.Color( 255, random(0,144), 0 ) );
  }
  for(int i=0; i<1; i++) {
    float tempRandom=floor(random(0,num_leds));
    leds.setPixelColor(tempRandom, leds.Color( 255, 255, 255 ) );
  }
  leds.setBrightness(100);
  leds.show();
  delay(92);
}

void rainbow(int wait) {
  leds.setBrightness(100);
  rainbowFirstPixelHue=(rainbowFirstPixelHue+256)%(5*65536);
  for(int i=0; i<leds.numPixels(); i++) { // For each pixel in leds...
    int pixelHue = rainbowFirstPixelHue + (i * 65536L / leds.numPixels());
    leds.setPixelColor(i, leds.gamma32(leds.ColorHSV(pixelHue)));
  }
  leds.show(); // Update strip with new contents
  if(digitalRead(mode_button_pin)!=previousSwitchValue){
    if(digitalRead(13)==0){
      Serial.println("Change Pattern!");
      pattern=(pattern+1)%numPatterns;
      previousSwitchValue=digitalRead(mode_button_pin);
      leds.setBrightness(brightnessArray[pattern]);
    }
  }
  delay(wait);  // Pause for a moment
}

void scrapeAccellerometer(bool logging, bool oscEnable){
  valx=applyMaxAndMinLimits((getAccelData("x")+16000)/100000*3.0);
  valy=applyMaxAndMinLimits((getAccelData("y")+16000)/100000*3.0);
  valz=applyMaxAndMinLimits((getAccelData("z")+16000)/100000*3.0);
  
  diffx=lastx-valx;
  diffy=lasty-valy;
  diffz=lastz-valz;
  if(diffx<0){
    diffx=diffx*-1.0;
  }
  if(diffy<0){
    diffy=diffy*-1.0;
  }
  if(diffz<0){
    diffz=diffz*-1.0;
  }
  lastx=valx;
  lasty=valy;
  lastz=valz;

  if(logging==true){
    Serial.print(valx);
    Serial.print("  ");
    Serial.print(valy);
    Serial.print("  ");
    Serial.println(valz);
  }
  if(oscEnable==true){
    oscSend(epdata.oscAddressX, valx);
    oscSend(epdata.oscAddressY, valy);
    oscSend(epdata.oscAddressZ, valz);
  }
}

void stopStandaloneHotspot(void){
      Serial.println("Stopping Web Server");
      server.close();
      server.stop();
      WiFi.softAPdisconnect(true);
      wifiConnected = connectToSavedWifi();
      if(!wifiConnected){
        startStandaloneHotspot();
      }
}

void startStandaloneHotspot(void){
        startupMode=true;
        Serial.println("Entering Startup Mode(WiFi Host: 192.168.4.1/data/)");
        leds.clear();
        for(int i=0;i<num_leds;i++){
          leds.setPixelColor(i, 0, 0, 255);
        }
        Serial.println();
        Serial.print("Center LED is: ");
        Serial.println(floor(num_leds/2));
        Serial.println();
        leds.setBrightness(25);
        leds.show();
        WiFi.softAP(ssid, password);
        IPAddress myIP = WiFi.softAPIP();
        server.on("/data/", HTTP_GET, adminUI); // when the server receives a request with /data/ in the string then run the handleSentVar function
        server.begin(); 
}

boolean connectToSavedWifi(void){
  for(int i=0;i<num_leds;i++){
    leds.setPixelColor(i, 255, 0, 0);  
  }
  leds.setBrightness(25); 
  leds.show();  
  
  boolean state = true;
  int i = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(epdata.ssid, epdata.password);
  Serial.println("");
  Serial.println("Connecting to WiFi");
  Serial.print("ssid:");
  Serial.println(epdata.ssid);
  Serial.print("password:");
  Serial.println(epdata.password);
  // Wait for connection
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  
    if (i > 20){
      state = false;
      break;
    }

    if(previousSwitchValue!=digitalRead(13)){
      previousSwitchValue=digitalRead(13);
      if(POVMode==true){
        pattern=(pattern+1)%numPatterns;
        Serial.print("pattern:");
        Serial.println(pattern);
      }
      if(startupMode==true){
        startupMode=false;
        POVMode=true;
        Serial.println("User button press, switching to POV mode. ");
          for(int i=0;i<num_leds;i++){
            leds.setPixelColor(i, 0, 0, 0);  
          }
          leds.setBrightness(2);
          leds.show();
      }
      state = false;
      break;
    }
    
    i++;
  }
  if (state){
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(epdata.ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    for(int i=0;i<num_leds;i++){
      leds.setPixelColor(i, 0, 255, 0);  
    }
    leds.setBrightness(2); 
    leds.show(); 
  } else {
    Serial.println("");
    Serial.println("Connection failed.");
  }
  
  return state;
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data){
  int filterUniverse=atoi(epdata.universe);
  int filterChan=atoi(epdata.startChan)-1;
  int fixtureMode=atoi(epdata.fixtureMode);
  //fixtureMode 0= 3chan
  //fixtureMode 1= indiviudal addressable
  Serial.print("Filtering for universe:");
  Serial.println(filterUniverse);
  if(universe==filterUniverse){
    Serial.print("Received artnet data for universe:");
    Serial.println(universe);
    if(fixtureMode==2){
      fixtureValues.colorIndex=data[filterChan+0];
      fixtureValues.colorSpread=data[filterChan+1];
      fixtureValues.cycleTime=data[filterChan+2];
      fixtureValues.trailLength=data[filterChan+3];
      fixtureValues.trailSpread=data[filterChan+4];
      fixtureValues.dir=data[filterChan+5];
      fixtureValues.strobe=data[filterChan+6];
      fixtureValues.brightness=data[filterChan+7];
    }else if(fixtureMode==1){
      for(int i=0;i<num_leds;i++){
        leds.setPixelColor(i, data[filterChan+0+(i*3)], data[filterChan+1+(i*3)], data[filterChan+2+(i*3)]);  
      }
      leds.setBrightness(255); 
      leds.show(); 
    }else{
      for(int i=0;i<num_leds;i++){
        leds.setPixelColor(i, data[filterChan+0], data[filterChan+1], data[filterChan+2]);  
      }
      leds.setBrightness(255); 
      leds.show(); 
    }
  }
}

void adminUI() {
    if (server.hasArg("ssid")) { // this is the variable sent from the client
      Serial.println("");
      Serial.println("User submitted form with settings:");
      Serial.print("ssid:");
      Serial.println(server.arg("ssid"));    }
      server.arg("ssid").toCharArray(epdata.ssid,200);
    if (server.hasArg("password")) { // this is the variable sent from the client
      Serial.print("password:");
      Serial.println(server.arg("password"));
      server.arg("password").toCharArray(epdata.password,200);
    }
    if (server.hasArg("universe")) { // this is the variable sent from the client
      Serial.print("universe:");
      Serial.println(server.arg("universe"));
      server.arg("universe").toCharArray(epdata.universe,16);
    }
    if (server.hasArg("startChan")) { // this is the variable sent from the client
      Serial.print("startChan:");
      Serial.println(server.arg("startChan"));
      server.arg("startChan").toCharArray(epdata.startChan,32);
    }
    if (server.hasArg("fixtureMode")) { // this is the variable sent from the client
      Serial.print("fixtureMode:");
      Serial.println(server.arg("fixtureMode"));
      server.arg("fixtureMode").toCharArray(epdata.fixtureMode,16);
    }
    if (server.hasArg("oscAddressX")) { // this is the variable sent from the client
      Serial.print("oscAddressX:");
      Serial.println(server.arg("oscAddressX"));
      server.arg("oscAddressX").toCharArray(epdata.oscAddressX,400);
    }
    if (server.hasArg("oscAddressY")) { // this is the variable sent from the client
      Serial.print("oscAddressY:");
      Serial.println(server.arg("oscAddressY"));
      server.arg("oscAddressY").toCharArray(epdata.oscAddressY,400);
    }
    if (server.hasArg("oscAddressZ")) { // this is the variable sent from the client
      Serial.print("oscAddressZ:");
      Serial.println(server.arg("oscAddressZ"));
      server.arg("oscAddressZ").toCharArray(epdata.oscAddressZ,400);
    }
    if (server.hasArg("oscTargetIP")) { // this is the variable sent from the client
      Serial.print("oscTargetIP:");
      Serial.println(server.arg("oscTargetIP"));
      server.arg("oscTargetIP").toCharArray(epdata.oscTargetIP,200);
    }
    if (server.hasArg("oscPort")) { // this is the variable sent from the client
      Serial.print("oscPort:");
      Serial.println(server.arg("oscPort"));
      server.arg("oscPort").toCharArray(epdata.oscPort,50);
    }
    if (server.hasArg("oscPort")) { 
      startupMode=false;
      EEPROM.put(0,epdata);
      EEPROM.commit();
      Serial.println("");
      Serial.println("Settings Saved");
      Serial.println("");
      server.send(200, "text/html", "Settings Saved!");   
    }else{
      Serial.println("");
      Serial.println("Page Load");
      Serial.println("");
      server.send(200, "text/html", fullResponsePayload);   
    } 
}

char* getResponsePayoad(void){
  char responseString1[]="<html><head><title>FlowMapper</title><style>body{margin: 0px;background-color: grey;}input{width: 60%;height: 5.5%;left: 18%;margin-top: 1%;position: absolute;}button{background-color:lightgrey;width: 60%; height: 10%;left: 18.0%;             margin-top: 2.5%;position: absolute;}label {width: 10%;                 height: 10%;left: 6%;margin-top: 1.5%;position: absolute;vertical-align: middle;}body{ padding-top: 1.5%;}</style></head><body cz-shortcut-listen='true'><label for='SSID'>SSID:</label><input type='text' id='SSID'><br><br><label for='password'>Password:</label><input type='text' id='password'><br><br><label for='universe'>Universe</label><input type='text' id='universe'><a style='top:3.5%;position:relative;left:80%;font-size:.75em;'>Note:indexes at 0</a><br><br><label for='startChannel'>Start Channel:</label><input type='text' id='startChannel'><a style='top:3.5%;position:relative;left:80%;font-size:.75em;'>Note:indexes at 1</a><br><br><label for='fixtureMode'>Fixture Mode:</label><input type='text' id='fixtureMode'><a style='top:3.5%;position:relative;left:80%;font-size:.75em;'>0=3 Channel<br> 1=Individual Addressable</a><br>2=DMX Fixture<br><br>";
  char responseString2[]="<label for='oscAddressX'>oscAddressX:</label><input type='text' id='oscAddressX'><br><br>";
  char responseString3[]="<label for='oscAddressY'>oscAddressY:</label><input type='text' id='oscAddressY'><br><br>";
  char responseString4[]="<label for='oscAddressZ'>oscAddressZ:</label><input type='text' id='oscAddressZ'><br><br>";
  char responseString5[]="<label for='oscTargetIP'>oscTargetIP:</label><input type='text' id='oscTargetIP'><br><br>";
  char responseString6[]="<label for='oscPort'>oscPort:</label><input type='text' id='oscPort'><br><br>";  
  char responseString7[]="<button onclick='submitForm();'>Save</button><script>document.getElementById('SSID').value = '";
  char responseString8[]="';document.getElementById('password').value='";
  char responseString9[]="';document.getElementById('universe').value='";
  char responseString10[]="';document.getElementById('startChannel').value='";
  char responseString11[]="';document.getElementById('fixtureMode').value='";
  char responseString12[]="';document.getElementById('oscAddressX').value='";
  char responseString13[]="';document.getElementById('oscAddressY').value='";
  char responseString14[]="';document.getElementById('oscAddressZ').value='";
  char responseString15[]="';document.getElementById('oscTargetIP').value='";
  char responseString16[]="';document.getElementById('oscPort').value='";
  char responseString17[]="'; function submitForm(){payload='http://'+location.host + '/data/?ssid=' + document.getElementById('SSID').value + '&password=' + document.getElementById('password').value + '&universe=' + document.getElementById('universe').value + '&startChan=' + document.getElementById('startChannel').value + '&fixtureMode=' + document.getElementById('fixtureMode').value + '&oscAddressX=' + document.getElementById('oscAddressX').value + '&oscAddressY=' + document.getElementById('oscAddressY').value + '&oscAddressZ=' + document.getElementById('oscAddressZ').value + '&oscTargetIP=' + document.getElementById('oscTargetIP').value + '&oscPort=' + document.getElementById('oscPort').value; var xhttp = new XMLHttpRequest(); xhttp.onreadystatechange = function() { if(this.readyState == 4 && this.status == 200){console.log(this.responseText);}};xhttp.open('GET', payload, true);xhttp.send();}</script></body></html>";
  strcat(fullResponsePayload,responseString1);
  strcat(fullResponsePayload,responseString2);
  strcat(fullResponsePayload,responseString3);
  strcat(fullResponsePayload,responseString4);
  strcat(fullResponsePayload,responseString5);
  strcat(fullResponsePayload,responseString6);
  strcat(fullResponsePayload,responseString7);
  strcat(fullResponsePayload,epdata.ssid);
  strcat(fullResponsePayload,responseString8);
  strcat(fullResponsePayload,epdata.password);
  strcat(fullResponsePayload,responseString9);
  strcat(fullResponsePayload,epdata.universe);
  strcat(fullResponsePayload,responseString10);
  strcat(fullResponsePayload,epdata.startChan);
  strcat(fullResponsePayload,responseString11);
  strcat(fullResponsePayload,epdata.fixtureMode);
  strcat(fullResponsePayload,responseString12);
  strcat(fullResponsePayload,epdata.oscAddressX);
  strcat(fullResponsePayload,responseString13);
  strcat(fullResponsePayload,epdata.oscAddressY);
  strcat(fullResponsePayload,responseString14);
  strcat(fullResponsePayload,epdata.oscAddressZ);
  strcat(fullResponsePayload,responseString15);
  strcat(fullResponsePayload,epdata.oscTargetIP);
  strcat(fullResponsePayload,responseString16);
  strcat(fullResponsePayload,epdata.oscPort);
  strcat(fullResponsePayload,responseString17);

  Serial.println(fullResponsePayload);
  return fullResponsePayload;
}

void oscSend(char* chan, float val){
//  OSCMessage msg(chan);
//  msg.add(val);
//  Udp.beginPacket(epdata.oscTargetIP, atoi(epdata.oscPort) );
//  msg.send(Udp);
//  Udp.endPacket();
//  msg.empty();
}

float applyMaxAndMinLimits(float in){
  float out=in;
  if(out<0.0){
    out=0.0;
  }
  if(out>1){
    out=1.0;
  }
  return out;
}

float getAccelData(char* chan){
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);  
  Wire.endTransmission(false);
  Wire.requestFrom(MPU,12,true);  
  AcX=Wire.read()<<8|Wire.read();    
  AcY=Wire.read()<<8|Wire.read();  
  AcZ=Wire.read()<<8|Wire.read(); 
  Tmp=Wire.read()<<8|Wire.read();  
  GyX=Wire.read()<<8|Wire.read();  
  GyY=Wire.read()<<8|Wire.read();  
  GyZ=Wire.read()<<8|Wire.read();  

  if(chan=="x"){
    return AcX;
  }
  if(chan=="y"){
    return AcY;
  }
  if(chan=="z"){
    return AcZ;
  }else{
    return AcZ;
  }
}
