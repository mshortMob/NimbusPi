#include <painlessMesh.h>
#include <ArduinoJson.h>
#include <map>

#define MESH_PORT 5555
#define MESH_NODE_INFO_INTERVAL_MS 20000

painlessMesh mesh;
bool meshApplyingIncoming = false;

struct NodeInfo {
  String hostname;
  int isLeader;
};
std::map<uint32_t, NodeInfo> knownNodes;
unsigned long lastNodeInfoAnnounce = 0;

void meshReceivedCallback(uint32_t from, String &msg);
void meshNewConnectionCallback(uint32_t nodeId);
void meshChangedConnectionsCallback();
void announce_node_info(void);
void handleGetMeshNodes(void);

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

  handleGetMeshNodes();
  announce_node_info();
  lastNodeInfoAnnounce = millis();
}

void handle_mesh(void){
  mesh.update();
  MDNS.update();
  if(millis() - lastNodeInfoAnnounce > MESH_NODE_INFO_INTERVAL_MS){
    announce_node_info();
    lastNodeInfoAnnounce = millis();
  }
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
  if(type=="nodeInfo"){
    // Self-announcement, not a config change - apply outside the
    // meshApplyingIncoming guard so it's never suppressed.
    NodeInfo info;
    info.hostname = data["hostname"].as<String>();
    info.isLeader = int(data["isLeader"]);
    knownNodes[from] = info;
    return;
  }
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
  announce_node_info();
}

void meshChangedConnectionsCallback(){
  Serial.println("Mesh: connections changed");
}

// Tells the rest of the mesh who we are (hostname + leader status), since
// mesh.getNodeList() only ever returns bare numeric node IDs. Sent on join,
// on every new connection, and periodically (see handle_mesh) so late
// joiners and reconnects stay in sync.
void announce_node_info(void){
  StaticJsonDocument<128> infoDoc;
  infoDoc["hostname"] = epdata.ap_name;
  infoDoc["isLeader"] = epdata.mesh_leader;
  mesh_broadcast_json("nodeInfo", infoDoc);
}

// Empty/absent targetNodeIds means "everyone", matching the pre-existing
// broadcast-to-all behavior - so it counts as including self too.
bool mesh_targets_include_self(JsonArray targetNodeIds){
  if(epdata.network_mode!=1){
    return true;
  }
  if(targetNodeIds.isNull() || targetNodeIds.size()==0){
    return true;
  }
  for(JsonVariant t : targetNodeIds){
    if(t.as<uint32_t>() == mesh.getNodeId()){
      return true;
    }
  }
  return false;
}

// Like mesh_broadcast_json, but when a specific target list is given, sends
// a point-to-point message to just those nodes instead of flooding everyone.
// Falls back to mesh_broadcast_json when the target list is empty/absent.
void mesh_send_to_targets(String typeTag, JsonVariant payload, JsonArray targetNodeIds){
  if(epdata.network_mode!=1){
    return;
  }
  if(meshApplyingIncoming){
    return;
  }
  if(targetNodeIds.isNull() || targetNodeIds.size()==0){
    mesh_broadcast_json(typeTag, payload);
    return;
  }
  String payloadStr;
  serializeJson(payload, payloadStr);
  String out = "{\"type\":\"" + typeTag + "\",\"src\":" + String(mesh.getNodeId()) + ",\"data\":" + payloadStr + "}";
  for(JsonVariant t : targetNodeIds){
    uint32_t targetId = t.as<uint32_t>();
    if(targetId != mesh.getNodeId()){
      mesh.sendSingle(targetId, out);
    }
  }
}

// Lists the currently-connected mesh nodes (self included) with whatever
// identity info we have for each - cross-referenced against
// mesh.getNodeList() so a node that's dropped off the mesh never lingers
// here, even if we never got a chance to forget its old nodeInfo.
void handleGetMeshNodes(void){
  server.on("/getMeshNodes", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument data(1024);
    JsonArray nodes = data.createNestedArray("nodes");

    JsonObject selfNode = nodes.createNestedObject();
    selfNode["nodeId"] = mesh.getNodeId();
    selfNode["hostname"] = epdata.ap_name;
    selfNode["isLeader"] = epdata.mesh_leader;
    selfNode["isSelf"] = 1;

    for(auto &&nodeId : mesh.getNodeList(false)){
      JsonObject node = nodes.createNestedObject();
      node["nodeId"] = nodeId;
      auto it = knownNodes.find(nodeId);
      if(it != knownNodes.end()){
        node["hostname"] = it->second.hostname;
        node["isLeader"] = it->second.isLeader;
      }else{
        node["hostname"] = String(nodeId);
        node["isLeader"] = 0;
      }
      node["isSelf"] = 0;
    }

    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
  });
}
