#include <ax12.h>
#include "display.h"
#include "mrm.h"
#include "Arduino.h"

unsigned long modeStartTime = 0;        

void displayMenuOptions()
{
  Serial.print("\n");
  Serial.println("Enter an option with form <#> where # is a number between 1-8, inclusive, or <?> for menu.");
  Serial.print("\n");
  Serial.println("###########################");
  Serial.println("1) Spherical Sweep.");                                 
  Serial.println("2) Azimuth Sweep.");                               
  Serial.println("3) Elevation Sweep.");         
  Serial.println("4) Move to specific position (in ticks). No MRM Collection Done.");      
  Serial.println("5) Move to specific position (in degrees). No MRM Collection Done.");                               
  Serial.println("6) Read MRM Values.");
  Serial.println("7) Display Servo Information");
  Serial.println("8) Configure MRM");
  Serial.println("###########################");
}


void displayTableHeaders()
{
  Serial.println("1. Azimuth in Ticks\n2. Elevation in Ticks\n3. Azimuth in Degrees [0,300] (corresponds to [150,150])\n4. Elevation in Degrees [0,300] (corresponds to [-150,150])\n5. X-Axis\n6. Y-Axis\n7. Z-Axis\n8. Time in Milliseconds");
  Serial.println("1 \t2 \t3 \t4 \t5 \t6 \t7 \t8");
  Serial.println("_ \t_ \t_ \t_ \t_ \t_ \t_ \t_");
}

void displayData()
{ 
  double azServoTicks = ax12GetRegister(AZ_SERVO,AX_PRESENT_POSITION_L,2);
  double elServoTicks = ax12GetRegister(EL_SERVO,AX_PRESENT_POSITION_L,2);
  Serial.print(azServoTicks);
  Serial.print("\t");
  Serial.print(elServoTicks);
  Serial.print("\t");
  Serial.print(azServoTicks*.293);
  Serial.print("\t");
  Serial.print(elServoTicks*.293);
  Serial.print("\t");
  Serial.print(getMRMRegister(X));
  Serial.print("\t");
  Serial.print(getMRMRegister(Y));
  Serial.print("\t");
  Serial.print(getMRMRegister(Z));
  Serial.print("\t");
  Serial.println(millis()-modeStartTime);
}
