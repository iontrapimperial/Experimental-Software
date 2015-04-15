/*
  ////////////////////////////////////
  // Writen by Sean D, Dan C & Joe G//
  ////////////////////////////////////
 
 This arduino controls two DACs, which feed into the PZT controllers for the 397 nm lasers' PDH lock cavities
 
 ///////////////////////////////
 */

#define A 32768         // Half of the full output of the DAC
#define WINDOW 50000
#define LOCKSWITCH 7
#define RESET 17 //Integrator reset switch pin
#define LOCKLED 4
#define R2FIRST 5 //Define pins to check which laser comes first in scan (if both pins low, look for only one laser)
#define R5FIRST 6
#define R2DIR 15  //Define pins to choose set point increment direction (up/down)
#define R5DIR 14
#define DELTA 20  //Define step to increment set points by

int NSLAVE = 0;

// Define timing counters
// They must be volatile because they are changed within an interupt function.
volatile unsigned int hene[2] = {0};
volatile unsigned int red[4] = {0};

volatile unsigned int hcount = 0;
volatile unsigned int rcount = 0;

volatile char readin = 0; //Read/write cycle indicator

int peak = 0; //Peak number index

unsigned int setpoint[2] = {0};
float heneset; //HeNe peak separation setpoint for amplitude/pressure drift scaling
int incstep[2] = {0}; //Array to define set point increment direction for each laser
int error = 0;
float P = 0; //Proportional gains for each laser
float I = 0; //Integral gains for each laser
long integral[2] = {0}; //Integrator for each laser
long output = 0; //DAC output for each laser

char lockcheck = 0; //Checks whether to light 'locked' LED
char lockengage = 0; //Checks whether lock is ready to be engaged, conditioned on finding enough peaks
char resetflag = 1; //0=lock run and not yet reset, 1 = lock reset or not yet run

