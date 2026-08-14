#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

// Standalone/LAN-mode targeting has the browser send one independent,
// CORS-enabled request directly to each selected device (see the Nodes-page
// JS and the CORS setup below) - there's no device-to-device relay, so every
// request a device receives here is already meant for just that device.
static AsyncWebServer server(80);

void handleGetSettings(){
  server.on("/getSettings", HTTP_GET, [](AsyncWebServerRequest *request) {
    EEPROM.get(0,epdata);
    StaticJsonDocument<768> data;
    data["ssid"]=epdata.ssid;
    data["password"]=epdata.password;
    data["universe"]=epdata.universe;
    data["startChan"]=epdata.startChan;
    data["fixtureMode"]=epdata.fixtureMode;
    data["num_base_leds"]=epdata.num_base_leds;
    data["pixel_start_offset"]=epdata.pixel_start_offset;
    data["ap_name"]=epdata.ap_name;
    data["ssid_timeout"]=epdata.ssid_timeout_seconds;
    data["ap_password"]=epdata.ap_password;
    data["control_mode"]=epdata.control_mode;
    data["max_brightness_percent"]=epdata.max_brightness_percent;
    data["enable_led_yield"]=epdata.enable_led_yield;
    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
  });
}

void apply_settings_update(JsonVariant data){
  data["ssid"].as<String>().toCharArray(epdata.ssid,64);
  data["password"].as<String>().toCharArray(epdata.password,64);
  data["universe"].as<String>().toCharArray(epdata.universe,32);
  data["startChan"].as<String>().toCharArray(epdata.startChan,64);
  data["fixtureMode"].as<String>().toCharArray(epdata.fixtureMode,64);
  epdata.num_base_leds=int(data["num_base_leds"]);
  epdata.pixel_start_offset=int(data["pixel_start_offset"]);
  data["ap_name"].as<String>().toCharArray(epdata.ap_name,64);
  epdata.ssid_timeout_seconds=int(data["ssid_timeout"]);
  data["ap_password"].as<String>().toCharArray(epdata.ap_password,64);
  epdata.control_mode=int(data["control_mode"]);
  epdata.max_brightness_percent=int(data["max_brightness_percent"]);
  epdata.enable_led_yield=int(data["enable_led_yield"]);
  EEPROM.put(0,epdata);
  EEPROM.commit();
  Serial.println("Updated EEPROM values");
  FastLED.clear();
  apply_max_brightness();
}

void handleUpdateSettings(){
  AsyncCallbackJsonWebHandler *updateSettingsProcessor = new AsyncCallbackJsonWebHandler("/updateSettings", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<768> data;
    if (json.is<JsonArray>())
    {
      data = json.as<JsonArray>();
    }
    else if (json.is<JsonObject>())
    {
      data = json.as<JsonObject>();
    }
    apply_settings_update(data);
    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
    Serial.println(response);
  });
  server.addHandler(updateSettingsProcessor);
}

void handleGetLedPresets(){
  server.on("/getLedPresets", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("handleGetLedPresets");
    // Built via plain string concatenation instead of ArduinoJson - v7's
    // document model allocates dynamically per element (confirmed via a
    // captured crash: a heap-exhaustion null-pointer write inside
    // AsyncWebServerResponse's constructor, with an ArduinoJson
    // MemberProxy::getOrCreateData() call on the same stack), which adds up
    // for this shape under tight heap conditions. This is simple, regular
    // data - string-building it directly is much lighter.
    String response;
    response.reserve(280);
    response += "{\"ledPresets\":[";
    for(int x=0;x<5;x++){
      response += "[";
      for(int y=0;y<8;y++){
        response += String(epdata.ledPresets[x][y]);
        if(y<7) response += ",";
      }
      response += "]";
      if(x<4) response += ",";
    }
    response += "],\"presetTypes\":[";
    for(int x=0;x<5;x++){
      response += String(epdata.presetTypes[x]);
      if(x<4) response += ",";
    }
    response += "],\"selectedPreset\":" + String(selectedMode%5) + "}";
    request->send(200, "application/json", response);
  });
}

