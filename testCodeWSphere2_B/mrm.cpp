#include "mrm.h"
#include <Wire.h>
#include "Arduino.h"

int  xRegister, zRegister, yRegister, elServoPos, azServoPos;
byte currentMode;
byte statusRegister;

void configureMRM()
{
  /* Select MRM Mode (see top of code for explanation) */
  Wire.beginTransmission(MRMADDRESS);
  Wire.write(MODEREGISTER);
  //Wire.write(0x01);                   //single-shot mode
  Wire.write(CONTINUOUSMODE);                     //continuous-shot mode
  currentMode = CONTINUOUSMODE;
  Wire.endTransmission();

  /* Select MRM Output Data to Registers Rate */
  Wire.beginTransmission(MRMADDRESS);
  Wire.write(0x00);
  Wire.write(0x10);
  Wire.endTransmission();

  /*Resolution Select*/
  Wire.beginTransmission(MRMADDRESS);
  Wire.write(0x01);
  Wire.write(0x40);
  Wire.endTransmission();
}

void mrmCollection()
{

  int xRegister1, xRegister2, zRegister1, zRegister2, yRegister1, yRegister2;

  /* Move register pointer to MRM data registers */
  Wire.beginTransmission(MRMADDRESS);
  Wire.write(FIRSTDATAREGISTER);
  Wire.endTransmission();

  /* Start reading MRM Registers */
  Wire.requestFrom(MRMADDRESS, 6);
  //Serial.print("Wire.available(): "); 
  //Serial.println(Wire.available());
  if(Wire.available()>=6)
    {
      //Serial.println("I'm Inside and Reading");
      xRegister1 = Wire.read();
      xRegister2 = Wire.read();
      zRegister1 = Wire.read();                 // x,z,y is reg order
      zRegister2 = Wire.read();
      yRegister1 = Wire.read();
      yRegister2 = Wire.read();

      xRegister = (xRegister1<<8) | xRegister2;
      zRegister = (zRegister1<<8) | zRegister2;
      yRegister = (yRegister1<<8) | yRegister2;

      delay(250);                                    //for continuous-mode; datasheet says 67 minimum
    }
}

int getMRMRegister(unsigned char letter)
{
  switch(letter)
  {
    case X:
      return xRegister;
    case Y:
      return yRegister;
    case Z:
      return zRegister;
  }
}