//Reverses bit order of a byte, in this case to correct for backwards wiring of PORTA for PCB layout convenience 
static const unsigned char BitReverseTable256[] = 
{
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() 
{  
  TCCR1A = 0x00;    // CONTROL REGISTER 1A  
  TCCR1B = 0x01;    // CONTROL REGISTER 1B
  // Clock pre-multipliers: 0x01 = clock/1  . 0.05 micro increments. 3.28ms overflow.
  //                        0x02 = clock/8  . 0.4 micro increments. 26.2ms overflow.
  //                        0x03 = clock/64 . 3.2 micro increments. 209.7ms overflow.  
  TIMSK1 = 0x00;    // INTERRUPT MASK REGISTER  
  TIMSK0 = 0;       // Timer/Counter Interrupt Mask Register (Turns of timer interrupts...which seem to be turned on by the Arduino)
  attachInterrupt(3, trigger_up, RISING);       // TRIGGER PULSE INTERRUPTS (PIN 20)
  attachInterrupt(4, trigger_down, FALLING);       // TRIGGER PULSE INTERRUPTS (PIN 19)
  attachInterrupt(5, henepulse, RISING);            // HENE PULSE INTERRUPTS (PIN 18)
  attachInterrupt(2, redpulse, RISING);            // RED PULSE INTERRUPT (PIN 21)
  attachInterrupt(0, R2inc, RISING);            // RED 2 SET POINT INCREMENT INTERRUPT (PIN 2)
  attachInterrupt(1, R5inc, RISING);            // RED 5 SET POINT INCREMENT INTERRUPT (PIN 3)
  
  DDRA = 255;          // Set ports A and C as output pins
  DDRC = 255; 
  DDRB = 255;          // Set port B as outputs 
  DDRK = 255;          // Set ports K and F as output pins
  DDRF = 255;

  pinMode(LOCKSWITCH, INPUT);
  pinMode(LOCKLED,OUTPUT);   // Set 'locked' led pin as output
  pinMode(R2FIRST, INPUT); //Set laser ordering pins as inputs
  pinMode(R5FIRST, INPUT);
  pinMode(R2DIR, INPUT); //Set increment direction pins as inputs
  pinMode(R5DIR, INPUT);
  pinMode(RESET, INPUT); //Set integrator reset pin as input

  //Set number of slave lasers, if R1/R2 ordering switch is in center ('neither first'), expect one slave only
  if (digitalRead(R2FIRST) == HIGH || digitalRead(R5FIRST) == HIGH) NSLAVE = 2;
  else NSLAVE = 1;
  
 //set gains for each laser
 P = 0;
 I = 0.01;
}


void loop()
{  
  if (digitalRead(LOCKSWITCH) == LOW && resetflag == 1)  //When not yet locked, or after reset, try to constantly update setpoints to current positions
  {
    for (int slavenum = 0; slavenum < NSLAVE; slavenum++)
    {
      if (rcount == NSLAVE & hcount == 2) //Do not assign new setpoints unless correct number of peaks found
      {
        if (digitalRead(R5FIRST) == HIGH) peak = NSLAVE - 1 - slavenum;     //If red 5 comes first in scan, assign setpoints in reverse order slavenum now always refers to the laser, not the peak number)
        else peak = slavenum;  //Otherwise, when red 2 is first, or there is only one laser, assign in normal order
        setpoint[slavenum] = red[peak];
        heneset = hene[1]-hene[0]; //Set nominal HeNe separation to correctly scale future scans as amplitude or pressure drifts
        lockengage = 1;  //Allow lock to switch on once set points are assigned
      }
      else lockengage = 0; //If insufficient peaks detected, lock cannot be engaged
      error = 0;    //reset error counters and integrators, set DACs to midpoint 5V
      integral[slavenum] = 0;
      writeVoltage(slavenum, A);
    }
    digitalWrite(LOCKLED,LOW);    
  }
  else if (digitalRead(LOCKSWITCH) == LOW) //If lock switched off but not reset allow DAC to hold at existing value, but switch off LED
  {
    digitalWrite(LOCKLED, LOW);
  }
  if (digitalRead(RESET) == HIGH) resetflag = 1; //If reset clicked, enable reset of setpoints and DAC outputs
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void updateDACs()
{
  resetflag = 0; //Flags current state as 'running' - when lock switched off will hold current output values and setpoints until reset
  
  lockcheck = 1;
  
  for (int slavenum = 0; slavenum < NSLAVE; slavenum++)
  {
    if (digitalRead(R5FIRST) == HIGH) peak = (NSLAVE - 1 - slavenum);
    else peak = slavenum;
    
    red[peak]=(unsigned int)(red[peak]*(heneset/(hene[1]-hene[0])));    //Scale red positions by HeNe peak separation relative to nominal separation
    
    //Only update errors and feed back if red peak is within a small window around the setpoint
    if (red[peak] < setpoint[slavenum] + WINDOW && red[peak] > setpoint[slavenum] - WINDOW)
    {
      error = red[peak] - setpoint[slavenum];
          
      integral[slavenum] = integral[slavenum] + (int)(I*error);
    
      if (integral[slavenum] > A) integral[slavenum] = A; //Make sure integral doesn't go beyond half range of DAC 
      else if (integral[slavenum] < -A) integral[slavenum] = -A; //(otherwise can move far beyond DAC rail, and be a nuisance to pull back)
          
      output = A + (int)(error * P) + integral[slavenum];
    
      if (output > 65535) output = 65535; //Make sure output doesnt go beyond range of DAC
      else if (output < 0) output = 0;
    
      if (digitalRead(R2FIRST) == HIGH || digitalRead(R5FIRST) == HIGH) writeVoltage(slavenum, (unsigned int)output);
      else writeVoltage(0, (unsigned int)output); //If only one slave expected, output to both ports as unknown which laser present
        
      if (abs(error) > 500) lockcheck = 0;  //If any error is too high or any peaks are missing, switch off LOCK LED
    }
  }
  
  if (lockcheck==0 || rcount < NSLAVE) digitalWrite(LOCKLED,LOW);    //if error signal is too high or any peaks are missing, turn off 'locked' LED
  else digitalWrite(LOCKLED,HIGH);
  
}


void writeVoltage(int DAC_number, unsigned int voltage)  // DAC0 is connected to ports A and C.  DAC1 to ports K and F
{   
  if (DAC_number == 0)
  {
    PORTC = voltage;
    PORTA = BitReverseTable256[voltage >> 8];
    PORTB &= B11110011;      // LD, WR LOW
    PORTB |= B00001100;      // LD, WR HIGH
  }
  else if (DAC_number == 1)
  {
    PORTF = voltage;
    PORTK = voltage >> 8;
    PORTB &= B11111100;      // LD, WR LOW
    PORTB |= B00000011;      // LD, WR HIGH
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////
// Interupt functions://
////////////////////////

void trigger_up()  // Scan has begun
{ 
  TCNT1 = 0; //reset clock
  hcount = 0;  //reset pulse counters
  rcount = 0;
  for (int j = 0; j < NSLAVE; j++) red[j] = 0; //Clear all peak positions
  readin = 1;
}

void trigger_down()  // Outward scan has begun
{ 
  readin = 0;
  if (digitalRead(LOCKSWITCH)==HIGH && hcount == 2 && rcount <= NSLAVE && lockengage == 1) updateDACs(); // Don't feedback unless approx correct number of peaks found
}

void henepulse()  // HeNe pulse has been detected
{
  if (readin == 1)
  {
    if (hcount <= 1) hene[hcount] = TCNT1;
    hcount++;
  }
}

void redpulse()  // red pulse has been detected
{  
  if (hcount == 1)
  {
    if (rcount < NSLAVE) red[rcount] = TCNT1 - hene[0];
    rcount++;
  }
}

void R2inc()  // Red 2 set point increment
{
  //Use increment direction pins to set incdir array
  if (digitalRead(R2DIR)==HIGH) setpoint[0] += DELTA;
  else setpoint[0] -= DELTA;
}

void R5inc()  // Red 5 set point increment
{
   //Use increment direction pins to set incdir array
  if (digitalRead(R5DIR)==HIGH) setpoint[1] += DELTA;
  else setpoint[1] -= DELTA;
}

/////////////////////////////////////////////////////////////////////////////////////////////





