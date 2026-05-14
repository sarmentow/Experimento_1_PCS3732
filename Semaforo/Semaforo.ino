const int green_pin =  2;
const int red_pin   =  3;
const int yellow_pin=  4;

int state = 0;

void setup() {
  pinMode(green_pin, OUTPUT);
  pinMode(red_pin, OUTPUT);
  pinMode(yellow_pin, OUTPUT);
}

void green_state(){
  digitalWrite(green_pin, HIGH);
  digitalWrite(red_pin, LOW);
  digitalWrite(yellow_pin, LOW);
  delay(3000);
}

void red_state(){
  digitalWrite(green_pin, LOW);
  digitalWrite(red_pin, HIGH);
  digitalWrite(yellow_pin, LOW);
  delay(4000);
}

void yellow_state(){
  digitalWrite(green_pin, LOW);
  digitalWrite(red_pin, LOW);
  digitalWrite(yellow_pin, HIGH);
  delay(1000);
}

void loop() {
  switch(state){
    case 1:
      green_state();
      state++;
      break;
    case 2:
      red_state();
      state++;
      break;
    case 3:
      yellow_state();
      state=1;
      break;
    default:
      state=1;
      break;
  }
}
