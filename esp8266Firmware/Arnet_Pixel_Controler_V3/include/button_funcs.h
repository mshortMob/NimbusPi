
int mode_button_pin=13;
int previous_mode_button_value=1;
int selectedMode;
bool buttonWasPressed = false;

struct buttonState {
  int selectedMode=0;
  bool buttonWasPressed=false;
};

void cycle_mode(void){
  selectedMode=(selectedMode+1)%5;
  Serial.println("SelectedMode: "+ (String) selectedMode);
}

void check_for_startup_button_hold(int initial_mode_button_value){
    int timeOfStartup=millis();
    bool has_changed_after_initial_value=false;
    Serial.println();
    Serial.println("Initial Button Value:" + String(initial_mode_button_value));
    Serial.println();
    if(initial_mode_button_value==0){
      while( (millis()-timeOfStartup) < 3000 && !has_changed_after_initial_value ){
        if(digitalRead(mode_button_pin)==1){
          has_changed_after_initial_value=true;
          Serial.println("Long Button Hold on Startup Detected, Resetting EEPROM ...");

        }else{
          Serial.println("Button Still Held ...");
          delay(50);
        }
      }
      if(!has_changed_after_initial_value){
        reset_eeprom_to_initial_values();
      }
    }
}

void setup_button(void){
    pinMode(mode_button_pin, INPUT_PULLUP); 
    digitalWrite(mode_button_pin, HIGH); 
    previous_mode_button_value=digitalRead(mode_button_pin);
    check_for_startup_button_hold(previous_mode_button_value);

}

bool interupt_startup_routine(){
  bool shouldBreakLoop=false;
  if(previous_mode_button_value!=digitalRead(mode_button_pin)){
    previous_mode_button_value=digitalRead(mode_button_pin);
    if(digitalRead(mode_button_pin)==0){
      shouldBreakLoop=true;
    }
  }
  return shouldBreakLoop;
}

buttonState handle_button(bool wifiConnected){
  buttonState bs;
  buttonWasPressed=false;
  if(previous_mode_button_value!=digitalRead(mode_button_pin)){
    previous_mode_button_value=digitalRead(mode_button_pin);
    if(digitalRead(mode_button_pin)==0){
      cycle_mode();
      buttonWasPressed=true;
    }
  }
  bs.buttonWasPressed=buttonWasPressed;
  bs.selectedMode=selectedMode;
  return bs;
}