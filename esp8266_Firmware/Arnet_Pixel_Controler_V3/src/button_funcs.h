
int mode_button_pin=13;
int previous_mode_button_value=1;
int selectedMode;

void cycle_mode(void){
  selectedMode=(selectedMode+1)%4;
}

void setup_button(void){
    pinMode(mode_button_pin, INPUT_PULLUP); 
    digitalWrite(mode_button_pin, HIGH); 
    previous_mode_button_value=digitalRead(mode_button_pin);
}

int handle_button(){
  if(previous_mode_button_value!=digitalRead(mode_button_pin)){
    previous_mode_button_value=digitalRead(mode_button_pin);
    if(digitalRead(mode_button_pin)==0){
      cycle_mode();
    }
  }
  return selectedMode;
}
