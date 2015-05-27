/* Code for external control of an AD9910/PCBZ evaluation board in single tone mode. 
   The logic levels of the profile pins are not controlled by the Arduino here.
   This version allows amplitude modulation.
   Written by Vincent Jarlaud
   Imperial College London, March 2015*/

#include <stdio.h>
#include <SPI.h>

const int masterReset = 47;
const int powerDown = 45;
const int IOUpdate = 43;
const int IOReset = 49;
const int slaveSelect = 53;
const int switchState = 7;
const int switchOn = 6;

const byte CFR1[4] = {0b00000000, 0b00000000, 0b00000000, 0b00000000}; // default
const byte CFR2[4] = {0b00000001, 0b01000000, 0b00001000, 0b00100000}; // amplitude scale factor enabled
const byte CFR3[4] = {0b00011111, 0b00111111, 0b11000000, 0b00000000}; // divider bypassed

const byte defProfile[8] = {0b00101110, 0b10101010, 0, 0, 58, 225, 71, 174};

byte STP0[8] = {0b00101110, 0b10101010, 0, 0, 58, 225, 71, 174}; 
byte STP1[8] = {0b00101110, 0b10101010, 0, 0, 58, 225, 71, 174}; 
byte STP2[8] = {0b00101110, 0b10101010, 0, 0, 58, 225, 71, 174};
byte STP3[8] = {0b00101110, 0b10101010, 0, 0, 58, 225, 71, 174}; // initialisation of the profile registers
byte STP4[8] = {0b00101110, 0b10101010, 0, 0, 58, 225, 71, 174}; // 33 dBm
byte STP5[8] = {0b00101110, 0b10101010, 0, 0, 58, 225, 71, 174}; 
byte STP6[8] = {0b00101110, 0b10101010, 0, 0, 58, 225, 71, 174}; 
byte STP7[8] = {0b00101110, 0b10101010, 0, 0, 58, 225, 71, 174};

void setup()
{ 
  pinMode(switchState, INPUT);
  pinMode(switchOn, OUTPUT);
  pinMode(masterReset, OUTPUT);
  pinMode(powerDown, OUTPUT);
  pinMode(IOUpdate, OUTPUT);
  pinMode(IOReset, OUTPUT);
  pinMode(slaveSelect, OUTPUT);
  
  digitalWrite(switchOn, HIGH); // input of the DDS switch, should remain HIGH
  
  if(digitalRead(switchState) == HIGH)
  {
    digitalWrite(powerDown, HIGH); // turns off the DDS
  }
  
  if(digitalRead(switchState) == LOW)
  {
    digitalWrite(powerDown, LOW); // turns on the DDS
  }
  
  Serial.begin(9600); // maximum working Baud rate for some reason
  
  SPI.begin(); // start SPI communication
  SPI.setClockDivider(SPI_CLOCK_DIV2); // maximum rate, frequency of the synchronisation clock (8 MHz)
  SPI.setBitOrder(MSBFIRST); // most sigificant bit first
  SPI.setDataMode(SPI_MODE0); // the logic level has to be on the rising edge of the clock
  
  Reset(); // sets the DDS in a well known initial state
  
  digitalWrite(IOReset, LOW); 
  
  WriteRegister4(0b00000000, CFR1[0], CFR1[1], CFR1[2], CFR1[3]);
  WriteRegister4(0b00000001, CFR2[0], CFR2[1], CFR2[2], CFR2[3]); // write in the control registers
  WriteRegister4(0b00000010, CFR3[0], CFR3[1], CFR3[2], CFR3[3]);
  
  //digitalWrite(IOReset, HIGH);
  //digitalWrite(IOReset, LOW);
  
  WriteRegister8(0b00001110, STP0[0], STP0[1], STP0[2], STP0[3], STP0[4], STP0[5], STP0[6], STP0[7]);
  WriteRegister8(0b00001111, STP1[0], STP1[1], STP1[2], STP1[3], STP1[4], STP1[5], STP1[6], STP1[7]);
  WriteRegister8(0b00010000, STP2[0], STP2[1], STP2[2], STP2[3], STP2[4], STP2[5], STP2[6], STP2[7]);
  WriteRegister8(0b00010001, STP3[0], STP3[1], STP3[2], STP3[3], STP3[4], STP3[5], STP3[6], STP3[7]); // write in the profile registers
  WriteRegister8(0b00010010, STP4[0], STP4[1], STP4[2], STP4[3], STP4[4], STP4[5], STP4[6], STP4[7]);
  WriteRegister8(0b00010011, STP5[0], STP5[1], STP5[2], STP5[3], STP5[4], STP5[5], STP5[6], STP5[7]);
  WriteRegister8(0b00010100, STP6[0], STP6[1], STP6[2], STP6[3], STP6[4], STP6[5], STP6[6], STP6[7]);
  WriteRegister8(0b00010101, STP7[0], STP7[1], STP7[2], STP7[3], STP7[4], STP7[5], STP7[6], STP7[7]);
  
  SendUpdate();
}



