#include <ESP8266WebServer.h>
#include "html_index.h"
#include <ArduinoJson.h>

ESP8266WebServer    server(80);

void handleMain() {
  server.send_P(200, "text/html", html_main ); 
}

void handleUpdateSettings() {
  StaticJsonDocument<640> JSONData;
  String jsonString = server.arg("plain");
  DeserializationError error = deserializeJson(JSONData, jsonString);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    server.send(500,"application/json","Error in parsing");
    return;
  }else{
   if(JSONData.containsKey("ssid")){
    JSONData["ssid"].as<String>().toCharArray(epdata.ssid,64);
    JSONData["password"].as<String>().toCharArray(epdata.password,64);
    JSONData["universe"].as<String>().toCharArray(epdata.universe,64);
    JSONData["startChan"].as<String>().toCharArray(epdata.startChan,64);
    JSONData["fixtureMode"].as<String>().toCharArray(epdata.fixtureMode,64);
    JSONData["oscAddressX"].as<String>().toCharArray(epdata.oscAddressX,64);
    JSONData["oscAddressY"].as<String>().toCharArray(epdata.oscAddressY,64);
    JSONData["oscAddressZ"].as<String>().toCharArray(epdata.oscAddressZ,64);
    JSONData["oscTargetIP"].as<String>().toCharArray(epdata.oscTargetIP,64);
    JSONData["oscPort"].as<String>().toCharArray(epdata.oscPort,64);
    EEPROM.put(0,epdata);
    EEPROM.commit();
    Serial.println("Updated EEPROM values");
    server.send(200,"application/json",String(JSONData["ssid"].as<String>())+" Received");
   }
   else{
     server.send(400,"application/json","Bad JSON");
   }
  }
}

void handleGetSettings() {
  if (server.method() != HTTP_GET) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    EEPROM.get(0,epdata);
    StaticJsonDocument<640> JSONData;
    JSONData["ssid"]=epdata.ssid;
    JSONData["password"]=epdata.password;
    JSONData["universe"]=epdata.universe;
    JSONData["startChan"]=epdata.startChan;
    JSONData["fixtureMode"]=epdata.fixtureMode;
    JSONData["oscAddressX"]=epdata.oscAddressX;
    JSONData["oscAddressY"]=epdata.oscAddressY;
    JSONData["oscAddressZ"]=epdata.oscAddressZ;
    JSONData["oscTargetIP"]=epdata.oscTargetIP;
    JSONData["oscPort"]=epdata.oscPort;
    char data[640];
    serializeJson(JSONData,data);
    server.send(200,"application/json",data);
    Serial.println("handleGetSettings");
  }
}

void handleReset() {
    Serial.println("Reset-ing in 3,2,1....");
    server.send(200,"application/json",String(" Reset request received"));
    delay(1000);
    stopStandaloneHotspot();
}

void handleNotFound() {
  server.send(404,   "text/html", "<html><body><p>404 Error</p></body></html>" );
}

void setup_http_server(){
  server.on("/", handleMain);
  server.on("/updateSettings", handleUpdateSettings);
  server.on("/getSettings", handleGetSettings);
  server.on("/reset", handleReset);
  server.onNotFound(handleNotFound);
  server.begin();
}

void handle_http_server(){
  server.handleClient();
}
