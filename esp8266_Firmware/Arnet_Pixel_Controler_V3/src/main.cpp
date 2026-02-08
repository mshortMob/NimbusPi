#include <Arduino.h>
#include <button_funcs.h>
#include <eeprom_funcs.h>
#include <led_funcs.h>
#include <artnet_funcs.h>
#include <accelerometer_funcs.h>
#include <socket_server.h>
#include <http_server.h>

void setup() {
  setup_button();
  setup_accelerometer();
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  setup_eeprom();
  setup_leds();
  wifiConnected=setup_ap(epdata.ssid, epdata.password);
  setup_http_server();
  setup_socket_server();
}

void loop() {
  int currentMode=handle_button();
  handle_leds(currentMode, wifiConnected);
  handle_accelerometer();
  handle_socket_server();
  handle_artnet();  
}
