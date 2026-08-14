#include <WiFiUdp.h>
#include <IPAddress.h>
#include <ArduinoJson.h>
#include <map>

#define LAN_DISCOVERY_PORT 5556
#define LAN_NODE_INFO_INTERVAL_MS 10000

struct LanPeerInfo {
  String hostname;
  String ip;
  unsigned long lastSeen;
};
std::map<String, LanPeerInfo> discoveredLanPeers; // keyed by ip string
WiFiUDP lanDiscoveryUdp;
unsigned long lastLanAnnounce = 0;

// This device-listing feature only makes sense when: we're actually on a
// real LAN (STA-connected to an external router, not the isolated AP
// hotspot fallback) and GUI Presets - not ArtNet/DMX - are what's actually
// driving the LEDs.
//
// Devices never talk to each other at all - this module only handles
// discovery (so the Nodes page can list peers and their addresses).
// Targeted preset changes are sent by the browser directly to each selected
// device's own address (see the Nodes-page JS in data/index.html and the
// CORS setup in http_server.h); a device here never relays anything to
// another device. An earlier device-to-device relay design was scrapped
// after it repeatedly crashed the ESP8266 - starting outbound async
// connections from a device, even carefully throttled to one at a time,
// kept running into ESP8266-specific ESPAsyncTCP/AsyncWebServer fragility
// (see github.com/me-no-dev/ESPAsyncWebServer issues #364, #89). Having the
// browser fan out instead avoids all of that entirely.
bool lan_targeting_available(void){
  return wifiConnected && epdata.control_mode==0;
}

void announce_lan_peer_info(void){
  StaticJsonDocument<96> doc;
  doc["hostname"] = epdata.ap_name;
  String out;
  serializeJson(doc, out);
  lanDiscoveryUdp.beginPacket(IPAddress(255,255,255,255), LAN_DISCOVERY_PORT);
  lanDiscoveryUdp.write((const uint8_t*)out.c_str(), out.length());
  lanDiscoveryUdp.endPacket();
}

void handle_lan_peers(void){
  if(!lan_targeting_available()){
    return;
  }
  unsigned long now = millis();
  if(now - lastLanAnnounce > LAN_NODE_INFO_INTERVAL_MS){
    announce_lan_peer_info();
    lastLanAnnounce = now;
  }
  int packetSize = lanDiscoveryUdp.parsePacket();
  if(packetSize > 0){
    char buf[128];
    int len = lanDiscoveryUdp.read(buf, min(packetSize, 127));
    if(len > 0){
      buf[len] = '\0';
      StaticJsonDocument<128> doc;
      if(!deserializeJson(doc, buf)){
        String ip = lanDiscoveryUdp.remoteIP().toString();
        LanPeerInfo info;
        info.hostname = doc["hostname"].as<String>();
        info.ip = ip;
        info.lastSeen = now;
        discoveredLanPeers[ip] = info;
      }
    }
  }
}

bool lan_target_matches_self(const String &target){
  return target.equalsIgnoreCase(epdata.ap_name) || target == WiFi.localIP().toString();
}

void handleGetLanPeers(void){
  server.on("/getLanPeers", HTTP_GET, [](AsyncWebServerRequest *request) {
    sanitize_lan_peers();
    DynamicJsonDocument data(1536);
    JsonArray peers = data.createNestedArray("peers");

    JsonObject selfPeer = peers.createNestedObject();
    selfPeer["hostname"] = epdata.ap_name;
    selfPeer["ip"] = WiFi.localIP().toString();
    selfPeer["isSelf"] = 1;
    selfPeer["source"] = "self";

    for(auto &kv : discoveredLanPeers){
      if(lan_target_matches_self(kv.second.hostname) || kv.first==WiFi.localIP().toString()){
        continue;
      }
      JsonObject peer = peers.createNestedObject();
      peer["hostname"] = kv.second.hostname;
      peer["ip"] = kv.second.ip;
      peer["isSelf"] = 0;
      peer["source"] = "auto";
    }

    for(int x=0;x<8;x++){
      if(strnlen(epdata.lan_peers[x],64)==0){
        continue;
      }
      String entry = String(epdata.lan_peers[x]);
      bool alreadyListed = false;
      for(auto &kv : discoveredLanPeers){
        if(kv.second.hostname.equalsIgnoreCase(entry) || kv.first==entry){
          alreadyListed = true;
          break;
        }
      }
      if(alreadyListed || lan_target_matches_self(entry)){
        continue;
      }
      JsonObject peer = peers.createNestedObject();
      peer["hostname"] = entry;
      peer["ip"] = "";
      peer["isSelf"] = 0;
      peer["source"] = "manual";
    }

    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
  });
}

void handleUpdateLanPeers(void){
  AsyncCallbackJsonWebHandler *updateLanPeersProcessor = new AsyncCallbackJsonWebHandler("/updateLanPeers", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<768> data;
    if(json.is<JsonObject>()){
      data = json.as<JsonObject>();
    }
    JsonArray peers = data["peers"];
    for(int x=0;x<8;x++){
      if(!peers.isNull() && x<(int)peers.size()){
        String entry = peers[x].as<String>();
        entry.toCharArray(epdata.lan_peers[x], 64);
      }else{
        epdata.lan_peers[x][0] = '\0';
      }
    }
    EEPROM.put(0,epdata);
    EEPROM.commit();
    Serial.println("Updated lan_peers");
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });
  server.addHandler(updateLanPeersProcessor);
}

void setup_lan_peers(void){
  lanDiscoveryUdp.begin(LAN_DISCOVERY_PORT);
  handleGetLanPeers();
  handleUpdateLanPeers();
}
