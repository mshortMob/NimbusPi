#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>

const char *password = "mshort123";

bool wifiConnected=false;
bool hasDisplayedEepromResetFlash=false;

void startStandaloneHotspot(){
  Serial.println("Configuring access point...");
  setLEDSToASingleColor(0,0,25);
  WiFi.mode(WIFI_AP);
  WiFi.hostname(epdata.ap_name); 
  Serial.print("SSID: ");
  Serial.println(epdata.ap_name);
  Serial.print("Password: ");
  Serial.println(password);
  WiFi.softAP(epdata.ap_name, password);
  IPAddress myIP = WiFi.softAPIP();
  if (MDNS.begin(epdata.ap_name)) {
    Serial.println("mDNS responder started");
  }
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  Serial.println(WiFi.localIP());
}

boolean connectToSavedWifi(char* savedWifiName, char* savedWifiPassword, char* savedAPName){
  if(!hasDisplayedEepromResetFlash && eeprom_was_reset_on_startup ){
    hasDisplayedEepromResetFlash=true;
    setLEDSToASingleColor(3,0,25);
    delay(2000);
  }
  setLEDSToASingleColor(25,0,0);
  boolean state = true;
  int i = 0;
  WiFi.mode(WIFI_STA);
  WiFi.hostname(savedAPName); 
  WiFi.begin(savedWifiName, savedWifiPassword);
  Serial.println("");
  Serial.println("Connecting to WiFi");
  Serial.print("ssid:");
  Serial.println(savedWifiName);
  Serial.print("password:");
  Serial.println(savedWifiPassword);
  // Wait for connection
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 25 | interupt_startup_routine() ){
      state = false;
      break;
    }
    i++;
  }
  if (state){
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(savedWifiName);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Connection failed.");
    Serial.println("");
  }
  
  return state;
}

bool setup_ap(char* savedWifiName, char* savedWifiPassword, char* savedAPName){
  wifiConnected=connectToSavedWifi(savedWifiName, savedWifiPassword, savedAPName);
  if(!wifiConnected){
    startStandaloneHotspot();
  }else{
    setup_artnet();
    clear_leds();
  }
  return wifiConnected;
}

void stopStandaloneHotspot(void){
      Serial.println("Stopping Standalone Hotspot");
      WiFi.softAPdisconnect(true);
      setup_ap(epdata.ssid, epdata.password, epdata.ap_name);
}

bool handle_ap(bool wifiConnected, bool buttonWasPressed ){
  if(wifiConnected && buttonWasPressed){
    wifiConnected=false;
    startStandaloneHotspot();
  }
  MDNS.update();
  return wifiConnected;
}
