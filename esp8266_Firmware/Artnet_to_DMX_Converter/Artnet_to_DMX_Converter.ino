#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include <ArtnetWifi.h>
#include <ESPDMX.h>

int led_pin=15;
int encoderPin1=12;
int encoderPin2=14;
int encoderButtonPin=4;

const int numDMXChannels=64;
const char *ssid = "labnetnode"; //HotSpot Password, use web UI to setup local wifi
const char *password = "mshort123";
bool startupMode=true;
bool wifiConnected=false;
ESP8266WebServer server(80);
struct {
  char ssid[256] = "TempSSID";
  char password[256] = "TempPassword";
  char universe[32] = "5";
  char startChan[32] = "6";
}epdata;
Adafruit_NeoPixel leds = Adafruit_NeoPixel(1, led_pin, NEO_GRBW + NEO_KHZ800);
ArtnetWifi artnet;
DMXESPSerial dmx;
int encoderLock=0;
int encoderLock2=0;
int encoderValue=120;
int encoderButtonValue=0;

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
      server.arg("universe").toCharArray(epdata.universe,32);
    }
    if (server.hasArg("startChan")) { // this is the variable sent from the client
      Serial.print("startChan:");
      Serial.println(server.arg("startChan"));
      server.arg("startChan").toCharArray(epdata.startChan,32);
      startupMode=false;
      EEPROM.put(0,epdata);
      EEPROM.commit();
    }
    Serial.println("");
    Serial.println("Page Load");
    Serial.println("");
    server.send(200, "text/html", "<html><head><title>FlowMapper</title><style>  body{    margin:0px;    background-color:grey;  }  input{    width:60%;    height:5.5%;    left:18%;    margin-top:1%;    position: absolute;    }  button{    background-color:lightgrey;    width:60%;    height:10%;    left:18.0%;    margin-top:2.5%;    position: absolute;  }   label{    width:10%;    height:10%;    left:6%;    margin-top:1.5%;    position: absolute;    vertical-align:middle;  }  body{    padding-top:1.5%;  }</style></head><body cz-shortcut-listen='true'>  <label for='SSID'>SSID:</label><input type='text' id='SSID'><br><br>  <label for='password'>Password:</label><input type='test' id='password'><br><br>  <label for='universe'>Universe</label><input type='text' id='universe'><br><br>  <label for='startChannel'>Start Channel:</label><input type='text' id='startChannel'><br><br>    <button onclick='submitForm();'>Save</button><script> document.getElementById('SSID').value='Test1234';document.getElementById('password').value='Test1234';document.getElementById('universe').value='1';document.getElementById('startChannel').value='1';function submitForm() {  payload='http://' + location.host + '/data/?ssid='+document.getElementById('SSID').value+'&password='+document.getElementById('password').value+'&universe='+document.getElementById('universe').value+'&startChan='+document.getElementById('startChannel').value;  var xhttp = new XMLHttpRequest();  xhttp.onreadystatechange = function() {    if (this.readyState == 4 && this.status == 200) {      console.log(this.responseText);    }  };  xhttp.open('GET', payload, true);  xhttp.send();} </script></body></html>");
}


