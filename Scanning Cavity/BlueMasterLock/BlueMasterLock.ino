/*
///////////////////////////////////////
// Writen by Sean D, Dan C and Joe G //
///////////////////////////////////////

This arduino controls two DACs, one which adjusts the DC piezo of the scanning cavity to lock it to the HeNe,
one which adjusts the amplitude of the function generator scan to keep this constant

////////////////////////////////
*/

#define A 32768  // Half the max DAC voltage
#define ELEMENTS 4000  // Number of elements in the lookup table (8000 = 1khz scan, 40001/16e6 = 2khz scan, 3200 = 2.5khz scan)
#define TOGGLE 7
#define LOCKLED 4
int lookup[ELEMENTS] = {0};      

volatile long hene_pulse1;
volatile long hene_pulse2;
volatile int hene_pulse_number;

int LOCK = LOW;

int setpoint = A;
int error = 0;
float integral = 0;
const float P = 0;          //Cavity bias piezo feedback variables
const float I = -0.02;      // IF P IS MADE SO BIG THAT THE OUTPUT GOES PAST 4294967295 (max number for UNSIGNED LONG). Then things will go wrong. Probably just won't load.
float output = A;

int fsr = 0;   //Variable to measure free spectral range
int fsrset = 0; //FSR setpoint
float ampintegral = 0; //Scan amplitude feedback integral
const float ampP = 0;       //Scan amplitude modulation feedback variables
const float ampI = 0.005;
float ampoutput = A;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////


// PORT A : Most significant data BITS
// PORT B : WR and LD DAC triggers
// PORT C : Least significant data BITS

void output_A_C(unsigned int voltage)
{    // Send most significant to port A and least significant to port C
 PORTC = voltage;
 PORTA = voltage >> 8;
 
 PORTB = B00000000;      // LD, WR LOW
 PORTB = B11111111;      // LD, WR HIGH
}

void output_K_F(unsigned int voltage)
{  //Send most significant to port K and least significant to port F
    PORTF = voltage;
    PORTK = voltage >> 8;
    PORTB &= B11111100;      // LD, WR LOW
    PORTB |= B00000011;      // LD, WR HIGH
}

void setup() 
{  
  TCCR1A = 0x00;    // CONTROL REGISTER 1A
  TCCR1B = 0x01;    // CONTROL REGISTER 1B
                    // Clock pre-multipliers: 0x01 = clock/1  . 0.0625 micro increments. //3.28ms overflow. (overflow values wrong)
                    //                        0x02 = clock/8  . 0.5 micro increments. //26.2ms overflow.
                    //                        0x03 = clock/64 . 4.0 micro increments. //209.7ms overflow.
  
  TIMSK1 = 0x00;    // INTERRUPT MASK REGISTER
  
  TIMSK0 = 0;       // Timer/Counter Interrupt Mask Register (Turns of timer interrupts...which seem to be turned on by the Arduino)
  
  attachInterrupt(2, trigger_up, RISING);       // TRIGGER PULSE INTERRUPTS (PINS 21, 20)
  
  attachInterrupt(1, trigger_down, FALLING);      //TRIGGER PULSE FALL INTERRUPT (PINS 21, 20)
  
  attachInterrupt(4, hene, RISING);            // HENE PULSE INTERRUPTS (PINS 19)
 
  DDRA = 255;          // Set ports A and C as output pins
  DDRC = 255;
 
  DDRB = 255;    // Set port B as outputs
  
  DDRK = 255;          // Set ports K and F as output pins
  DDRF = 255;
 
  pinMode(TOGGLE, INPUT);    // Set switch pin as input
  pinMode(LOCKLED,OUTPUT);  //Set 'locked' led pin as output
 
  output_A_C(A);    //initialise DACs at half-max (zero-error point, 5V out)
  output_K_F(A);
  
 /* // Produce lookup table
  for(int i = 0; i < ELEMENTS; i++)
  {
    lookup[i]=A * (1 - cos(3.14159 * i / ELEMENTS));
  }*/  
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() 
{ 
  LOCK = digitalRead(TOGGLE);  // Check if LOCK switch is on

  if (LOCK == LOW)
  {
    if(hene_pulse_number == 2)  //If exactly two pulses have been detected, update 'setpoint' state to current
    {
      setpoint = hene_pulse2;
      integral = 0;
      error = 0;
      fsrset = fsr;
      ampintegral = 0;
    }
  } 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Interupt functions:
/////////////////////

void trigger_up()
{
  TCNT1 = 0;                   // reset clock
  hene_pulse_number = 0;
}

void trigger_down()
{
  
  LOCK = digitalRead(TOGGLE); //check lock switch state
  
  if (LOCK == HIGH)
  {
    if (hene_pulse_number == 2 && abs(fsrset-fsr) < 3000)  // If 2 HeNe pulses have been read and are a sensible distance apart
    {
      error =  setpoint - (hene_pulse2); //calculate error, possibly scaled by realityChecker
      integral += error;
      output = A + ((P*error) + (I*integral));
      output_A_C(output); //update DAC
      
      ampintegral += fsrset-fsr;
      ampoutput = A + (ampI*ampintegral);
      output_K_F(ampoutput);
      
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
    hene_pulse1 = TCNT1;//lookup[TCNT1];
  }
  else if (hene_pulse_number == 1)
  {
    hene_pulse2 = TCNT1;//lookup[TCNT1];
    fsr = hene_pulse2-hene_pulse1; //Measures free spectral range
                                   //to avoid feeding back on occasions when only two pulses are found, but one is a fake
                                   //and to feed back to amplitude modulation of function generator
  }  
  hene_pulse_number++; // Make sure this comes at the end of the function
}

/////////////////////////////////

