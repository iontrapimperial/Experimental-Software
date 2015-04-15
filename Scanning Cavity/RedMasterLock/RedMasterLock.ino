/*
///////////////////////////////////////
// Writen by Sean D, Dan C and Joe G //
///////////////////////////////////////

This arduino controls one DAC, which adjusts the DC piezo of the scanning cavity to lock it to the HeNe

////////////////////////////////
*/

#define A 32768  // Half the max DAC voltage
#define TOGGLE 7    //Set lock-switch pin number
#define LOCKLED 4  //Set 'locked' LED pin number

volatile long hene_pulse1;
volatile long hene_pulse2;
volatile int hene_pulse_number;

int LOCK = LOW;

int setpoint = A;
int error = 0;
float integral = 0;
const float P = 0;//-0.015;          // IF P IS MADE SO BIG THAT THE OUTPUT GOES PAST 4294967295 (max number for UNSIGNED LONG). Then things will go wrong. Probably just won't load.
const float I = -0.02;
float output = A;

int realityChecker = 0;   //Variable to stop feeback being applied if one pulse is missed and a fake found instead
int diffint = 0;

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


// PORT A : Most significant data BITS
// PORT B : WR and LD DAC triggers
// PORT C : Least significant data BITS

void output_A_C(unsigned int x)
{    // Send most significant to port A and least significant to port C
 PORTC = x;
 PORTA = BitReverseTable256[x >> 8];
 
 PORTB = B00000000;      // LD, WR LOW
 PORTB = B11111111;      // LD, WR HIGH
}

void output_K_F(unsigned int x)
{    // Send most significant to port A and least significant to port C
 PORTF = x;
 PORTK = x >> 8;
 
 PORTB = B00000000;      // LD, WR LOW
 PORTB = B11111111;      // LD, WR HIGH
} 

void setup() 
{  
  TCCR1A = 0x00;    // CONTROL REGISTER 1A
  TCCR1B = 0x01;    // CONTROL REGISTER 1B
  // Clock pre-multipliers: 0x01 = clock/1  . 0.0625 micro increments. 4.1ms overflow.  (16MHz clock) 
  //                        0x02 = clock/8  . 0.5 micro increments. 32.8ms overflow.    (2MHz clock)
  //                        0x03 = clock/64 . 4 micro increments. 262.4ms overflow.     (250kHz clock)
  
  TIMSK1 = 0x00;    // INTERRUPT MASK REGISTER
  
  TIMSK0 = 0;       // Timer/Counter Interrupt Mask Register (Turns of timer interrupts...which seem to be turned on by the Arduino)
  
  attachInterrupt(2, trigger_up, RISING);       // TRIGGER PULSE INTERRUPTS (PINS 21, 20)
  
  attachInterrupt(1, trigger_down, FALLING);      //TRIGGER PULSE FALL INTERRUPT (PINS 21, 20)
  
  attachInterrupt(4, hene, RISING);            // HENE PULSE INTERRUPTS (PINS 19)
 
  DDRA = 255;          // Set ports A and C as output pins
  DDRC = 255;
  
  DDRK = 255;          // Set ports A and C as output pins
  DDRF = 255; 
 
  DDRB = 255;    // Set port B as outputs
 
  pinMode(TOGGLE,INPUT);    // Set switch pin as input
  pinMode(LOCKLED,OUTPUT);   // Set 'locked' led pin as output
  
  output_A_C(A);
  output_K_F(A);    //Initialise DAC at half max voltage (5V), the zero-error position
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() 
{ 
  LOCK = digitalRead(TOGGLE);  // Check if lock switch is on

    if (LOCK == LOW)
    {
      if(hene_pulse_number == 2)  //If exactly two pulses have been detected, update 'setpoint' state to current one
      {
        setpoint = hene_pulse2;///realityChecker;
        integral = 0;
        error = 0;
        diffint = realityChecker;
      }
    } 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Interrupt functions:
/////////////////////

void trigger_up()
{
  TCNT1 = 0;                   // reset clock
  hene_pulse_number = 0;       // reset pulse count
}

void trigger_down()
{
 
  LOCK = digitalRead(TOGGLE); //check lock switch state
  
  if (LOCK == HIGH)
  {
    if (hene_pulse_number == 2 && abs(diffint - realityChecker) < 3000)    //If 2 HeNe pulses have been read and are close to the expected difference apart
    {
      error =  setpoint - (hene_pulse2);///realityChecker); //update error, potentially scaled by realityChecker
      integral = integral + error;
      output = A + ((P*error) + (I*integral));

      output_A_C(output);
      output_K_F(output);  //update DAC
       
      if (abs(error)<500)
      {
        digitalWrite(LOCKLED,HIGH);    //if error signal is low enough, turn on 'locked' LED
      }
      else
      {
        digitalWrite(LOCKLED,LOW);
      }
    }
  }
  else
  {
    output_A_C(A);
    output_K_F(A);
    digitalWrite(LOCKLED,LOW);
  }
}

void hene()
{  
  if (hene_pulse_number == 0)
  {
    hene_pulse1 = TCNT1;
  }
  else if (hene_pulse_number == 1)
  {
    hene_pulse2 = TCNT1;
    realityChecker = hene_pulse2-hene_pulse1; //checks that the two pulses are a reasonable distance apart
                                              //(to avoid feeding back on occasions when only two pulses are found, but one is a fake)
  } 
  hene_pulse_number++; // Make sure this comes at the end of the function
}

/////////////////////////////////

