#include<Wire.h>

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

void handle_accelerometer() {
  scrapeAccellerometer(false);
}
