#include <painlessMesh.h>
#include <ArduinoJson.h>

#define MESH_PORT 5555

painlessMesh mesh;
bool meshApplyingIncoming = false;

void meshReceivedCallback(uint32_t from, String &msg);
void meshNewConnectionCallback(uint32_t nodeId);
void meshChangedConnectionsCallback();

void setup_mesh(void){
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(epdata.mesh_name, epdata.mesh_password, MESH_PORT);
  mesh.setContainsRoot(true); // every node: mesh has a root, organize around it
  if(epdata.mesh_leader==1){
    mesh.setRoot(true);
  }
  mesh.onReceive(&meshReceivedCallback);
  mesh.onNewConnection(&meshNewConnectionCallback);
  mesh.onChangedConnections(&meshChangedConnectionsCallback);
  if (MDNS.begin(epdata.ap_name)) {
    Serial.println("mDNS responder started (mesh mode)");
  }
  Serial.print("Mesh started, nodeId: ");
  Serial.println(mesh.getNodeId());

  // Bounded wait for at least one peer, with a visual join-status indicator.
  // Most meaningful for Pure Nodes - a Leader booting first with zero peers
  // isn't a failure, it's just waiting, but the same indicator is still
  // informative there too.
  setLEDSToASingleColor(0,25,25); // joining mesh
  unsigned long meshJoinDeadline = millis() + 8000;
  bool meshJoined = false;
  while(millis() < meshJoinDeadline){
    mesh.update();
    if(mesh.getNodeList().size() > 0){
      meshJoined = true;
      break;
    }
    delay(50);
  }
  if(meshJoined){
    setLEDSToASingleColor(0,25,0); // found at least one peer
  }else{
    setLEDSToASingleColor(25,0,25); // no peers found within the wait window
  }
  delay(1000);
}

void handle_mesh(void){
  mesh.update();
  MDNS.update();
}

// Called by http_server.h after a locally-applied change. No-op outside mesh
// mode, and never re-broadcasts a change that just arrived FROM the mesh -
// painlessMesh already floods a broadcast to every connected node at the
// protocol layer, so re-sending here would just amplify traffic on every hop.
//
// Built via plain string concatenation rather than nesting `payload` inside a
// second StaticJsonDocument - payloads like ledPresets/pixelMap are already
// sized close to 1560 bytes on their own, and copying them into a *second*
// fixed-size arena (on top of that arena's own wrapper overhead) silently
// overflowed a same-sized envelope document, dropping/corrupting exactly the
// larger broadcast types while the tiny selectedMode one kept working fine.
void mesh_broadcast_json(String typeTag, JsonVariant payload){
  if(epdata.network_mode!=1){
    return;
  }
  if(meshApplyingIncoming){
    return;
  }
  String payloadStr;
  serializeJson(payload, payloadStr);
  String out = "{\"type\":\"" + typeTag + "\",\"src\":" + String(mesh.getNodeId()) + ",\"data\":" + payloadStr + "}";
  mesh.sendBroadcast(out);
}

void meshReceivedCallback(uint32_t from, String &msg){
  StaticJsonDocument<2048> envelope;
  DeserializationError err = deserializeJson(envelope, msg);
  if(err){
    return;
  }
  String type = envelope["type"].as<String>();
  JsonVariant data = envelope["data"];
  meshApplyingIncoming = true;
  if(type=="settings"){
    apply_settings_update(data);
  }else if(type=="ledPresets"){
    apply_led_presets_update(data);
  }else if(type=="pixelMap"){
    apply_pixel_map_update(data);
  }else if(type=="selectedMode"){
    selectedMode = int(data) % 5;
  }
  meshApplyingIncoming = false;
}

void meshNewConnectionCallback(uint32_t nodeId){
  Serial.print("Mesh: new connection, nodeId=");
  Serial.println(nodeId);
}

void meshChangedConnectionsCallback(){
  Serial.println("Mesh: connections changed");
}
