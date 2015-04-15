/*
  ////////////////////////////////////
 // Written by Joe G & Dan C//
 ////////////////////////////////////
 
 This arduino controls two DACs, which feed into the PZT controllers for the 397 nm lasers' PDH lock cavities
 
 ////////////////////////////////////////
 // Modifications by Juvid Aryaman and Sarah Woodrow
 // Last Modified: 11/12/12
 ////////////////////////////////////////
 
 Modified to communicate data from this Arduino, using serial communication on TX0 and RX0, to another Arduino. 
 A comment beginnig '// AW:' is a comment by Sarah and Juvid.
 
 ///////////////////////////////
 */

#define A 32768         // Half of the full output of the DAC
#define DELTA 22  //Define step to increment set points by (10 units ~ 1 MHz for normal blue settings)

/////////////////////////////////////////////////////////////////////////////
// AW: Definitions for use in communication

#define dataOut_Length 11  // Set length of data array for serial communication
// This array contains bytes, so the length is the total number of bytes that are to be sent.

// We will send a single byte containing on/off type data.
// The following variables indicate the position of each bit inside the byte. 
#define lockswitch_bit 0   
#define lockcheck_bit 1
#define swapOutputs_bit 2
#define bluePeaks_bit0 3
#define bluePeaks_bit1 4
#define bluePeaks_bit2 5
// number of blue peaks can take values up to 4 - needs 3 bits to store
// bits 6 & 7 are used for error code
// they will only ever be 1 when something has gone wrong
#define longError_bit 6  // For problems converting blue[] from long -> int
#define boolError_bit 7  // For problems converting chars into bit values
/////////////////////////////////////////////////////////////////////////////



//Define table to reverse bits in PORTA byte
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

int NSLAVE = 2;
int BPMAX = 0;

// When a peak is detected, the microprocessor timer value is stored in here
volatile long myTimer = 0;


// Define timing counters
// These don't need to be volatile anymore because they are no longer changed within an interrupt
long hene[2];
long blue[3];
long preliminaryBlue[3];

unsigned int hcount = 0;
unsigned int bcount = 0;

char readin = 0; //Read/write cycle indicator

char swapOutputs = 0;
char lockcheck = 0; //Checks whether to light 'locked' LED

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AW: Variables for Juvid and Sarah's serial communication

char lockswitch = 0; // AW: Monitors whether lock switch is on/off
unsigned char bluePeaks = 0;  // To temporarily store the number of blue peaks

unsigned  int blueInt[3] = {0}; // Array to store blue[2] cast as integers
byte dataOut[dataOut_Length] = {0};         // Declare the array which will contain the data to be sent

byte dataOut_Boolean = 0;   // This will contain bits for this misc. data

/////////////////////////////////////////////////////////////////////////////////////////////////////////////


boolean trancheck[2] = {LOW}; //PDH cavity transmission check (if this drops, stop feeding back and hold integrator)

unsigned int setpoint[2] = {0}; //Set points for each laser
int error = 0;
//float P[2] = {0}; //Proportional gains for each laser
float I[2] = {0}; //Integral gains for each laser
long integral[2] = {0}; //Integrator for each laser
long output = 0; //DAC output for each laser


//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() 
{  
  TCCR1A = 0x00;    // CONTROL REGISTER 1A  
  TCCR1B = 0x01;    // CONTROL REGISTER 1B
  // Clock pre-multipliers: 0x01 = clock/1  . 0.0625 micro increments. 3.28ms? overflow.
  //                        0x02 = clock/8  . 0.5 micro increments. 26.2ms? overflow.
  //                        0x03 = clock/64 . 4 micro increments. 209.7ms? overflow.  
  TIMSK1 = 0x00;    // INTERRUPT MASK REGISTER  
  TIMSK0 = 0;       // Timer/Counter Interrupt Mask Register (Turns of timer interrupts...which seem to be turned on by the Arduino)
  attachInterrupt(3, trigger_up, FALLING);          // TRIGGER PULSE INTERRUPTS (PIN 20)
  attachInterrupt(4, trigger_down, RISING);         // TRIGGER PULSE INTERRUPTS (PIN 19)
  attachInterrupt(5, henepulse, RISING);            // HENE PULSE INTERRUPTS (PIN 18)
  attachInterrupt(2, bluepulse, RISING);            // BLUE PULSE INTERRUPT (PIN 21)

  attachInterrupt(0, B1inc, RISING);            // BLUE 1 SET POINT INCREMENT INTERRUPT (PIN 2)
  attachInterrupt(1, B2inc, RISING);            // BLUE 2 SET POINT INCREMENT INTERRUPT (PIN 3)

  DDRA = 255;           // Set ports A and C (DAC data 1) as output pins
  DDRC = 255; 

  DDRK = 255;           // Set ports K and F (DAC data 2) as output pins
  DDRF = 255;

  DDRB = 255;           // Set port B (Both DACs write) as outputs

  DDRE &= (1<<PE3);     // Set PE3 (B1FIRST) as input

  DDRJ &= B11111100;    // Set PJ0,1 (B2DIR, B1DIR) as inputs

  DDRH &= B11100100;    // Set port PH0,1,3,4 (B2TRAN, B1TRAN, B2FIRST, Lock switch) as inputs
  PORTH |= B00000011;   // Switch on pull-up resistors on PH0,1
  DDRG |= (1<<PG5);     // Set port G5 (LockedLED) as output pin

  Serial.begin(115200);  // AW: Initialise serial communication on TX1, RX1

  //Set number of slave lasers, if B1/B2 ordering switch is in center ('neither first'), expect one slave only
  if ((PINE & (1<<PE3)) || (PINH & (1<<PH3))) NSLAVE = 2;
  else NSLAVE = 1;
  BPMAX = 2 * NSLAVE;

  //set gains for each laser
  I[0] = 1;
  I[1] = 1;
}


