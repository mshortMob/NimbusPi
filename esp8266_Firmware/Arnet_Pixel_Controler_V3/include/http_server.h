#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

static AsyncWebServer server(80);

void handleGetSettings(){
  server.on("/getSettings", HTTP_GET, [](AsyncWebServerRequest *request) {
    EEPROM.get(0,epdata);
    StaticJsonDocument<640> data;
    data["ssid"]=epdata.ssid;
    data["password"]=epdata.password;
    data["universe"]=epdata.universe;
    data["startChan"]=epdata.startChan;
    data["fixtureMode"]=epdata.fixtureMode;
    data["oscAddressX"]=epdata.oscAddressX;
    data["oscAddressY"]=epdata.oscAddressY;
    data["oscAddressZ"]=epdata.oscAddressZ;
    data["oscTargetIP"]=epdata.oscTargetIP;
    data["oscPort"]=epdata.oscPort;
    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
  });
}

void handleGetLedPresets(){
  server.on("/getLedPresets", HTTP_GET, [](AsyncWebServerRequest *request) {
    // EEPROM.get(0,epdata);
    StaticJsonDocument<640> data;
    Serial.println("handleGetLedPresets");
    for(int x=0;x<4;x++){
      for(int y=0;y<8;y++){
        data["ledPresets"][x][y]=epdata.ledPresets[x][y];
      }
    }
    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
  });
}

void handleRoot(){
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/index.html");
  });
}

void handleIndex(){
  server.serveStatic("/index.html", SPIFFS, "/index.html");
}

void handleUpdateSettings(){
  AsyncCallbackJsonWebHandler *updateSettingsProcessor = new AsyncCallbackJsonWebHandler("/updateSettings", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<640> data;
    if (json.is<JsonArray>())
    {
      data = json.as<JsonArray>();
    }
    else if (json.is<JsonObject>())
    {
      data = json.as<JsonObject>();
    }
    data["ssid"].as<String>().toCharArray(epdata.ssid,64);
    data["password"].as<String>().toCharArray(epdata.password,64);
    data["universe"].as<String>().toCharArray(epdata.universe,32);
    data["startChan"].as<String>().toCharArray(epdata.startChan,64);
    data["fixtureMode"].as<String>().toCharArray(epdata.fixtureMode,64);
    data["oscAddressX"].as<String>().toCharArray(epdata.oscAddressX,64);
    data["oscAddressY"].as<String>().toCharArray(epdata.oscAddressY,64);
    data["oscAddressZ"].as<String>().toCharArray(epdata.oscAddressZ,64);
    data["oscTargetIP"].as<String>().toCharArray(epdata.oscTargetIP,64);
    data["oscPort"].as<String>().toCharArray(epdata.oscPort,64);
    EEPROM.put(0,epdata);
    EEPROM.commit();
    Serial.println("Updated EEPROM values");
    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
    Serial.println(response);
  });
  server.addHandler(updateSettingsProcessor);
}

void handleUpdateLedPresets(){
  AsyncCallbackJsonWebHandler *updateLedPresetsProcessor = new AsyncCallbackJsonWebHandler("/updateLedPresets", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<640> data;
    if (json.is<JsonArray>())
    {
      data = json.as<JsonArray>();
    }
    else if (json.is<JsonObject>())
    {
      data = json.as<JsonObject>();
    }
    Serial.println("handleUpdateLedPresets");
    for(int x=0;x<4;x++){
      for(int y=0;y<8;y++){
        epdata.ledPresets[x][y]=data["ledPresets"][x][y];
      }
    }
    // EEPROM.put(0,epdata);
    // EEPROM.commit();
    // Serial.println("Updated EEPROM values");
    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
    Serial.println(response);
  });
  server.addHandler(updateLedPresetsProcessor);
}

void setup_http_server(){
  SPIFFS.begin();
  handleRoot();
  handleIndex();
  handleGetSettings();
  handleGetLedPresets();
  handleUpdateSettings();
  handleUpdateLedPresets();
  server.begin();
}

void handleReset() {
    Serial.println("Reset-ing in 3,2,1....");
    delay(1000);
    stopStandaloneHotspot();
}