void apply_led_presets_update(JsonVariant data){
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
}

void handleUpdateLedPresets(){
  AsyncCallbackJsonWebHandler *updateLedPresetsProcessor = new AsyncCallbackJsonWebHandler("/updateLedPresets", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // 1560 fits ledPresets/presetTypes/selectedPreset (see handleGetLedPresets).
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
    apply_led_presets_update(data);
    String response;
    serializeJson(data, response);
    // The CORS header is added only on this specific response (not via
    // DefaultHeaders, which applies to every response) since standalone/LAN
    // targeted requests (browser fanning out to each selected device's own
    // address) are the only ones ever cross-origin.
    AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", response);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    request->send(resp);
    Serial.println(response);
  });
  server.addHandler(updateLedPresetsProcessor);
}

void handleGetPixelMapPresets(){
  server.on("/getPixelMapPresets", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("handleGetPixelMap");
    // Built via plain string concatenation instead of ArduinoJson - see the
    // comment in handleGetLedPresets. This is by far the biggest of these
    // responses (5x8x8 = 320 values), so the heaviest place ArduinoJson's
    // per-element dynamic allocation was adding up.
    String response;
    response.reserve(1400);
    response += "{\"pixelMap\":[";
    for(int x=0;x<5;x++){
      response += "[";
      for(int y=0;y<8;y++){
        response += "[";
        for(int z=0;z<8;z++){
          response += String(epdata.pixelMap[x][y][z]);
          if(z<7) response += ",";
        }
        response += "]";
        if(y<7) response += ",";
      }
      response += "]";
      if(x<4) response += ",";
    }
    response += "]}";
    request->send(200, "application/json", response);
  });
}

void apply_pixel_map_update(JsonVariant data){
  for(int x=0;x<5;x++){
    for(int y=0;y<8;y++){
      for(int z=0;z<8;z++){
        epdata.pixelMap[x][y][z]=data["pixelMap"][x][y][z];
      }
    }
  }
}

void handleUpdatePixelMapPresets(){
  AsyncCallbackJsonWebHandler *updatePixelMapProcessor = new AsyncCallbackJsonWebHandler("/updatePixelMap", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // 1560 fits pixelMap alone (see handleGetPixelMapPresets).
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
    apply_pixel_map_update(data);
    String response;
    serializeJson(data, response);
    // See handleUpdateLedPresets - CORS header scoped to just this response,
    // not applied globally via DefaultHeaders.
    AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", response);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    request->send(resp);
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

void handleFavicon(){
  server.serveStatic("/favicon.svg", SPIFFS, "/favicon.svg");
}

// Standalone/LAN-mode targeting has the browser POST directly to each
// selected device's own address, which makes it a cross-origin request from
// the browser's perspective (the page itself was loaded from a *different*
// device). Content-Type: application/json makes it a "non-simple" request,
// so the browser sends a preflight OPTIONS check first - it has to succeed,
// and the actual response needs the Allow-Origin header too, or the browser
// throws the whole exchange away before our code ever sees it.
void handleCorsPreflight(const char* path){
  server.on(path, HTTP_OPTIONS, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse(204);
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "POST");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    request->send(response);
  });
}

void setup_http_server(){
  SPIFFS.begin();
  handleRoot();
  handleIndex();
  handleFavicon();
  handleGetSettings();
  handleGetLedPresets();
  handleUpdateSettings();
  handleUpdateLedPresets();
  handleGetPixelMapPresets();
  handleUpdatePixelMapPresets();
  handleCorsPreflight("/updateLedPresets");
  handleCorsPreflight("/updatePixelMap");
  server.begin();
}

void handleReset() {
    Serial.println("Reset-ing in 3,2,1....");
    delay(1000);
    stopStandaloneHotspot();
}
