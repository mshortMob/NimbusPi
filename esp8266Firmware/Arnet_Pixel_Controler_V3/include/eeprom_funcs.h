#include <EEPROM.h>

// Bumped whenever ConfigSettings's layout changes - removing
// ledPresets/pixelMap/presetTypes (now on the filesystem, see
// preset_store.h) shifted every field after them, so a device still
// running the old layout needs to be force-reset rather than booting with
// misread values. Bumped again for the OSC/accelerometer fields below -
// appended at the end so it's not strictly required this time, but a fresh,
// deterministic default (rather than whatever garbage happens to be in that
// previously-unused EEPROM range) is worth it for fields that control
// active network transmission (send_osc/osc_ip/osc_port).
int eeprom_signature=123458;
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
  int enable_led_yield = 1; // enable_led_yield 1=Enabled ; 0=Disabled
  char lan_peers[8][64] = {"","","","","","","",""}; // manually-entered LAN peer hostnames/IPs for standalone-mode targeted control
  int accelerometer_mode = 1; // accelerometer_mode 0=Disabled ; 1=Enabled (ESP-sourced) ; 2=Mobile (browser-sourced)
  int send_osc = 0; // send_osc 0=Disabled ; 1=Enabled - only meaningful when accelerometer_mode==1
  char osc_ip[32] = "192.168.1.100";
  int osc_port = 9000;
  char osc_path_x[32] = "/accel/x";
  char osc_path_y[32] = "/accel/y";
  char osc_path_z[32] = "/accel/z";
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
  Serial.print("enable_led_yield: ");
  Serial.println(epdata.enable_led_yield);
  Serial.print("accelerometer_mode: ");
  Serial.println(epdata.accelerometer_mode);
  Serial.print("send_osc: ");
  Serial.println(epdata.send_osc);
  Serial.print("osc_ip: ");
  Serial.println(epdata.osc_ip);
  Serial.print("osc_port: ");
  Serial.println(epdata.osc_port);
  Serial.print("osc_path_x: ");
  Serial.println(epdata.osc_path_x);
  Serial.print("osc_path_y: ");
  Serial.println(epdata.osc_path_y);
  Serial.print("osc_path_z: ");
  Serial.println(epdata.osc_path_z);
  sanitize_lan_peers();
  Serial.println("lan_peers: ");
  for(int x=0;x<8;x++){
    Serial.println(epdata.lan_peers[x]);
  }
  Serial.println("");
  // ledPresets/pixelMap/presetTypes now live in the presets file (see
  // preset_store.h), not EEPROM.
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