void setup() {
  Serial.begin(115200);
  pinMode(encoderPin1, INPUT_PULLUP); 
  digitalWrite(encoderPin1, HIGH);
  pinMode(encoderPin2, INPUT_PULLUP); 
  digitalWrite(encoderPin2, HIGH);
  pinMode(encoderButtonPin, INPUT_PULLUP); 
  digitalWrite(encoderButtonPin, HIGH);
  pinMode(led_pin, OUTPUT); 
  Serial.println("");
  Serial.println("");
  EEPROM.begin(576);
  leds.begin();
  dmx.init(numDMXChannels);
  delay(1000);

//  EEPROM.put(0,epdata);
//  EEPROM.commit();
  EEPROM.get(0,epdata);
  Serial.println("EEPROM READ:");
  Serial.println(epdata.ssid);
  Serial.println(epdata.password);
  Serial.println(epdata.universe);
  Serial.println(epdata.startChan);

  wifiConnected = connectToSavedWifi();
  if(wifiConnected){
    startupMode=false;
    artnet.begin();
    artnet.setArtDmxCallback(onDmxFrame);
  }else{
    startupMode=true;
    Serial.println("Entering Startup Mode(WiFi Host: 192.168.4.1/data/)");
    leds.setPixelColor(0, 0, 0, 255);
    leds.setBrightness(100);
    leds.show();
    WiFi.softAP(ssid, password);
    IPAddress myIP = WiFi.softAPIP();
    server.on("/data/", HTTP_GET, adminUI); // when the server receives a request with /data/ in the string then run the handleSentVar function
    server.begin();
  }
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
  leds.setPixelColor(0, 255, 0, 255);
  leds.setBrightness(100);
  leds.show();
  int filterUniverse=atoi(epdata.universe);
  int filterChan=atoi(epdata.startChan)-1;
  Serial.print("Filtering for universe:");
  Serial.println(filterUniverse);
  if(universe==filterUniverse){
    Serial.print("Received artnet data for universe:");
    Serial.println(universe);
    for(int i=0;i<numDMXChannels;i++){
      dmx.write(i+1+filterChan, data[i]);      // channal 1 on    
    }
    dmx.update();           // update the DMX bus    
  }
}

void loop() {
  if(startupMode){
    server.handleClient();
  }else{
    if(!wifiConnected){
      Serial.println("Stopping Web Server");
      server.close();
      server.stop();
      WiFi.softAPdisconnect(true);
      wifiConnected = connectToSavedWifi();
      if(!wifiConnected){
        startupMode=true;
        Serial.println("Entering Startup Mode(WiFi Host: 192.168.4.1/data/)");
        leds.setPixelColor(0, 0, 0, 255);
        leds.setBrightness(100);
        leds.show();
        WiFi.softAP(ssid, password);
        IPAddress myIP = WiFi.softAPIP();
        server.on("/data/", HTTP_GET, adminUI); // when the server receives a request with /data/ in the string then run the handleSentVar function
        server.begin();        
      }
    }else{
      leds.setPixelColor(0, 0, 255, 0);
      leds.setBrightness(100);
      leds.show();
      artnet.read();      
    }
  }
  getEncoderData();
  if(encoderButtonValue==2 and startupMode){
    startupMode=false;
  }
}

boolean connectToSavedWifi(void)
{
  leds.setPixelColor(0, 255, 0, 0);
  leds.setBrightness(100);
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
    leds.setPixelColor(0, 0, 255, 0);
    leds.setBrightness(100);
    leds.show();
  } else {
    Serial.println("");
    Serial.println("Connection failed.");
  }
  
  return state;
}


void getEncoderData(){
  if(digitalRead(encoderPin1)==0 and digitalRead(encoderPin2)==1 and encoderLock==0){
    encoderLock=1;
    Serial.print("Encoder Position:");
    Serial.println(encoderValue);
    if(encoderValue<250){
      encoderValue=encoderValue+10;
    }
  }
  if(digitalRead(encoderPin1)==1 and digitalRead(encoderPin2)==0 and encoderLock==0){
    encoderLock=1;
    Serial.print("Encoder Position:");
    Serial.println(encoderValue);
    if(encoderValue>0){
      encoderValue=encoderValue-10;
    }   
  }
  if(digitalRead(encoderPin1)==1 and digitalRead(encoderPin2)==1){
    encoderLock=0;
  }

  if(digitalRead(encoderButtonPin)==0 and encoderLock2==0){
    encoderLock2=1;
    Serial.print("Encoder Button:");
    Serial.println(encoderButtonValue);
    encoderButtonValue=(encoderButtonValue+1)%3;
  }
  if(digitalRead(encoderButtonPin)==1){
    encoderLock2=0;
  }
  
}
