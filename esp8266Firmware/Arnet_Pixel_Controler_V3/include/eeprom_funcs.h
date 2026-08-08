#include <EEPROM.h>

int eeprom_signature=123456;
bool eeprom_was_reset_on_startup=false;

struct ConfigSettings {
  char ssid[64] = "NimbusPi-ApcMini";
  char password[64] = "NimbusPi123";
  char universe[32] = "1";
  char startChan[64] = "25";
  char fixtureMode[64] = "2"; // fixtureMode 0=3chan ; fixtureMode 1=indiviudal addressable ; fixtureMode 2=chase patterns
  int num_base_leds = 100;
  int pixel_start_offset = 0;
  char ap_name[64] = "EspTestBed";
  int ssid_timeout_seconds = 30;
  char ap_password[64] = "EspTestBed";
  int control_mode = 0; // control_mode 0=GUI Presets ; 1=ArtNet/DMX
  int max_brightness_percent = 100;
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
  int network_mode = 0; // network_mode 0=Standalone ; 1=Mesh Fleet
  char mesh_name[64] = "NimbusMesh";
  char mesh_password[64] = "NimbusMesh123";
  int mesh_leader = 0; // mesh_leader 0=Pure Node ; 1=Leader (sets mesh root for faster/more reliable formation)
  int enable_led_yield = 1; // enable_led_yield 1=Enabled ; 0=Disabled
  char lan_peers[8][64] = {"","","","","","","",""}; // manually-entered LAN peer hostnames/IPs for standalone-mode targeted control
};

ConfigSettings epdata;
ConfigSettings initial_epdata;

struct {
  int hasInitializedEeprom;
}eptemp;

// lan_peers was appended after other fields ever got their signature-reset
// treatment on already-deployed devices, so a stale EEPROM region can hand
// back unterminated garbage instead of the empty-string default. Force
// null-termination and blank out anything that isn't printable ASCII so a
// garbage entry reads as "empty" rather than corrupting the peers list.
void sanitize_lan_peers(void){
  for(int x=0;x<8;x++){
    epdata.lan_peers[x][63]='\0';
    int len=strnlen(epdata.lan_peers[x],64);
    bool looksValid=true;
    for(int c=0;c<len;c++){
      char ch=epdata.lan_peers[x][c];
      if(ch<32 || ch>126){
        looksValid=false;
        break;
      }
    }
    if(!looksValid){
      epdata.lan_peers[x][0]='\0';
    }
  }
}

void eeprom_load_all(void){
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
  Serial.print("ssid_timeout_seconds: ");
  Serial.println(epdata.ssid_timeout_seconds);
  Serial.print("ap_password: ");
  Serial.println(epdata.ap_password);
  Serial.print("control_mode: ");
  Serial.println(epdata.control_mode);
  Serial.print("max_brightness_percent: ");
  Serial.println(epdata.max_brightness_percent);
  Serial.print("network_mode: ");
  Serial.println(epdata.network_mode);
  Serial.print("mesh_name: ");
  Serial.println(epdata.mesh_name);
  Serial.print("mesh_password: ");
  Serial.println(epdata.mesh_password);
  Serial.print("mesh_leader: ");
  Serial.println(epdata.mesh_leader);
  Serial.print("enable_led_yield: ");
  Serial.println(epdata.enable_led_yield);
  sanitize_lan_peers();
  Serial.println("lan_peers: ");
  for(int x=0;x<8;x++){
    Serial.println(epdata.lan_peers[x]);
  }
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

void reset_eeprom_to_initial_values(void){
    Serial.println("Initializing - i.e. over-writing - EEPROM now");
    Serial.println();  
    EEPROM.put(0,initial_epdata);
    EEPROM.commit();
    eeprom_was_reset_on_startup=true;
    eeprom_load_all();
}

bool need_eeprom_reset(void){
  bool result=false;
  Serial.println("Checking if EEPROM needs to be initialized...");
  EEPROM.get(4064,eptemp);
  delay(100);
  Serial.println("hasInitializedEeprom:");
  Serial.println(eptemp.hasInitializedEeprom);
  Serial.println();
  if(eptemp.hasInitializedEeprom!=eeprom_signature){
    result=true;
    eptemp.hasInitializedEeprom=eeprom_signature;
    EEPROM.put(4064,eptemp);
    EEPROM.commit();
  }
  return result;
}

void setup_eeprom(){
  EEPROM.begin(4096);
  delay(1000);
  if(need_eeprom_reset()){
    reset_eeprom_to_initial_values();
  }else{
    eeprom_load_all();
  }
}