void loop()
{
  //Swap outputs if switch set to "B2 first"
  if ((PINH & (1<<PH3))) {
    swapOutputs = 1;
  }
  else {
    swapOutputs = 0;
  }

  if ((PINH & (1<<PH4)) == 0)         //When lock switch is LOW, update setpoints with current positions
  {
    for (int slavenum = 0; slavenum < NSLAVE; slavenum++)
    {
      setpoint[slavenum] = blue[slavenum];
      error = 0;    //reset error counters, set DACs to midpoint 5V
      integral[slavenum] = 0;
      writeVoltage(slavenum, A);
    }
    PORTG &= B11011111; //Switch off LockLED
    lockswitch = 0;		// AW: Indicate that lock switch is LOW
    lockcheck = 0;              // AW: Not currently locking
  }
  else lockswitch = 1;	// AW: Indicate that lock switch is HIGH

  trancheck[0] = (PINH & (1<<PH1));    //Read transmission monitor pins into trancheck array
  trancheck[1] = (PINH & (1<<PH0));

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void updateDACs()
{

  lockcheck = 1;

  for (int slavenum = 0; slavenum < NSLAVE; slavenum++)
  {
    error = blue[slavenum] - setpoint[slavenum];

    if (abs(error) > 1000 || trancheck[slavenum] == LOW)
    {
      lockcheck = 0;  //If any error is too high or PDH transmission blocked, switch off LOCK LED and don't feed back to cavity
    }
    else
    {
      if (!swapOutputs) integral[slavenum] = integral[slavenum] + (int)(I[slavenum]*error);
      else integral[slavenum] = integral[slavenum] + (int)(I[NSLAVE - 1 - slavenum]*error);

      if (integral[slavenum] > A) integral[slavenum] = A; //Make sure integral doesn't go beyond half range of DAC 
      else if (integral[slavenum] < -A) integral[slavenum] = -A; //(otherwise can move far beyond DAC rail, and be a nuisance to pull back)

      output = A + integral[slavenum];// + (int)(error * P);

      if (output > 65535) output = 65535; //Make sure output doesnt go beyond range of DAC
      else if (output < 0) output = 0;

      if (!swapOutputs) writeVoltage(slavenum, (unsigned int)output);
      else writeVoltage(NSLAVE - 1 - slavenum, (unsigned int)output);         
    }
  }

  if (lockcheck==1) PORTG |= B00100000; //If error signal is low enough, switch on LockLED
  else PORTG &= B11011111; //Otherwise, switch off LockLED;

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


// To minimise the amount of time spent in interrupts, use the "interruptCode" variable
// All of the non-time-critical processing is then done in the main loop


void trigger_up()  // Inward scan has begun
{ 
  TCNT1 = 0; //reset clock
  myTimer = 0;  // clock is now fully reset
  hcount = 0;  //reset pulse counters
  bcount = 0;
  readin = 1;
}

void henepulse()  // HeNe pulse has been detected
{
  myTimer = TCNT1;
  if (readin == 1)
  {
    if (hcount == 0) {
      if (myTimer < 10000) {
        hene[0] = myTimer;
        hcount++;
      }
    }
    else if (hcount == 1) {
      hene[1] = myTimer;
      hcount++;  
      // Now, if we have detected exactly 3 blue peaks at this point, the scan was good.  
      if (bcount == 3) {
        for (int i = 0; i <= 2; i++) {
          blue[i] = preliminaryBlue[i];
        }
      }
    } 
  }
}

void bluepulse()  // Blue pulse has been detected
{  
  myTimer = TCNT1;
  if (hcount == 1)
  {
    if (bcount <= 2) {
      preliminaryBlue[bcount] = myTimer - hene[0];
    }
    bcount++;
  }
}

void trigger_down()  // Outward scan has begun
{ 
  readin = 0;  // Back scan.  This is where the DAC update happens.   
  
  if ((PINH & (1<<PH4)) && hcount >= 2) updateDACs(); // Don't feedback unless approx correct number of peaks found and lock is ready to engage
  
  //communicate();    //Serial communication with other Arduino
}

void B1inc()  // Blue 1 set point increment
{
  if (!swapOutputs) {
    if ((PINJ & (1<<PJ0))) {
      setpoint[0] += DELTA;
    }
    else {
      setpoint[0] -= DELTA;
    }
  }
  else {
    if ((PINJ & (1<<PJ0))) {
      setpoint[1] += DELTA;
    }
    else {
      setpoint[1] -= DELTA;
    }    
  }
}

void B2inc()  // Blue 2 set point increment
{
  if (!swapOutputs) {
    if ((PINJ & (1<<PJ1))) {
      setpoint[1] += DELTA;
    }
    else {
      setpoint[1] -= DELTA;
    }
  }
  else {
    if ((PINJ & (1<<PJ1))) {
      setpoint[0] += DELTA;
    }
    else {
      setpoint[0] -= DELTA;
    }    
  }
}


////////////////////////////////////////////////////////////////////////////////////////
//AW: Code to send communication via serial 1

void communicate(){
  // Only send data if "ReceiveReadyPin" is set HIGH by listening Arduino
  if ((PINH & (1<<PH1))){

    // Deal with the boolean data first, then we can use bits 6&7 of dataOut_Boolean for error codes
    
    dataOut_Boolean = 0; // Ensure all bits are set to 0 - this is necessary for the logic to work!

    // Set boolean logic bits in dataOut_Boolean using function
    setBit(lockswitch, lockswitch_bit);
    setBit(lockcheck, lockcheck_bit);
    setBit(swapOutputs, swapOutputs_bit);

    // To set bluePeaks bits in dataOut_Boolean
    bluePeaks = bcount & B00000111;      // Take lowest 3 bits only - required to avoid messing with bits 6&7 in dataOut_Boolean
    bluePeaks = bluePeaks << bluePeaks_bit0;         // Bitshift left until original 0th bit gets to bluePeaks_bit0
    dataOut_Boolean |= bluePeaks;        // Set appropriate bits according to the value of bluePeaks
    
    // Cast blue[] as unsigned integers safely
    if(blue[0] < 65536){
      blueInt[0] = (unsigned int) blue[0];  // Only if value is less than max value for unsigned int
      bitClear(dataOut_Boolean, longError_bit); 	// Clear error flag
    }
    else bitSet(dataOut_Boolean, longError_bit);              // If it is greater, do nothing and flag an error

    if(blue[1] < 65536){
      blueInt[1] = (unsigned int) blue[1];  // Same as above
      bitClear(dataOut_Boolean, longError_bit); 	// Clear error flag
    }
    else bitSet(dataOut_Boolean, longError_bit); 
    
    if(blue[2] < 65536){
      blueInt[2] = (unsigned int) blue[2];  // Same as above
      bitClear(dataOut_Boolean, longError_bit); 	// Clear error flag
    }
    else bitSet(dataOut_Boolean, longError_bit);


    dataOut[10] = dataOut_Boolean;       // Finally, put into array ready to send

    // Decompose integers into high, low bytes, case as bytes and store in DataOut[]. Note low byte first!
    // No error checking here - couldn't think of anything obvious that would go wrong

    // Blue 0 current
    dataOut[0] = (byte) lowByte(blueInt[0]);
    dataOut[1] = (byte) highByte(blueInt[0]);
    // Blue 1 current
    dataOut[2] = (byte) lowByte(blueInt[1]);
    dataOut[3] = (byte) highByte(blueInt[1]);
    // Blue 2 current
    dataOut[4] = (byte) lowByte(blueInt[2]);
    dataOut[5] = (byte) highByte(blueInt[2]);    

    // Setpoint 0
    dataOut[6] = (byte) lowByte(setpoint[0]);
    dataOut[7] = (byte) highByte(setpoint[0]);
    // Setpoint 1
    dataOut[8] = (byte) lowByte(setpoint[1]);
    dataOut[9] = (byte) highByte(setpoint[1]);

/*
    // Testing
    // Output lockswitch, lockcheck, swapOutputs, bluePeaks
    dataOut[6] = (byte) lockswitch;
    dataOut[7] = (byte) lockcheck;
    dataOut[8] = (byte) swapOutputs;
    dataOut[9] = (byte) bluePeaks;
*/


    Serial.write(dataOut, dataOut_Length);   // Write data array to the serial port  
  }
}


// Function sets a specific bit in DataOut_Boolean depending on the data value given
int setBit(char data, int bit_number){

  // If data = 1, set corresponding bit to 1
  if(data == 1) bitSet(dataOut_Boolean, bit_number);
  // Or if data = 0, set corresponding bit to 0
  else if(data == 0) bitClear(dataOut_Boolean, bit_number);
  // If data is anything else, do nothing but send an error code back to say so
  else{
    bitSet(dataOut_Boolean, boolError_bit);    // Flag an error in the byte to be sent
  }

}

/////////////////////////////////////////////////////////////////////////////////////////////

