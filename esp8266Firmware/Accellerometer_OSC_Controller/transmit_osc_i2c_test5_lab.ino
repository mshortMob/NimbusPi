#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <Adafruit_NeoPixel.h>
#include <ArtnetWifi.h>
#include<Wire.h>

const char* ssid = "wifi_network";
const char* pass = "wifi_password";

const int MPU=0x68; 
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
WiFiUDP Udp;                                // A UDP instance to let us send and receive packets over UDP
const IPAddress outIp(192,168,2,59);        // remote IP of your computer
const unsigned int outPort = 10001;          // remote port to receive OSC
const unsigned int localPort = 8888;        // local port to listen for OSC packets (actually not used for sending)
Adafruit_NeoPixel strip(16, 15, NEO_GRB + NEO_KHZ800);
ArtnetWifi artnet;
int smode=0;
int previousSwitchValue=1;

void setup() {
    artnet.begin();
    artnet.setArtDmxCallback(onDmxFrame);
    //pinMode(12, INPUT_PULLUP); 
    pinMode(13, INPUT_PULLUP); 
    digitalWrite(13, HIGH); 
    previousSwitchValue=digitalRead(13);
    Serial.begin(115200);

    // Connect to WiFi network
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");

    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    Serial.println("Starting UDP");
    Udp.begin(localPort);
    Serial.print("Local port: ");
#ifdef ESP32
    Serial.println(localPort);
#else
    Serial.println(Udp.localPort());
#endif
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)

  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B); 
  Wire.write(0);    
  Wire.endTransmission(true);

}


void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data){
  //Serial.println("got dmx");
  for(int i=0; i<16; i++) {
    strip.setPixelColor(i, strip.Color( data[(i*3)+0], data[(i*3)+1], data[(i*3)+2] ));
  }
  strip.show();
}

void loop() {
  if(previousSwitchValue!=digitalRead(13)){
    previousSwitchValue=digitalRead(13);
    smode=(smode+1)%3;
    Serial.print("Mode: ");
    Serial.println(smode);
    oscSend("/mode",smode);
  }
  
  float valx = getAccelData("x");
  Serial.print("valx: ");
  Serial.print(valx);
  Serial.print("  ");
  valx=(valx+16000)/100000*3.0;
  Serial.print(valx);

  float valy = getAccelData("y");
  Serial.print("  valy: ");
  Serial.print(valy);
  Serial.print("  ");
  valy=(valy+16000)/100000*3.0;
  Serial.print(valy);

  float valz = getAccelData("z");
  Serial.print("  valz: ");
  Serial.print(valz);
  Serial.print("  ");
  valz=(valz+16000)/100000*3.0;
  Serial.println(valz);

  if(smode==0){
    oscSend("/x2",valx);
    oscSend("/y2",valy);
    oscSend("/z2",valz);
  }
  if(smode==1){
    oscSend("/x2",valx);
    oscSend("/y2",valy);
    oscSend("/z2",valz);
  }
  if(smode==2){
    oscSend("/x2",valx);
    oscSend("/y2",valy);
    oscSend("/z2",valz);
  }
  delay(50);
  artnet.read();
}

void oscSend(char* chan, float val){
  OSCMessage msg(chan);
  msg.add(val);
  Udp.beginPacket(outIp, outPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();
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
  
//  Serial.print("Accelerometer: ");
//  Serial.print("X = "); Serial.print(AcX);
//  Serial.print(" | Y = "); Serial.print(AcY);
//  Serial.print(" | Z = "); Serial.println(AcZ); 
//  
//  Serial.print("Gyroscope: ");
//  Serial.print("X = "); Serial.print(GyX);
//  Serial.print(" | Y = "); Serial.print(GyY);
//  Serial.print(" | Z = "); Serial.println(GyZ);
//  Serial.println(" ");

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
