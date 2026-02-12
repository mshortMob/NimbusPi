#include <EEPROM.h>

bool writeEEPROM=false;

struct {
  char ssid[64] = "NimbusPi-ApcMini"; // This is what gets stored in EEPROM if you uncomment 
  char password[64] = "NimbusPi123"; //  EEPROM.put and EEPROM.commit in setup
  char universe[32] = "1";
  char startChan[64] = "25";
  char fixtureMode[64] = "2";   //fixtureMode 0= 3chan ; fixtureMode 1= indiviudal addressable ; fixtureMode 2= chase patterns
  int num_base_leds = 100;
  int pixel_start_offset = 0;
  char ap_name[64] = "EspTestBed";
  int ledPresets[5][8] = {
    {130,50,200,4,4,3,0,50},
    {50,134,130,16,30,4,0,187},
    {200,2,220,12,1,5,0,55},
    {130,161,200,38,4,3,0,50},
    {50,134,130,16,174,12,0,69}
  };
  int pixelMap[5][8][8] = {
   {{156,156,156,156,156,156,156,138},
    {156,156,156,156,156,156,138,156},
    {156,156,156,156,156,138,156,156},
    {156,156,156,156,28,156,156,156},
    {156,156,156,28,156,156,156,156},
    {156,156,138,156,156,156,156,156},
    {156,138,156,156,156,156,156,156},
    {138,156,156,156,156,156,156,156}},
   {{156,156,156,156,156,156,156,24},
    {156,156,156,156,156,156,156,24},
    {156,101,101,101,101,101,101,156},
    {156,101,156,156,156,156,101,156},
    {156,101,156,156,156,156,101,156},
    {156,101,101,101,101,101,101,156},
    {24,156,156,156,156,156,156,156},
    {24,156,156,156,156,156,156,156}},
   {{156,156,156,156,156,156,156,156},
    {156,156,156,156,156,156,50,156},
    {31,31,31,156,156,156,50,156},
    {156,156,31,156,156,50,50,156},
    {156,156,31,156,156,50,156,156},
    {156,156,156,156,50,50,156,156},
    {156,156,156,156,50,156,156,156},
    {156,156,156,156,156,156,156,156}},
   {{156,156,156,156,156,156,156,24},
    {156,156,156,156,156,156,156,24},
    {156,101,101,101,101,101,101,156},
    {156,101,156,156,156,156,101,156},
    {156,101,156,156,156,156,101,156},
    {156,101,101,101,101,101,101,156},
    {24,156,156,156,156,156,156,156},
    {24,156,156,156,156,156,156,156}},
   {{156,156,156,156,156,156,156,138},
    {156,156,156,156,156,156,138,156},
    {156,156,156,156,156,138,156,156},
    {156,156,156,156,28,156,156,156},
    {156,156,156,28,156,156,156,156},
    {156,156,138,156,156,156,156,156},
    {156,138,156,156,156,156,156,156},
    {138,156,156,156,156,156,156,156}}
  };
  int presetTypes[5]={1,1,1,0,0};
}epdata;

void setup_eeprom(){
  EEPROM.begin(4096);
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
  Serial.print("num_base_leds: ");
  Serial.println(epdata.num_base_leds);
  Serial.print("pixel_start_offset: ");
  Serial.println(epdata.pixel_start_offset);
  Serial.print("ap_name: ");
  Serial.println(epdata.ap_name);
  Serial.println("ledPresets: ");
  for(int x=0;x<5;x++){
    for(int y=0;y<8;y++){
      Serial.print(epdata.ledPresets[x][y]);
      Serial.print(",");
    }
    Serial.println("");
  }
  Serial.println("presetType: ");
  for(int x=0;x<5;x++){
    Serial.print(epdata.presetTypes[x]);
    Serial.print(",");
  }
  Serial.println("");
  Serial.println("pixelMap: ");
  for(int x=0;x<5;x++){
    for(int y=0;y<8;y++){
      for(int z=0;z<8;z++){
        Serial.print(epdata.pixelMap[x][y][z]);
        Serial.print(",");
      }
      Serial.println("");
    }
    Serial.println("-------");
  }
  Serial.println("");
}