void loop()
{ 
  if(digitalRead(switchState) == HIGH)
  {
    digitalWrite(powerDown, HIGH);
  }
  
  if(digitalRead(switchState) == LOW)
  {
    digitalWrite(powerDown, LOW);
  }
  
  int data[64];
  int check = 0;
  int reset = 0;
  
  for(int i = 0; i < 64; i++)
  {
    data[i] = -1;  // initialisation
  }
  
  while (Serial.available()>0)
  {
    for(int i = 0; i < 64; i++)
    {
      data[i] = Serial.parseInt(); 
    }
    
    int i = 0;
    do
    {
      if(data[i] != -1) check = 1;
      else check = 0; 
      i++;
    }
    while(check ==1 && i < 64);
       
    Serial.println(check); // handshake
    
    int reset = 0;
    int j = 0;
    
    do
    {
      if(data[j] == 256) reset = 1;
      else reset = 0;
      j++;
    }
    while(reset == 1 && j < 64);
    
    if (reset == 1)
    {
      digitalWrite(powerDown, HIGH);
      delay(100);
      digitalWrite(powerDown, LOW);
      
      Reset();
      digitalWrite(IOReset, LOW);
  
      WriteRegister4(0b00000000, CFR1[0], CFR1[1], CFR1[2], CFR1[3]);
      WriteRegister4(0b00000001, CFR2[0], CFR2[1], CFR2[2], CFR2[3]);
      WriteRegister4(0b00000010, CFR3[0], CFR3[1], CFR3[2], CFR3[3]);
      
      WriteRegister8(0b00001110, defProfile[0], defProfile[1], defProfile[2], defProfile[3], defProfile[4], defProfile[5], defProfile[6], defProfile[7]);
      WriteRegister8(0b00001111, defProfile[0], defProfile[1], defProfile[2], defProfile[3], defProfile[4], defProfile[5], defProfile[6], defProfile[7]);
      WriteRegister8(0b00010000, defProfile[0], defProfile[1], defProfile[2], defProfile[3], defProfile[4], defProfile[5], defProfile[6], defProfile[7]);
      WriteRegister8(0b00010001, defProfile[0], defProfile[1], defProfile[2], defProfile[3], defProfile[4], defProfile[5], defProfile[6], defProfile[7]);
      WriteRegister8(0b00010010, defProfile[0], defProfile[1], defProfile[2], defProfile[3], defProfile[4], defProfile[5], defProfile[6], defProfile[7]);
      WriteRegister8(0b00010011, defProfile[0], defProfile[1], defProfile[2], defProfile[3], defProfile[4], defProfile[5], defProfile[6], defProfile[7]);
      WriteRegister8(0b00010100, defProfile[0], defProfile[1], defProfile[2], defProfile[3], defProfile[4], defProfile[5], defProfile[6], defProfile[7]);
      WriteRegister8(0b00010101, defProfile[0], defProfile[1], defProfile[2], defProfile[3], defProfile[4], defProfile[5], defProfile[6], defProfile[7]);
      
      SendUpdate();
    }  
    
    if (Serial.read() == '\n' && check == 1 && reset == 0) // Modifications to the registers will be performed only if all the data ints were received properly.
    {
      if(data[4] != 0 || data[5] != 0 || data[6] != 0 || data[7] != 0) // This allows to check if the Profile 0 was selected
      {
        for(int i = 0; i < 8; i++)
        {
          int j = i;
          STP0[j] = byte(data[i]);
        }
      }
      
      if(data[12] != 0 || data[13] != 0 || data[14] != 0 || data[15] != 0) // This allows to check if the Profile 1 was selected
      {
        for(int i = 8; i < 16; i++)
        {
          int j = i - 8;
          STP1[j] = byte(data[i]);
        }
      }
      
      if(data[20] != 0 || data[21] != 0 || data[22] != 0 || data[23] != 0) // This allows to check if the Profile 2 was selected
      {
        for(int i = 16; i < 24; i++)
        {
          int j = i - 16;
          STP2[j] = byte(data[i]);
        }
      }
      
      if(data[28] != 0 || data[29] != 0 || data[30] != 0 || data[31] != 0) // This allows to check if the Profile 3 was selected
      {
        for(int i = 24; i < 32; i++)
        {
          int j = i - 24;
          STP3[j] = byte(data[i]);
        }
      }
      
      if(data[36] != 0 || data[37] != 0 || data[38] != 0 || data[39] != 0) // This allows to check if the Profile 4 was selected
      {
        for(int i = 32; i < 40; i++)
        {
          int j = i - 32;
          STP4[j] = byte(data[i]);
        }
      }
      
      if(data[44] != 0 || data[45] != 0 || data[46] != 0 || data[47] != 0) // This allows to check if the Profile 5 was selected
      {
        for(int i = 40; i < 48; i++)
        {
          int j = i - 40;
          STP5[j] = byte(data[i]);
        }
      }
      
      if(data[52] != 0 || data[53] != 0 || data[54] != 0 || data[55] != 0) // This allows to check if the Profile 6 was selected
      {
        for(int i = 48; i < 56; i++)
        {
          int j = i - 48;
          STP6[j] = byte(data[i]);
        }
      }
      
      if(data[60] != 0 || data[61] != 0 || data[62] != 0 || data[63] != 0) // This allows to check if the Profile 7 was selected
      {
        for(int i = 56; i < 64; i++)
        {
          int j = i - 56;
          STP7[j] = byte(data[i]);
        }
      }
      
      digitalWrite(IOReset, HIGH); // reset the IO line before communication, probably overkill
      digitalWrite(IOReset, LOW);
          
      if(data[4] != 0 || data[5] != 0 || data[6] != 0 || data[7] != 0)
      {
        WriteRegister8(0b00001110, STP0[0], STP0[1], STP0[2], STP0[3], STP0[4], STP0[5], STP0[6], STP0[7]);
      }
      
      if(data[12] != 0 || data[13] != 0 || data[14] != 0 || data[15] != 0)
      {
        WriteRegister8(0b00001111, STP1[0], STP1[1], STP1[2], STP1[3], STP1[4], STP1[5], STP1[6], STP1[7]);
      }
      
      if(data[20] != 0 || data[21] != 0 || data[22] != 0 || data[23] != 0)
      {
        WriteRegister8(0b00010000, STP2[0], STP2[1], STP2[2], STP2[3], STP2[4], STP2[5], STP2[6], STP2[7]);
      }
      
      if(data[28] != 0 || data[29] != 0 || data[30] != 0 || data[31] != 0)
      {
        WriteRegister8(0b00010001, STP3[0], STP3[1], STP3[2], STP3[3], STP3[4], STP3[5], STP3[6], STP3[7]);
      }
      
      if(data[36] != 0 || data[37] != 0 || data[38] != 0 || data[39] != 0)
      {
        WriteRegister8(0b00010010, STP4[0], STP4[1], STP4[2], STP4[3], STP4[4], STP4[5], STP4[6], STP4[7]);
      }
      
      if(data[44] != 0 || data[45] != 0 || data[46] != 0 || data[47] != 0)
      {
        WriteRegister8(0b00010011, STP5[0], STP5[1], STP5[2], STP5[3], STP5[4], STP5[5], STP5[6], STP5[7]);
      }
      
      if(data[52] != 0 || data[53] != 0 || data[54] != 0 || data[55] != 0)
      {
        WriteRegister8(0b00010100, STP6[0], STP6[1], STP6[2], STP6[3], STP6[4], STP6[5], STP6[6], STP6[7]);
      }
      
      if(data[60] != 0 || data[61] != 0 || data[62] != 0 || data[63] != 0)
      {
        WriteRegister8(0b00010101, STP7[0], STP7[1], STP7[2], STP7[3], STP7[4], STP7[5], STP7[6], STP7[7]);
      }
      
      SendUpdate(); 
    }
  }
}

