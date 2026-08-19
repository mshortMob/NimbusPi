#include<Wire.h>
#include <WiFiUdp.h>

const int MPU=0x68;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
float valx;
float valy;
float valz;
float lastx=0.0;
float lasty=0.0;
float lastz=0.0;
float diffx=0.0;
float diffy=0.0;
float diffz=0.0;

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

void scrapeAccellerometer(bool logging){
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
}

void setup_accelerometer() {
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

WiFiUDP oscUdp;
unsigned long lastOscSendTime=0;
#define OSC_SEND_INTERVAL_MS 100

// Builds and sends a minimal single-float OSC 1.0 message (address string +
// ",f" type tag + a big-endian float, each part null-padded to a 4-byte
// boundary per the spec). Hand-rolled rather than pulling in an OSC
// library - the message shape needed here (one address, one float
// argument) is small and fixed enough that a dependency wasn't worth it.
void send_osc_float(const char* path, float value){
  if(path==NULL || path[0]=='\0'){
    return;
  }
  IPAddress ip;
  if(!ip.fromString(epdata.osc_ip)){
    return;
  }
  int addrLen=strlen(path)+1; // +1 for the null terminator
  int addrPad=(4-(addrLen%4))%4;
  int totalAddrLen=addrLen+addrPad;
  const char* typeTag=",f";
  int typeLen=3; // ",f" + null terminator
  int typePad=(4-(typeLen%4))%4;
  int totalTypeLen=typeLen+typePad;
  int totalLen=totalAddrLen+totalTypeLen+4;
  if(totalLen>64){
    return; // paths this long aren't expected here - bail rather than overflow buf
  }
  uint8_t buf[64];
  memset(buf,0,sizeof(buf));
  memcpy(buf, path, strlen(path));
  memcpy(buf+totalAddrLen, typeTag, 2);
  uint8_t floatBytes[4];
  memcpy(floatBytes, &value, 4); // Xtensa is little-endian; OSC wants big-endian
  buf[totalAddrLen+totalTypeLen+0]=floatBytes[3];
  buf[totalAddrLen+totalTypeLen+1]=floatBytes[2];
  buf[totalAddrLen+totalTypeLen+2]=floatBytes[1];
  buf[totalAddrLen+totalTypeLen+3]=floatBytes[0];
  oscUdp.beginPacket(ip, epdata.osc_port);
  oscUdp.write(buf, totalLen);
  oscUdp.endPacket();
}

// Throttled independently of the accelerometer read itself (which runs
// unthrottled every loop() pass) - sending a UDP packet on every single
// pass would flood the network for no real benefit at real loop() rates.
void send_accelerometer_osc(void){
  if(epdata.send_osc!=1){
    return;
  }
  unsigned long now=millis();
  if(now-lastOscSendTime<OSC_SEND_INTERVAL_MS){
    return;
  }
  lastOscSendTime=now;
  send_osc_float(epdata.osc_path_x, valx);
  send_osc_float(epdata.osc_path_y, valy);
  send_osc_float(epdata.osc_path_z, valz);
}

void handle_accelerometer() {
  if(epdata.accelerometer_mode!=1){
    return; // Disabled or Mobile - no ESP-side reading/computation either way
  }
  scrapeAccellerometer(false);
  send_accelerometer_osc();
}
