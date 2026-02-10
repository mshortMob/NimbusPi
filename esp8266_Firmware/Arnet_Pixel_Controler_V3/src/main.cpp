#include <Arduino.h>
#include <eeprom_funcs.h>
#include <button_funcs.h>
#include <led_funcs.h>
#include <artnet_funcs.h>
#include <accelerometer_funcs.h>
#include <socket_server.h>
#include <http_server.h>

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  setup_eeprom();
  setup_button();
  setup_accelerometer();
  setup_leds();
  wifiConnected=setup_ap(epdata.ssid, epdata.password);
  setup_http_server();
  setup_socket_server();
}

void loop() {
  buttonState button_state=handle_button(wifiConnected);
  wifiConnected=handle_ap( wifiConnected, button_state.buttonWasPressed);
  handle_leds(button_state.selectedMode, wifiConnected);
  handle_accelerometer();
  handle_socket_server();
  handle_artnet();
}
