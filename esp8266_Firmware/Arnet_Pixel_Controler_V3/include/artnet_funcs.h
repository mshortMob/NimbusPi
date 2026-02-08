#include <ArtnetWifi.h>

bool enableArtnet=true;

ArtnetWifi artnet;

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data){
    int filterUniverse=atoi(epdata.universe);
    int filterChan=atoi(epdata.startChan)-1;
    int fixtureMode=atoi(epdata.fixtureMode);
    Serial.print("Filtering for universe:");
    Serial.println(filterUniverse);
    if(universe==filterUniverse){
      Serial.print("Received artnet data for universe:");
      Serial.println(universe);
      setLEDSForArtnet(data, filterChan, fixtureMode); 
    }
  }

void handle_artnet(){
    if(enableArtnet){
        artnet.read();
    }
}

void setup_artnet(){
    if(enableArtnet){
        artnet.begin();
        artnet.setArtDmxCallback(onDmxFrame);
    }
}