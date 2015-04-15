//  Serial comms for Quadrant photodiode.
//  By Dan Crick.  2012.
//  Based on Serial Call and Response by Tom Igoe
//  Language: Wiring/Arduino
//

int Sensor[8] = {0,0,0,0,0,0,0,0};    // Analog sensors
int inByte = 0;         // incoming serial byte

void setup()
{
  // start serial port at 9600 bps:
  Serial.begin(9600);
  establishContact();  // send a byte to establish contact until Processing responds 
}

void loop()
{
  // if we get a valid byte, read analog ins:
  if (Serial.available() > 0) {
    // get incoming byte:
    inByte = Serial.read();
    for (int i = 0; i <= 7; i++) { //Using analog ports A8 --> A15
      // read analog input, divide by 4 to make the range 0-255:
      Sensor[i] = analogRead(i+8);
      // delay 10ms to let the ADC recover:
      delay(10);
    }
    // send sensor values:
    for (int i = 0; i <= 7; i++) {    
      Serial.write(Sensor[i]);
      Serial.write(Sensor[i] >> 8);
    }
  }
}

void establishContact() {
 while (Serial.available() <= 0) {
      Serial.print('A');   // send a capital A
      delay(300);
  }
}





