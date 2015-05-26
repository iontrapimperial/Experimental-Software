/* ///////////////////
// Writen by Graham //
//////////////////////

Pulse box. Does short pulses.

This should use port addressing as it is faster.
/////////////////// */

const int pulse_output_pin = 12;
const int pulse_button_pin = 2;

int pulse_length = 5;
int input_byte = 0;

volatile boolean do_a_pulse = false;

void pulse(unsigned int pl)
{
  digitalWrite(pulse_output_pin, HIGH);
  delayMicroseconds(pl);
  digitalWrite(pulse_output_pin, LOW);
  delay(1);
  do_a_pulse = false;
}

void establish_contact() {
 while (Serial.available() <= 0) {
   Serial.print('A');   // send a capital A
   delay(50);
 }
}

void setup() 
{  
  attachInterrupt(0, pulse_button_press, FALLING);
  
  pinMode(pulse_output_pin, OUTPUT);
  pinMode(pulse_button_pin, INPUT_PULLUP);
  
  digitalWrite(pulse_output_pin, LOW);
  
  Serial.begin(9600);
  establish_contact();
}  

//////////////////////

void loop() 
{ 
  if (Serial.available() > 0) 
  {
    // get incoming byte:
    input_byte = Serial.read();
    switch (input_byte) {
    case 'A':
      establish_contact();
      break;
    case 'B':
      pulse_length = 5;
      break;
    case 'C':
      pulse_length = 10;
      break;
    case 'D':
      pulse_length = 20;
      break;
    case 'E':
      pulse_length = 30;
      break;
    case 'F':
      pulse_length = 40;
      break;
    case 'G':
      pulse_length = 50;
      break;
    case 'H':
      do_a_pulse = true;
      break;
    default: 
      pulse_length = 5;
      do_a_pulse = false;
    }
  }
  if(do_a_pulse) pulse(pulse_length);
}

// Interupt functions

void pulse_button_press()
{
  do_a_pulse = true;
}
