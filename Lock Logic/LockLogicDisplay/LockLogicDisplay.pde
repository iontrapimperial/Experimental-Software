// Lock Logic Serial Reader
// By Graham Stutter (2014)
// Serial comm. based on Serial Call-Response by Tom Igoe. 
// 
// This is a quick and dirty bit of code to display 
// the laser lock states on the experiment PC.
// Also displayed is the last time that any particular 
// laser was out of lock.
//
// I am certain that this code could be subtantially 
// cleaned up with for() loops, but I CBA'ed 
// 
// This sketch assumes that the lock logic arduino will 
// send a byte of 'A' on startup. The sketch waits for 
// that byte, then sends an 'A' whenever it wants more data.
// In setup() this sketch sends a 'B' to arduino make sure 
// connection is established.
//

import processing.serial.*;
Serial myPort;                       // The serial port
int SerialIn = 0;    // Where we'll put what we receive
int a=0;
boolean firstContact = false;        // Whether we've heard from the microcontroller

boolean B1 = false;
boolean B2 = false;
boolean BSC = false;
boolean RSC = false;
boolean S29 = false;

int B1hour = 0;
int B2hour = 0;
int BSChour = 0;
int RSChour = 0;
int S29hour = 0;

int B1min = 0;
int B2min = 0;
int BSCmin = 0;
int RSCmin = 0;
int S29min = 0;

String B1time = "0";
String B2time = "0";
String BSCtime = "0";
String RSCtime = "0";
String S29time = "0";

int width = 256;
int height = 384;
PFont title_font;
color fontcolour = color(0, 128, 128);
color b = color(0, 0, 0);
color g = color(0, 128, 0);
color r = color(128, 0, 0);

void setup() {
  size(width, height);
  title_font = loadFont("ComicSansMS-Bold-48.vlw");
  
  println(Serial.list());

  myPort = new Serial(this, "COM6", 9600);
  
  myPort.write('B');
}

void serialEvent(Serial myPort) {
  
  // read a byte from the serial port:
  int InputByte = myPort.read();
  
  if (firstContact == false) {
    if (InputByte == 'A') { 
      myPort.clear();          // clear the serial port buffer
      firstContact = true;     // you've had first contact from the microcontroller
      myPort.write('A');       // ask for more
      print("reading");
    }
  } 
  else {
    SerialIn = InputByte;
    
    // Send a capital A to request new sensor readings:
    myPort.write('A');
  }
}

void draw() {
  background(255);
  
  a = SerialIn & 1;
  if(a == 1) B1 = true;
  else {
    B1 = false;
    B1hour=hour();
    B1min=minute();
  }
  
  a = SerialIn & 2;
  if(a == 2) B2 = true;
  else {
    B2 = false;
    B2hour=hour();
    B2min=minute();
  }
  
  a = SerialIn & 4;
  if(a == 4) BSC = true;
  else {
    BSC = false;
    BSChour=hour();
    BSCmin=minute();
  }
  
  a = SerialIn & 8;
  if(a == 8) RSC = true;
  else {
    RSC = false;
    RSChour=hour();
    RSCmin=minute();
  }
  
  a = SerialIn & 16;
  if(a == 16) S29 = true;
  else {
    S29 = false;
    S29hour=hour();
    S29min=minute();
  }
  
  if(B1) fill(g);
  else fill(r);
  ellipse(32, 96, 16, 16);
  
  if(B2) fill(g);
  else fill(r);
  ellipse(32, 160, 16, 16);
  
  if(BSC) fill(g);
  else fill(r);
  ellipse(32, 224, 16, 16);
  
  if(RSC) fill(g);
  else fill(r);
  ellipse(32, 288, 16, 16);
  
  if(S29) fill(g);
  else fill(r);
  ellipse(32, 352, 16, 16);
  
  
  fill(fontcolour);
  textAlign(CENTER);
  textFont(title_font, 42);
  text("Laser Locks", 128, 48);
  
  
  textAlign(LEFT);
  textFont(title_font,32);
  text("B1", 64, 108);
  text("B2", 64, 172);
  text("BSC", 64, 236);
  text("RSC", 64, 300);
  text("729", 64, 364);
  
  B1time = nf(B1hour,2) + ":" + nf(B1min,2);
  B2time = nf(B2hour,2) + ":" + nf(B2min,2);
  BSCtime = nf(BSChour,2) + ":" + nf(BSCmin,2);
  RSCtime = nf(RSChour,2) + ":" + nf(RSCmin,2);
  S29time = nf(S29hour,2) + ":" + nf(S29min,2);
  
  fill(b);
  textAlign(LEFT);
  textFont(title_font, 20);
  text(B1time, 160, 102);
  text(B2time, 160, 166);
  text(BSCtime, 160, 230);
  text(RSCtime, 160, 294);
  text(S29time, 160, 358);
  
  delay(100); //wait 100ms before redrawing
}
