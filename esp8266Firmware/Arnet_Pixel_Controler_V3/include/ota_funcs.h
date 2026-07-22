#include <ArduinoOTA.h>
#include <FS.h>

void setup_ota(){
  ArduinoOTA.setHostname(epdata.ap_name);
  ArduinoOTA.setPassword("NimbusPiOTA");
  ArduinoOTA.onStart([]() {
    if (ArduinoOTA.getCommand() == U_FS) {
      SPIFFS.end();
    }
    Serial.println("OTA update starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA update complete");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA progress: %u%%\r", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error [%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready");
}

void handle_ota(){
  ArduinoOTA.handle();
}
