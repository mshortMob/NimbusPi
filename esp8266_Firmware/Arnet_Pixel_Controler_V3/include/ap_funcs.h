#include <ESP8266WiFi.h>
#include <WiFiClient.h>

const char *ssid = "EspTestBed";
const char *password = "mshort123";

bool wifiConnected=false;

void startStandaloneHotspot(){
  Serial.println("Configuring access point...");
  setLEDSToASingleColor(0,0,25);
  WiFi.mode(WIFI_AP);
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  Serial.println(WiFi.localIP());  
}

boolean connectToSavedWifi(char* savedWifiName, char* savedWifiPassword){
  setLEDSToASingleColor(25,0,0);
  boolean state = true;
  int i = 0;
  WiFi.mode(WIFI_STA);
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

bool setup_ap(char* savedWifiName, char* savedWifiPassword){
  wifiConnected=connectToSavedWifi(savedWifiName, savedWifiPassword);
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
      setup_ap(epdata.ssid, epdata.password);
}

bool handle_ap(bool wifiConnected, bool buttonWasPressed ){
  if(wifiConnected && buttonWasPressed){
    wifiConnected=false;
    startStandaloneHotspot();
  }
  return wifiConnected;
}
