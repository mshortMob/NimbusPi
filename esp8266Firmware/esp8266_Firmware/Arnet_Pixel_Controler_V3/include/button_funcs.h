
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

void setup_button(void){
    pinMode(mode_button_pin, INPUT_PULLUP); 
    digitalWrite(mode_button_pin, HIGH); 
    previous_mode_button_value=digitalRead(mode_button_pin);
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