void Reset() // this function resets the DDS to put it in a known state before the serial communication
{
  digitalWrite(masterReset, HIGH);
  digitalWrite(masterReset, LOW);
}

void SendUpdate() // sends the data from the buffers of the DDS to the registers
{
  digitalWrite(IOUpdate, HIGH);
  digitalWrite(IOUpdate, LOW);
}

void WriteRegister4(byte adress, byte value1, byte value2, byte value3, byte value4) // function to write in 4 byte registers
{
  digitalWrite(slaveSelect, LOW); // opens the serial line
  SPI.transfer(adress);
  SPI.transfer(value1);
  SPI.transfer(value2);
  SPI.transfer(value3);
  SPI.transfer(value4);
  digitalWrite(slaveSelect, HIGH); // closes the serial line
}
  
void WriteRegister8(byte adress, byte value1, byte value2, byte value3, byte value4, byte value5, byte value6, byte value7, byte value8) // function to write in 8 byte registers
{
  digitalWrite(slaveSelect, LOW);
  SPI.transfer(adress);
  SPI.transfer(value1);
  SPI.transfer(value2);
  SPI.transfer(value3);
  SPI.transfer(value4);
  SPI.transfer(value5);
  SPI.transfer(value6);
  SPI.transfer(value7);
  SPI.transfer(value8);
  digitalWrite(slaveSelect, HIGH);
}

