#include "ap_funcs.h" 
// #include <WebSocketsServer_Generic.h>
#include <Hash.h>
#include <ArduinoJson.h>


// String old_value, value;
// int lastSocketSendTime=0;

// WebSocketsServer    webSocket = WebSocketsServer(81);

// void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
//   switch (type) {
//     case WStype_DISCONNECTED:
//       Serial.printf("[%u] Disconnected!\n", num);
//       break;

//     case WStype_CONNECTED: {
//         IPAddress ip = webSocket.remoteIP(num);
//         Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
//         // send message to client
//         webSocket.sendTXT(num, "0");
//       }
//       break;

//     case WStype_TEXT:
//       Serial.printf("[%u] get Text: %s\n", num, payload);
//       cycle_mode();
//       // send message to client
//       // webSocket.sendTXT(num, "message here");
//       // send data to all connected clients
//       // webSocket.broadcastTXT("message here");
//       break;
      
//     case WStype_BIN:
//       Serial.printf("[%u] get binary length: %u\n", num, length);
//       hexdump(payload, length);
//       // send message to client
//       // webSocket.sendBIN(num, payload, length);
//       break;
//   }

// }

// void setup_socket_server(){
//   webSocket.begin();
//   webSocket.onEvent(webSocketEvent);
// }

// void handle_socket_server(){
//   webSocket.loop();
//   if( (millis()-lastSocketSendTime) > 500 ){
//     value = (String) analogRead(0);
//     webSocket.broadcastTXT(value); 
//     lastSocketSendTime=millis();
//   }
// }
