#include <Arduino.h>
#include <eeprom_funcs.h>
#include <button_funcs.h>
#include <led_funcs.h>
#include <artnet_funcs.h>
#include <accelerometer_funcs.h>
#include <socket_server.h>
#include <http_server.h>
#include <lan_peers_funcs.h>
#include <mesh_funcs.h>
#include <ota_funcs.h>

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  setup_eeprom();
  setup_button();
  setup_accelerometer();
  setup_leds();
  if(epdata.network_mode==1){
    setup_mesh();
  }else{
    wifiConnected=setup_ap(epdata.ssid, epdata.password, epdata.ap_name);
  }
  setup_http_server();
  setup_lan_peers();
  setup_socket_server();
  setup_ota();
}

void loop() {
  buttonState button_state=handle_button(wifiConnected);
  if(epdata.network_mode==1){
    handle_mesh();
    if(button_state.buttonWasPressed){
      StaticJsonDocument<16> modeDoc;
      modeDoc.set(selectedMode);
      mesh_broadcast_json("selectedMode", modeDoc);
    }
  }else{
    wifiConnected=handle_ap( wifiConnected, button_state.buttonWasPressed);
    handle_lan_peers();
  }
  handle_leds(button_state.selectedMode);
  handle_accelerometer();
  handle_socket_server();
  if(epdata.network_mode==0){
    handle_artnet();
  }
  handle_ota();
}
