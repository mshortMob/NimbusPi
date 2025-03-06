#include <EEPROM.h>

bool writeEEPROM=false;

struct {
  char ssid[64] = "NimbusPi-ApcMini"; // This is what gets stored in EEPROM if you uncomment 
  char password[64] = "NimbusPi123"; //  EEPROM.put and EEPROM.commit in setup
  char universe[32] = "1";
  char startChan[64] = "25";
  char fixtureMode[64] = "2";   //fixtureMode 0= 3chan ; fixtureMode 1= indiviudal addressable
  char oscAddressX[64] = "/accell/x";
  char oscAddressY[64] = "/accell/y";
  char oscAddressZ[64] = "/accell/z";
  char oscTargetIP[64] = "192.168.4.1";  
  char oscPort[64] = "10001";
  int ledPresets[4][8] = {
    {130,50,200,4,4,3,0,100},
    {50,150,130,16,30,4,0,5},
    {200,2,220,12,1,5,0,55},
    {88,88,88,200,50,6,0,55}
  };
 
}epdata;

void setup_eeprom(){
  EEPROM.begin(1024);
  delay(1000);
  if(writeEEPROM){
    Serial.println("Initializing - i.e. over-writing - EEPROM now");
    EEPROM.put(0,epdata); //usefull if you want to flash it with the correct wifi data already
    EEPROM.commit();
  }
  EEPROM.get(0,epdata);
  delay(1000);
  Serial.println("");
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
  Serial.println("ledPresets: ");
  for(int x=0;x<4;x++){
    for(int y=0;y<8;y++){
      Serial.print(epdata.ledPresets[x][y]);
      Serial.print(",");
    }
    Serial.println("");
  }
  Serial.println("");
}
