#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include <ArtnetWifi.h>

int led_pin=15;
int num_leds=32;
bool writeEEPROM=false;

const char *ssid = "featherwingSTW"; //HotSpot Password, use web UI to setup local wifi
const char *password = "mshort123"; // must be different than your local wifi

struct {
  char ssid[256] = "NimbusPi"; // This is what gets stored in EEPROM if you uncomment 
  char password[256] = "NimbusPi123"; //  EEPROM.put and EEPROM.commit in setup
  char universe[16] = "1";
  char startChan[32] = "1";
  char fixtureMode[16] = "0";   //fixtureMode 0= 3chan ; fixtureMode 1= indiviudal addressable
}epdata;

Adafruit_NeoPixel leds = Adafruit_NeoPixel(num_leds, led_pin, NEO_GRB + NEO_KHZ800);
bool startupMode=true;
bool wifiConnected=false;
ESP8266WebServer server(80);
ArtnetWifi artnet;
char fullResponsePayload[7000];


void setup() {
  Serial.begin(115200);
  pinMode(led_pin, OUTPUT); 
  Serial.println("");
  Serial.println("");
  EEPROM.begin(576);
  leds.begin();
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
  getResponsePayoad();
  wifiConnected = connectToSavedWifi();
  if(wifiConnected){
    startupMode=false;
    artnet.begin();
    artnet.setArtDmxCallback(onDmxFrame);
  }else{
    startStandaloneHotspot();
  }
}

void loop() {
  if(startupMode){
    server.handleClient();
  }else{
    if(!wifiConnected){
      stopStandaloneHotspot();
    }else{
      artnet.read();      
    } 
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
        for(int i=0;i<num_leds;i++){
          leds.setPixelColor(i, 0, 0, 255);  
        }
        leds.setBrightness(2);
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
  leds.setBrightness(2); 
  leds.show();  
  
  boolean state = true;
  int i = 0;

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
    if(fixtureMode==1){
      for(int i=0;i<num_leds;i++){
        leds.setPixelColor(i, data[filterChan+0+(i*3)], data[filterChan+1+(i*3)], data[filterChan+2+(i*3)]);  
      }
    }else{
      for(int i=0;i<num_leds;i++){
        leds.setPixelColor(i, data[filterChan+0], data[filterChan+1], data[filterChan+2]);  
      }
    }
    leds.setBrightness(255); 
    leds.show(); 
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
    if (server.hasArg("startChan")) { 
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
  char responseString1[]="<html><head><title>FlowMapper</title><style>body{margin: 0px;background-color: grey;}input{width: 60%;height: 5.5%;left: 18%;margin-top: 1%;position: absolute;}button{background-color:lightgrey;width: 60%; height: 10%;left: 18.0%;             margin-top: 2.5%;position: absolute;}label {width: 10%;                 height: 10%;left: 6%;margin-top: 1.5%;position: absolute;vertical-align: middle;}body{ padding-top: 1.5%;}</style></head><body cz-shortcut-listen='true'><label for='SSID'>SSID:</label><input type='text' id='SSID'><br><br><label for='password'>Password:</label><input type='text' id='password'><br><br><label for='universe'>Universe</label><input type='text' id='universe'><a style='top:3.5%;position:relative;left:80%;font-size:.75em;'>Note:indexes at 0</a><br><br><label for='startChannel'>Start Channel:</label><input type='text' id='startChannel'><a style='top:3.5%;position:relative;left:80%;font-size:.75em;'>Note:indexes at 1</a><br><br><label for='fixtureMode'>Fixture Mode:</label><input type='text' id='fixtureMode'><a style='top:3.5%;position:relative;left:80%;font-size:.75em;'>0=3 Channel<br> 1=Individual Addressable</a><br><br><button onclick='submitForm();'>Save</button><script>document.getElementById('SSID').value = '";
  char responseString2[]="';document.getElementById('password').value='";
  char responseString3[]="';document.getElementById('universe').value='";
  char responseString4[]="';document.getElementById('startChannel').value='";
  char responseString5[]="';document.getElementById('fixtureMode').value='";
  char responseString6[]="'; function submitForm(){payload='http://'+location.host + '/data/?ssid=' + document.getElementById('SSID').value + '&password=' + document.getElementById('password').value + '&universe=' + document.getElementById('universe').value + '&startChan=' + document.getElementById('startChannel').value + '&fixtureMode=' + document.getElementById('fixtureMode').value;var xhttp = new XMLHttpRequest(); xhttp.onreadystatechange = function() { if(this.readyState == 4 && this.status == 200){console.log(this.responseText);}};xhttp.open('GET', payload, true);xhttp.send();}</script></body></html>";
  strcat(fullResponsePayload,responseString1);
  strcat(fullResponsePayload,epdata.ssid);
  strcat(fullResponsePayload,responseString2);
  strcat(fullResponsePayload,epdata.password);
  strcat(fullResponsePayload,responseString3);
  strcat(fullResponsePayload,epdata.universe);
  strcat(fullResponsePayload,responseString4);
  strcat(fullResponsePayload,epdata.startChan);
  strcat(fullResponsePayload,responseString5);
  strcat(fullResponsePayload,epdata.fixtureMode);
  strcat(fullResponsePayload,responseString6);
  Serial.println(fullResponsePayload);
  return fullResponsePayload;
}
