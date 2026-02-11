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
    data["num_base_leds"]=epdata.num_base_leds;
    data["pixel_start_offset"]=epdata.pixel_start_offset;
    data["ap_name"]=epdata.ap_name;
    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
  });
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
    epdata.num_base_leds=int(data["num_base_leds"]);
    epdata.pixel_start_offset=int(data["pixel_start_offset"]);
    data["ap_name"].as<String>().toCharArray(epdata.ap_name,64);
    EEPROM.put(0,epdata);
    EEPROM.commit();
    Serial.println("Updated EEPROM values");
    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
    Serial.println(response);
    FastLED.clear();
  });
  server.addHandler(updateSettingsProcessor);
}

void handleGetLedPresets(){
  server.on("/getLedPresets", HTTP_GET, [](AsyncWebServerRequest *request) {
    // EEPROM.get(0,epdata);
    StaticJsonDocument<1560> data;
    Serial.println("handleGetLedPresets");
    for(int x=0;x<5;x++){
      for(int y=0;y<8;y++){
          data["ledPresets"][x][y]=epdata.ledPresets[x][y];
      }
    }
    for(int x=0;x<5;x++){
      data["presetTypes"][x]=epdata.presetTypes[x];
    }
    data["selectedPreset"]=selectedMode%5;
    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
  });
}

void handleUpdateLedPresets(){
  AsyncCallbackJsonWebHandler *updateLedPresetsProcessor = new AsyncCallbackJsonWebHandler("/updateLedPresets", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<1560> data;
    if (json.is<JsonArray>())
    {
      data = json.as<JsonArray>();
    }
    else if (json.is<JsonObject>())
    {
      data = json.as<JsonObject>();
    }
    Serial.println("handleUpdateLedPresets");
    for(int x=0;x<5;x++){
      for(int y=0;y<8;y++){
        epdata.ledPresets[x][y]=data["ledPresets"][x][y];
      }
    }
    for(int x=0;x<5;x++){
      epdata.presetTypes[x]=data["presetTypes"][x];
    }
    selectedMode=int(data["selectedPreset"])%5;
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

void handleGetPixelMapPresets(){
  server.on("/getPixelMapPresets", HTTP_GET, [](AsyncWebServerRequest *request) {
    StaticJsonDocument<1560> data;
    Serial.println("handleGetPixelMap");
    for(int x=0;x<5;x++){
      for(int y=0;y<8;y++){
        for(int z=0;z<8;z++){
          data["pixelMap"][x][y][z]=epdata.pixelMap[x][y][z];
        }
      }
    }
    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
  });
}

void handleUpdatePixelMapPresets(){
  AsyncCallbackJsonWebHandler *updatePixelMapProcessor = new AsyncCallbackJsonWebHandler("/updatePixelMap", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<1560> data;
    if (json.is<JsonArray>())
    {
      data = json.as<JsonArray>();
    }
    else if (json.is<JsonObject>())
    {
      data = json.as<JsonObject>();
    }
    Serial.println("handleUpdatePixelMap");
    for(int x=0;x<5;x++){
      for(int y=0;y<8;y++){
        for(int z=0;z<8;z++){
          epdata.pixelMap[x][y][z]=data["pixelMap"][x][y][z];
        }
      }
    }
    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
    Serial.println(response);
  });
  server.addHandler(updatePixelMapProcessor);
}

void handleRoot(){
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/index.html");
  });
}

void handleIndex(){
  server.serveStatic("/index.html", SPIFFS, "/index.html");
}

void setup_http_server(){
  SPIFFS.begin();
  handleRoot();
  handleIndex();
  handleGetSettings();
  handleGetLedPresets();
  handleUpdateSettings();
  handleUpdateLedPresets();
  handleGetPixelMapPresets();
  handleUpdatePixelMapPresets();
  server.begin();
}

void handleReset() {
    Serial.println("Reset-ing in 3,2,1....");
    delay(1000);
    stopStandaloneHotspot();
}
