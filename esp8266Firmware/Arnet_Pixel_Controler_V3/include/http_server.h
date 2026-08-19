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
    StaticJsonDocument<1024> data;
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
    data["accelerometer_mode"]=epdata.accelerometer_mode;
    data["send_osc"]=epdata.send_osc;
    data["osc_ip"]=epdata.osc_ip;
    data["osc_port"]=epdata.osc_port;
    data["osc_path_x"]=epdata.osc_path_x;
    data["osc_path_y"]=epdata.osc_path_y;
    data["osc_path_z"]=epdata.osc_path_z;
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
  epdata.accelerometer_mode=int(data["accelerometer_mode"]);
  epdata.send_osc=int(data["send_osc"]);
  data["osc_ip"].as<String>().toCharArray(epdata.osc_ip,32);
  epdata.osc_port=int(data["osc_port"]);
  data["osc_path_x"].as<String>().toCharArray(epdata.osc_path_x,32);
  data["osc_path_y"].as<String>().toCharArray(epdata.osc_path_y,32);
  data["osc_path_z"].as<String>().toCharArray(epdata.osc_path_z,32);
  EEPROM.put(0,epdata);
  EEPROM.commit();
  Serial.println("Updated EEPROM values");
  FastLED.clear();
  apply_max_brightness();
}

void handleUpdateSettings(){
  AsyncCallbackJsonWebHandler *updateSettingsProcessor = new AsyncCallbackJsonWebHandler("/updateSettings", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<1024> data;
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

// Lightweight index of every preset's type + which one is active, without
// paying for any preset's full content (in particular, none of the pixel
// maps) - lets the frontend know the whole fleet's preset shape up front
// while staying tiny regardless of how many presets exist.
void handleGetPresetIndex(){
  server.on("/getPresetIndex", HTTP_GET, [](AsyncWebServerRequest *request) {
    String response;
    response.reserve(32 + NUM_PRESETS*2);
    response += "{\"selectedPreset\":" + String(selectedMode) + ",\"presetTypes\":[";
    for(int i=0;i<NUM_PRESETS;i++){
      response += String(preset_store_read_type(i));
      if(i<NUM_PRESETS-1) response += ",";
    }
    response += "]}";
    AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", response);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    request->send(resp);
  });
}

// Returns exactly one preset's full content (?index=N) - the only place a
// pixel map's worth of data is ever transferred, and only for the one
// preset actually being viewed/edited, rather than the whole bank.
void handleGetPreset(){
  server.on("/getPreset", HTTP_GET, [](AsyncWebServerRequest *request) {
    int index = 0;
    if(request->hasParam("index")){
      index = request->getParam("index")->value().toInt();
    }
    if(index<0 || index>=NUM_PRESETS){
      index = 0;
    }
    PresetRecord r;
    if(index==activePresetIndex){
      r = activePreset;
    }else if(!preset_store_read(index, r)){
      preset_store_default(index, r);
    }
    String response;
    response.reserve(1500);
    response += "{\"index\":" + String(index) + ",\"ledPresets\":[";
    for(int i=0;i<8;i++){
      response += String(r.ledPresets[i]);
      if(i<7) response += ",";
    }
    response += "],\"pixelMap\":[";
    for(int x=0;x<8;x++){
      response += "[";
      for(int y=0;y<8;y++){
        response += String(r.pixelMap[x][y]);
        if(y<7) response += ",";
      }
      response += "]";
      if(x<7) response += ",";
    }
    response += "],\"presetType\":" + String(r.presetType) + "}";
    AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", response);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    request->send(resp);
  });
}

// Switches which preset is actively driving the LEDs. Deliberately just an
// index, not a preset's content - "select preset N" is a fleet-wide,
// broadcast-to-every-node action (see the frontend), so keeping it tiny
// matters more here than almost anywhere else in this API.
void handleSelectPreset(){
  AsyncCallbackJsonWebHandler *selectPresetProcessor = new AsyncCallbackJsonWebHandler("/selectPreset", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<64> data;
    if(json.is<JsonObject>()){
      data = json.as<JsonObject>();
    }
    int index = ((int(data["index"]) % NUM_PRESETS) + NUM_PRESETS) % NUM_PRESETS;
    selectedMode = index;
    load_active_preset(selectedMode);
    Serial.println("handleSelectPreset: "+String(selectedMode));
    AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", "{\"status\":\"ok\"}");
    resp->addHeader("Access-Control-Allow-Origin", "*");
    request->send(resp);
  });
  server.addHandler(selectPresetProcessor);
}

// Updates one preset's synth (ledPresets) content and type - a
// read-modify-write against its file record, preserving that preset's own
// pixel map untouched. Refreshes the in-RAM active-preset cache too if this
// happens to be the currently-active preset, so the LEDs reflect the edit
// immediately without waiting on a file re-read.
void apply_led_preset_update(int index, JsonVariant data){
  PresetRecord r;
  if(!preset_store_read(index, r)){
    preset_store_default(index, r);
  }
  for(int i=0;i<8;i++){
    r.ledPresets[i] = data["ledPresets"][i];
  }
  r.presetType = int(data["presetType"]);
  preset_store_write(index, r);
  if(index==activePresetIndex){
    activePreset = r;
  }
}

void handleUpdateLedPresets(){
  AsyncCallbackJsonWebHandler *updateLedPresetsProcessor = new AsyncCallbackJsonWebHandler("/updateLedPresets", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<256> data;
    if(json.is<JsonObject>()){
      data = json.as<JsonObject>();
    }
    int index = ((int(data["index"]) % NUM_PRESETS) + NUM_PRESETS) % NUM_PRESETS;
    Serial.println("handleUpdateLedPresets: index="+String(index));
    apply_led_preset_update(index, data);
    AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", "{\"status\":\"ok\"}");
    resp->addHeader("Access-Control-Allow-Origin", "*");
    request->send(resp);
  });
  server.addHandler(updateLedPresetsProcessor);
}

// Same read-modify-write approach as apply_led_preset_update, for the pixel
// map half of a preset's record.
void apply_pixel_map_preset_update(int index, JsonVariant data){
  PresetRecord r;
  if(!preset_store_read(index, r)){
    preset_store_default(index, r);
  }
  for(int x=0;x<8;x++){
    for(int y=0;y<8;y++){
      r.pixelMap[x][y] = data["pixelMap"][x][y];
    }
  }
  preset_store_write(index, r);
  if(index==activePresetIndex){
    activePreset = r;
  }
}

void handleUpdatePixelMapPresets(){
  AsyncCallbackJsonWebHandler *updatePixelMapProcessor = new AsyncCallbackJsonWebHandler("/updatePixelMap", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<512> data;
    if(json.is<JsonObject>()){
      data = json.as<JsonObject>();
    }
    int index = ((int(data["index"]) % NUM_PRESETS) + NUM_PRESETS) % NUM_PRESETS;
    Serial.println("handleUpdatePixelMap: index="+String(index));
    apply_pixel_map_preset_update(index, data);
    AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", "{\"status\":\"ok\"}");
    resp->addHeader("Access-Control-Allow-Origin", "*");
    request->send(resp);
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
  handleUpdateSettings();
  handleGetPresetIndex();
  handleGetPreset();
  handleSelectPreset();
  handleUpdateLedPresets();
  handleUpdatePixelMapPresets();
  handleCorsPreflight("/selectPreset");
  handleCorsPreflight("/updateLedPresets");
  handleCorsPreflight("/updatePixelMap");
  server.begin();
}

void handleReset() {
    Serial.println("Reset-ing in 3,2,1....");
    delay(1000);
    stopStandaloneHotspot();
}
