#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

// Defined later in mesh_funcs.h (included after this file in main.cpp). No-op
// when epdata.network_mode!=1, so every call site here stays unconditional.
void mesh_broadcast_json(String typeTag, JsonVariant payload);
// Also defined in mesh_funcs.h - used by the Presets-page handlers below to
// support targeting specific mesh nodes instead of the whole fleet. In
// standalone/LAN mode, targeting works differently: the browser sends one
// independent, CORS-enabled request directly to each selected device (see
// the Nodes-page JS and the CORS setup below) rather than any one device
// relaying to the others, so no device-to-device dispatch exists for LAN
// mode at all - every request a device receives here is already meant for
// just that device.
bool mesh_targets_include_self(JsonArray targetNodeIds);
void mesh_send_to_targets(String typeTag, JsonVariant payload, JsonArray targetNodeIds);

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
    data["network_mode"]=epdata.network_mode;
    data["mesh_name"]=epdata.mesh_name;
    data["mesh_password"]=epdata.mesh_password;
    data["mesh_leader"]=epdata.mesh_leader;
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
  epdata.network_mode=int(data["network_mode"]);
  data["mesh_name"].as<String>().toCharArray(epdata.mesh_name,64);
  data["mesh_password"].as<String>().toCharArray(epdata.mesh_password,64);
  epdata.mesh_leader=int(data["mesh_leader"]);
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
    mesh_broadcast_json("settings", data);
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
    // 1560 fits ledPresets/presetTypes/selectedPreset alone (see handleGetLedPresets);
    // padded up here to leave headroom for the optional targetNodes array without
    // risking the exact silent-overflow bug already hit once on the mesh envelope side.
    StaticJsonDocument<1700> data;
    if (json.is<JsonArray>())
    {
      data = json.as<JsonArray>();
    }
    else if (json.is<JsonObject>())
    {
      data = json.as<JsonObject>();
    }
    Serial.println("handleUpdateLedPresets");
    // targetNodes only ever has meaning in mesh mode now - in standalone/LAN
    // mode the browser addresses each selected device directly, so a plain
    // request here always means "apply to me."
    JsonArray targetNodes = data["targetNodes"];
    bool hasMeshTargets = (epdata.network_mode==1) && !(targetNodes.isNull() || targetNodes.size()==0);
    bool applyLocally = hasMeshTargets ? mesh_targets_include_self(targetNodes) : true;
    String response;
    serializeJson(data, response);
    if(applyLocally){
      apply_led_presets_update(data);
      request->send(200, "application/json", response);
    }else{
      request->send(200, "application/json", "{\"status\":\"dispatched\"}");
    }
    Serial.println(response);
    if(hasMeshTargets){
      mesh_send_to_targets("ledPresets", data, targetNodes);
    }else{
      mesh_broadcast_json("ledPresets", data); // no-op outside mesh mode
    }
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
    // Padded past pixelMap's own 1560 (see handleGetPixelMapPresets) to leave
    // headroom for the optional targetNodes array - same overflow risk already
    // hit once on the mesh envelope side, avoided here the same way.
    StaticJsonDocument<1700> data;
    if (json.is<JsonArray>())
    {
      data = json.as<JsonArray>();
    }
    else if (json.is<JsonObject>())
    {
      data = json.as<JsonObject>();
    }
    Serial.println("handleUpdatePixelMap");
    // targetNodes only ever has meaning in mesh mode now - in standalone/LAN
    // mode the browser addresses each selected device directly, so a plain
    // request here always means "apply to me."
    JsonArray targetNodes = data["targetNodes"];
    bool hasMeshTargets = (epdata.network_mode==1) && !(targetNodes.isNull() || targetNodes.size()==0);
    bool applyLocally = hasMeshTargets ? mesh_targets_include_self(targetNodes) : true;
    String response;
    serializeJson(data, response);
    if(applyLocally){
      apply_pixel_map_update(data);
      request->send(200, "application/json", response);
    }else{
      request->send(200, "application/json", "{\"status\":\"dispatched\"}");
    }
    Serial.println(response);
    if(hasMeshTargets){
      mesh_send_to_targets("pixelMap", data, targetNodes);
    }else{
      mesh_broadcast_json("pixelMap", data); // no-op outside mesh mode
    }
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
    response->addHeader("Access-Control-Allow-Methods", "POST");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    request->send(response);
  });
}

void setup_http_server(){
  SPIFFS.begin();
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
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
