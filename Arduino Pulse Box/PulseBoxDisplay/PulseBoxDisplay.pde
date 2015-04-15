// Shit Arduino Pulse Box Serial
// By Graham Stutter (2014)
// Serial comm. based on Serial Call-Response by Tom Igoe. 
// 
// This sketch assumes that the lock logic arduino will 
// send a byte of 'A' on startup. The sketch waits for 
// that byte, then sends an 'A' whenever it wants more data.
// In setup() this sketch sends a 'A' to arduino make sure 
// connection is established.
//

import processing.serial.*;
Serial myPort;       // The serial port

int SerialIn = 0; 
int a = 0;
int small_button_size = 64;
int[] buttons_x = new int[6];
int buttons_y;
int button_pulse_x, button_pulse_y, button_pulse_width, button_pulse_height;
char[] output_codes = {'B','C','D','E','F','G','H'};
String[] labels = {"5 us", "10 us", "20 us","30 us","40 us","50 us"};
boolean first_contact = false;
boolean[] mouse_over = new boolean[6];
boolean mouse_over_pulse = false;

PFont title_font;
color fontcolour = color(0, 128, 128);
color b = color(0, 0, 0);
color g = color(0, 128, 0);
color r = color(128, 0, 0);

void setup() {
  size(512, 320);
  for(int i=0; i<6; i++){
    buttons_x[i] = 4 + (2*i + 1)*(width-8)/12 - small_button_size/2;
  }
  buttons_y = 96;
  button_pulse_width = small_button_size * 4;
  button_pulse_height = small_button_size;
  button_pulse_x = width/2 - button_pulse_width/2;
  button_pulse_y = buttons_y + 3*small_button_size/2;
  
  for(boolean i : mouse_over){
    i = false;
  }
  
  title_font = loadFont("ComicSansMS-Bold-48.vlw");
  
  println(Serial.list());

  myPort = new Serial(this, "COM4", 9600);
  
  myPort.write('A');
  
  
}

void serialEvent(Serial myPort) {
  
  // read a byte from the serial port:
  int InputByte = myPort.read();
  
  if (first_contact == false) {
    if (InputByte == 'A') { 
      myPort.clear();          // clear the serial port buffer
      first_contact = true;     // you've had first contact from the microcontroller
      myPort.write('A');       // ask for more
      print("reading");
    }
  }
}

void draw() {
  mouse_update(mouseX, mouseY);
  if(mouse_over[0]||mouse_over[1]||mouse_over[2]||mouse_over[3]||mouse_over[4]||mouse_over[5]||mouse_over_pulse){
    cursor(HAND);
  }else cursor(ARROW);
  
  background(255);
  
  fill(fontcolour);
  textAlign(CENTER);
  textFont(title_font, 42);
  text("Shit Arduino Pulse Box", width/2, 48);
  
  for(int i : buttons_x){
  rect(i, buttons_y, small_button_size, small_button_size);
  }
  
  rect(button_pulse_x, button_pulse_y, button_pulse_width, button_pulse_height);
  
  fill(color(0));
  textAlign(CENTER,CENTER);
  textFont(title_font, 24);
  text("PULSE", width/2, button_pulse_y + small_button_size/2);
  textFont(title_font, 18);
  for(int i=0; i<6; i++) {
    text(labels[i], buttons_x[i] + small_button_size/2, buttons_y + small_button_size/2);
  }
}

void mouse_update(int x, int y) {
  for(int i=0; i < 6; i++){
    mouse_over[i] = over_rect(buttons_x[i], buttons_y, small_button_size, small_button_size, x, y);
  }
  mouse_over_pulse = over_rect(button_pulse_x, button_pulse_y, button_pulse_width, button_pulse_height, x, y);
}

void mousePressed() {
  for(int i=0; i<6; i++){
  if (mouse_over[i]) myPort.write(output_codes[i]);
  }
  if (mouse_over_pulse) myPort.write(output_codes[6]);
}

boolean over_rect(int x, int y, int rect_width, int rect_height, int mouse_x, int mouse_y)  {
  if (mouse_x >= x && mouse_x <= x+rect_width && 
      mouse_y >= y && mouse_y <= y+rect_height) {
    return true;
  } else {
    return false;
  }
}
