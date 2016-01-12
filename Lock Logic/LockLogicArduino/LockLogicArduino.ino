// Lock Logic Serial
// By Graham Stutter (2014)
// Serial comm. based on Serial Call and Response by Tom Igoe.  
//
// Simple code to read laser lock state and control
// corresponding LEDs. Also communicates with processing code.
//
 
// 729 laser is referred to as S29 in this code,
// because it doesn't like variable names that start with a number.

int B1PDHin = 22;
int B2PDHin = 24;
int BSCin = 26;
int RSCin = 28;
int S29PDHin = 30;

int B1PDHon = 53;
int B1PDHoff = 51;

int B2PDHon = 49;
int B2PDHoff = 47;

int BSCon = 45;
int BSCoff = 43;

int RSCon = 41;
int RSCoff = 39;

int S29PDHon = 37;
int S29PDHoff = 35;

byte OutputByte = B00010001;
int InputByte = 0;         // incoming serial byte

// the setup routine runs once when you press reset:
void setup() {
  pinMode(B1PDHin, INPUT);
  pinMode(B2PDHin, INPUT);
  pinMode(BSCin, INPUT);
  pinMode(RSCin, INPUT);
  pinMode(S29PDHin, INPUT);
  
  pinMode(B1PDHon, OUTPUT);
  pinMode(B1PDHoff, OUTPUT);  
  pinMode(B2PDHon, OUTPUT);
  pinMode(B2PDHoff, OUTPUT);
  pinMode(BSCon, OUTPUT);
  pinMode(BSCoff, OUTPUT);
  pinMode(RSCon, OUTPUT);
  pinMode(RSCoff, OUTPUT);
  pinMode(S29PDHon, OUTPUT);
  pinMode(S29PDHoff, OUTPUT);
  
  Serial.begin(9600);
  establishContact();
}


void loop() {
  digitalWrite(B1PDHon, digitalRead(B1PDHin));
  digitalWrite(B1PDHoff, !digitalRead(B1PDHin));
  
  digitalWrite(B2PDHon, digitalRead(B2PDHin));
  digitalWrite(B2PDHoff, !digitalRead(B2PDHin));
  
  digitalWrite(BSCon, digitalRead(BSCin));
  digitalWrite(BSCoff, !digitalRead(BSCin));
  
  digitalWrite(RSCon, digitalRead(RSCin));
  digitalWrite(RSCoff, !digitalRead(RSCin));
   
  digitalWrite(S29PDHon, digitalRead(S29PDHin));
  digitalWrite(S29PDHoff, !digitalRead(S29PDHin));
  
  if (Serial.available() > 0) 
  {
    // get incoming byte:
    InputByte = Serial.read();
    if(InputByte == 'A') {
      if(digitalRead(B1PDHin)) {
        OutputByte = OutputByte | B00000001;
      }
      else {
        OutputByte = OutputByte & B11111110;
      }
      if(digitalRead(B2PDHin)) {
        OutputByte = OutputByte | B00000010;
      }
      else {
        OutputByte = OutputByte & B11111101;
      }
      if(digitalRead(BSCin)) {
        OutputByte = OutputByte | B00000100;
      }
      else {
        OutputByte = OutputByte & B11111011;
      }
      if(digitalRead(RSCin)) {
        OutputByte = OutputByte | B00001000;
      }
      else {
        OutputByte = OutputByte & B11110111;
      }
      if(digitalRead(S29PDHin)) {
        OutputByte = OutputByte | B00010000;
      }
      else {
        OutputByte = OutputByte & B11101111;
      }
    }
    else if (InputByte == 'B') {
      establishContact();      
    }
   
    Serial.write(OutputByte);
    delay(100); 
  }
}

void establishContact() {
 while (Serial.available() <= 0) {
   Serial.print('A');   // send a capital A
   delay(300);
 }
}

