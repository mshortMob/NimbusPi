#include <Arduino.h>
#include <button_funcs.h>
#include <eeprom_funcs.h>
#include <led_funcs.h>
#include <accelerometer_funcs.h>
#include <socket_server.h>
#include <http_server.h>

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  setup_eeprom();
  setup_leds();
  setup_ap(epdata.ssid, epdata.password);
  setup_http_server();
  setup_socket_server();
  setup_button();
  setup_accelerometer();
}

void loop() {
  int currentMode=handle_button();
  handle_leds(currentMode);
  handle_accelerometer();
  handle_socket_server();
}
