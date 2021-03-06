/** 
 * Quadrant photodiode reader
 * By Sandeep Mavadia and Dan Crick.  2012.
 * Serial part based on Serial Call-Response by Tom Igoe. 
 * 
 * Note: This sketch assumes that the device on the other end of the serial
 * port is going to send a single byte of value 65 (ASCII A) on startup.
 * The sketch waits for that byte, then sends an ASCII A whenever
 * it wants more data. 
 * 7/3/2012: Added markers to save diode positions
 */


import processing.serial.*;
Serial myPort;                       // The serial port
int[] serialInArray = new int[16];    // Where we'll put what we receive
int avgArraySize = 10;
float[] rPosYarray = new float [avgArraySize];
int serialCount = 0;                 // A count of how many bytes we receive
boolean firstContact = false;        // Whether we've heard from the microcontroller

int axpos, aypos;    // Axial position
int rxpos, rypos;  // Radial position
int mposxrad, mposyrad, mposxax, mposyax; //marker positions
int width = 512;
int height = 512;
int widtha = 512;
int heighta = 512;
color adotcolour = color(0, 128, 0);
color rdotcolour = color(128, 0, 0);
//PImage bg;
color bg = color(255,255,255);
int atl, atr, abl, abr;
int rtl, rtr, rbl, rbr, sumintensity;
PFont title_font;
PFont num_font;
int framenumber = 0;

void setup() {
  size(width, height);
  //  framerate(20);
  //bg = loadImage("tube_map_sm.jpg");  
  noStroke();  // No border on the next thing drawn
  // Set the starting position of the ball (middle of the stage)
  axpos = width/2;
  aypos = height/2;

  //Set the initial position of the marker to be in the centre of the page
  atr = 512;
  atl = 512;
  abl = 512;
  abr = 512;
  rtr = widtha/2;
  rtl = widtha/2;
  rbl = widtha/2;
  rbr = widtha/2;  

  for(int i =0;i<avgArraySize;i++){
    rPosYarray[i]=0;
  }
  title_font = loadFont("Tahoma-Bold-24.vlw");
  num_font = loadFont("Arial-BoldMT-24.vlw");

  // Print a list of the serial ports, for debugging purposes:
  println(Serial.list());

  // On Windows machines, this generally opens COM1.
  // Open whatever port is the one you're using.
  String portName = Serial.list()[1];
  myPort = new Serial(this, portName, 9600);
  
  
  //setup default marker positions
  mposxrad = 20;
  mposyrad = 20;
  mposxax = 20;
  mposyax = 20;
}





void serialEvent(Serial myPort) {
  // read a byte from the serial port:
  int inByte = myPort.read();
  // if this is the first byte received, and it's an A,
  // clear the serial buffer and note that you've
  // had first contact from the microcontroller. 
  // Otherwise, add the incoming byte to the array:
  if (firstContact == false) {
    if (inByte == 'A') { 
      myPort.clear();          // clear the serial port buffer
      firstContact = true;     // you've had first contact from the microcontroller
      myPort.write('A');       // ask for more
      print("reading");
    }
  } 
  else {
    // Add the latest byte from the serial port to array:
    serialInArray[serialCount] = inByte;
    serialCount++;


    // If we have 16 bytes:
    if (serialCount > 15 ) {
      
      rbr = 1024 - (serialInArray[0] + serialInArray[1]*256); //Into analog A08
      rtr = 1024 - (serialInArray[2] + serialInArray[3]*256); //Into analog A09
      rbl = 1024 - (serialInArray[4] + serialInArray[5]*256); //Into analog A10
      rtl = 1024 - (serialInArray[6] + serialInArray[7]*256); //Into analog A11
    sumintensity=rbr+rtr+rbl+rtl;
   
      //These numbers are set by the way the photodiode and the hardware boxes are linked to the analog inputs
      // print the values (for debugging purposes only):
      println("Radial "+ serialInArray[0] + "\t" + serialInArray[1] + "\t" + serialInArray[2] + "\t" + serialInArray[3] + "\t" + serialInArray[4] + "\t" + serialInArray[5] + "\t" + serialInArray[6] + "\t" + serialInArray[7]);
     // println("Axial "+serialInArray[8] + "\t" + serialInArray[9] + "\t" + serialInArray[10] + "\t" + serialInArray[11] + "\t" + serialInArray[12] + "\t" + serialInArray[13] + "\t" + serialInArray[14] + "\t" + serialInArray[15]);
      println(rbr + "\t" + rtr + "\t" + rbl + "\t" + rtl);
      // Send a capital A to request new sensor readings:
      myPort.write('A');
      // Reset serialCount:
      serialCount = 0;
    }
  }
}






void draw() {

  background(bg);

  // Crosshairs
  stroke(127);
  strokeWeight(1);
  line(0, widtha/2, widtha, widtha/2);
  line(widtha/2, 0, widtha/2, widtha);

  //calculate the coordinates for the beam centre
  axpos = 512 + ((atr + abr) - (atl + abl))/4;
  aypos = 512 - ((atl + atr) - (abl + abr))/4;

  //rxpos = (2*widtha + ((rtr + rbr) - (rtl + rbl)))/4;
  float abs = 0;
  float ord =0;
  float TR = rtr;
  float TL = rtl;
  float BR = rbr;
  float BL = rbl;
  if(TR+TL+BL+BR!=0)
  {
    //abs = widtha/2 + ((TR-TL)/(TR+TL)+(BR-BL)/(BR+BL))*widtha/4;
    abs = widtha/2 + (TR+BR-TL-BL)/(TR+BR+TL+BL)*widtha/4;
    ord = widtha/2 - (TR+TL-BR-BL)/(TR+BR+TL+BL)*widtha/4;
  }
  else abs = 0;
  println(abs);
  rxpos = round(abs);
  //rypos = round(ord);
  
  float ryposTemp =0;
  for(int i = 0;i<avgArraySize-1;i++){
   ryposTemp += rPosYarray[i];
   rPosYarray[i]=rPosYarray[i+1]; 
  }
  rPosYarray[avgArraySize-1]=ord;
  rypos = round((ryposTemp +rPosYarray[avgArraySize-1])/(avgArraySize));
  
  //rypos = (2*widtha - ((rtl + rtr) - (rbl + rbr)))/4;
  if (framenumber == 0) {
    //DrawAxial();  
    DrawRadial();      
    framenumber = 1;
  }
  else {
    DrawRadial();  
    //DrawAxial();      
    framenumber = 0;
  }
  
  DrawMarkers();
  
  delay(100); //wait 100ms before redrawing
}

void DrawMarkers()
{
  //draw buttons
  //fill( color(128, 255, 0) );  
  //rect(100, 750, 100, 35); //axial button
  
  fill(rdotcolour);
  rect(3*widtha/4-32, 7*widtha/8-12, 64, 24);  //radial button
  
  fill( color(255, 255, 255) );  
  //text("Mark",150,780);
  textFont(title_font, 16);
  textAlign(CENTER, CENTER);
  text("Mark",3*widtha/4,7*widtha/8);
  
  stroke(255); //bright white
  strokeWeight(3); //3 pixels thick
  
  //axial marker
  //line(mposxax-20, mposyax-20, mposxax+20, mposyax+20);
  //line(mposxax-20, mposyax+20, mposxax+20, mposyax-20);
  
  //radial marker
  line(mposxrad-20, mposyrad-20, mposxrad+20, mposyrad+20);
  line(mposxrad-20, mposyrad+20, mposxrad+20, mposyrad-20);
  
  stroke(127);
  strokeWeight(1); //reset lines
  
  //fill( color(128, 255, 0) );  
  //rect(mposxax+20, mposyax+20, 10, 10);
  
  fill( color(255, 0, 255) );
  rect(mposxrad+10, mposyrad+10, 4, 4);
}


void mousePressed()
{
  //is mouse over axial button?
  //if(overRect(100, 750, 100, 35)) 
  //{
  //    mposxax =  axpos ;
  //    mposyax = aypos;
  //}
  //else 
  if(overRect(3*widtha/4-32, 7*widtha/8-12, 64, 24))
  {
      mposxrad =  rxpos ;
      mposyrad = rypos;
  }

}

boolean overRect(int x, int y, int width, int height) 
{
  if (mouseX >= x && mouseX <= x+width && 
      mouseY >= y && mouseY <= y+height) {
    return true;
  } else {
    return false;
  }
}


void DrawAxial() {
  /////////////////////////////////
  // Draw the axial display

  fill(adotcolour);


  textFont(num_font, 48);
  text(atr, 700, 270);    // Axial Top Right
  text(atl, 300, 270);
  text(abl, 300, 670);
  text(abr, 700, 670);

  text("axpos", 120, 950);
  text(axpos, 120, 920);
  text("aypos", 300, 950);
  text(aypos, 300, 920);

  // Total intensity
  int a_total = atr + atl + abr + abl;
  text(a_total, 200, 870);

  // Moving circle
  ellipse(axpos, aypos, 25, 25);

  // Text label
  textAlign(CENTER);
  textFont(title_font, 32);
  text("Axial Detector", 200, 830);
}



void DrawRadial() {
  /////////////////////////////////  
  // Draw the radial display
  fill(rdotcolour);
  textFont(num_font, 24);
  textAlign(CENTER, CENTER);
  // Print the absolute intensities:
  text(rtr, 3*widtha/4, widtha/4);    // Radial Top Right
  text(rtl, widtha/4, widtha/4);
  text(rbl, widtha/4, 3*widtha/4);
  text(rbr, 3*widtha/4, 3*widtha/4);
  
  // Total intensity
  int r_total = rtr + rtl + rbr + rbl;
  textFont(num_font, 16);
  textAlign(CENTER, CENTER);
  text("Total: ", widtha/4-24, 7*widtha/8);
  text(r_total, widtha/4+24, 7*widtha/8);

  textFont(num_font, 16);
  textAlign(CENTER, CENTER);
  text("x pos", widtha/4-32, 7.5*widtha/8);
  text(rxpos, widtha/4-32, 7.75*widtha/8);
  text("y pos", widtha/4+32, 7.5*widtha/8);
  text(rypos, widtha/4+32, 7.75*widtha/8);

  // Moving circle
  ellipse(rxpos, rypos, 16, 16);

  // Text label
  //textAlign(CENTER);
  //textFont(title_font, 32);
  //text("Radial Detector", 800, 830);
}





/*

 //  Serial comms for Quadrant photodiode.
 //  By Dan Crick.  2012.
 //  Based on Serial Call and Response by Tom Igoe
 //  Language: Wiring/Arduino
 //
 
 int Sensor[4] = {0,0,0,0};    // Analog sensors
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
 for (int i = 0; i <= 3; i++) {
 // read analog input, divide by 4 to make the range 0-255:
 Sensor[i] = analogRead(i+11)/4;
 // delay 10ms to let the ADC recover:
 delay(10);
 }
 // send sensor values:
 for (int i = 0; i <= 3; i++) {    
 Serial.write(Sensor[i]);
 }
 }
 }
 
 void establishContact() {
 while (Serial.available() <= 0) {
 Serial.write('A');   // send a capital A
 delay(300);
 }
 }
 
 
 
 
 
 
 
 
 
 */
