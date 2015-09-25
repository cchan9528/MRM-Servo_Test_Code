//stalling is because of the rounding convversion angleToPos() for sphere sweeps
//punch adds to oitput torque

#include <ax12.h>                                   //Servos    
#include <Wire.h>                                   //I2C
#include "display.h"
#include "input.h"
#include "modes.h"
#include "mrm.h"

void setup()
{
  Serial.begin(9600);
  Wire.begin();
  
  ax12SetRegister(AZ_SERVO, AX_CW_COMPLIANCE_MARGIN, 0);
  ax12SetRegister(AZ_SERVO, AX_CCW_COMPLIANCE_MARGIN, 0);
  
    ax12SetRegister(EL_SERVO, AX_CW_COMPLIANCE_MARGIN, 0);
  ax12SetRegister(EL_SERVO, AX_CCW_COMPLIANCE_MARGIN, 0);

      ax12SetRegister(AZ_SERVO, AX_CW_COMPLIANCE_SLOPE, 128); // see if 6 reduces the errorl theory: won't be so much that jumps very quickly to output torque to immediately shift it the other direction but won't be so little so that we don't "drift" into the goal positon; however the higher steps also mean that you slow down/decrease output torque GOING INTO the goal position, which also helps.
  ax12SetRegister(AZ_SERVO, AX_CCW_COMPLIANCE_SLOPE, 128);
  
      ax12SetRegister(EL_SERVO, AX_CW_COMPLIANCE_SLOPE, 128);
  ax12SetRegister(EL_SERVO, AX_CCW_COMPLIANCE_SLOPE, 128);
}

void loop()
{
  int mode = -2; 
  int *modePointer = &mode;
  boolean inputReceived = false;
  boolean continueLoop  = false;
  
  displayMenuOptions();

  /* Get desired mode */
  continueLoop = requestFromUser(MODE, modePointer);
  if(!continueLoop)
    return;

  /* Execute desired mode */
  else if(mode == AZIMUTHSWEEP || mode == ELEVATIONSWEEP || mode == SPHERE)           //implement 
  {
    /* For Input */
    int inputAngle, numIterations, hotServoAngleInterval, coldServoAngleInterval, delayTime;
    int *inputAnglePointer             = &inputAngle;
    int *numIterationsPointer          = &numIterations;
    int *hotServoAngleIntervalPointer  = &hotServoAngleInterval;
    int *coldServoAngleIntervalPointer = &coldServoAngleInterval;
    int *delayTimePointer              = &delayTime;

    if(mode == SPHERE)
    {
      Serial.println("\nYou chose 1) Spherical Sweep.\n");
    }
    else 
    {
      if(mode == AZIMUTHSWEEP)
      {
        Serial.println("\nYou chose 2) Azimuth Sweep.\n");
        Serial.println("Enter the desired elevation angle x in degrees with form <x>.\nValid Range: x E [-150, 150].");
      }
      else 
      {
        Serial.println("\nYou chose 3) Elevation Sweep.\n");
        Serial.println("Enter the desired azimuth x in degrees with form <x>.\nValid Range: x E [-150, 150].");
      }
      continueLoop = requestFromUser(ANGLEDEGREES, inputAnglePointer);
      
      /*Check if Should Return to Menu*/
      if(!continueLoop)
        return;
      
      /*Report Input*/
      if(mode == AZIMUTHSWEEP)
        Serial.print("\nDesired Elevation Angle: "); 
      else
        Serial.print("\nDesired Azimuth Angle: "); 
      Serial.println(inputAngle);
      
    }

    /* Wait for desired angle interval(s) */
    if(mode == AZIMUTHSWEEP || mode == ELEVATIONSWEEP)
    {
    Serial.println("\nEnter the angle interval size y in degrees with form <y>. \nValid Range: y E [1, 300].");
    continueLoop = requestFromUser(HOTANGLEINTERVAL, hotServoAngleIntervalPointer);
    if(!continueLoop)
      return;
    Serial.print("\nDesired sweep angle interval: ");
    Serial.println(hotServoAngleInterval);
    }

    if(mode == SPHERE)
    {
      Serial.println("\nEnter the azimuth angle interval size a in degrees with form <a>. \nValid Range: a E [1,300].");
      continueLoop = requestFromUser(HOTANGLEINTERVAL, hotServoAngleIntervalPointer);
      if(!continueLoop)
        return;
      Serial.print("\nDesired azimuth angle interval: ");
      Serial.println(hotServoAngleInterval);
      
      Serial.print("\nEnter the elevation angle interval size e in degrees with form <e>. \nValid Range: a E [1,300].");
      continueLoop = requestFromUser(COLDANGLEINTERVAL, coldServoAngleIntervalPointer);
      if(!continueLoop)
        return;
      Serial.print("\nDesired elevation angle interval: ");
      Serial.println(coldServoAngleInterval);
    }

    /* Request Desired Delay Time */
    Serial.println("\nEnter the time delay t in seconds with form <t>. Valid Range: t E [0, 32767].");
    continueLoop = requestFromUser(DELAYTIME, delayTimePointer);
    if(!continueLoop)
      return;
    Serial.print("\nDesired delay time: ");
    Serial.println(delayTime);
    Serial.print("\n");

    /* Wait for desired number of iterations */
    if(mode == AZIMUTHSWEEP || mode == ELEVATIONSWEEP)
    {
      Serial.println("\nPlease enter the number of sweep iterations y with form <y>.\nOne iteration is both forth and back.\nValid Range: y E [0,65535].");
      continueLoop = requestFromUser(ITERATIONS, numIterationsPointer);
      if(!continueLoop)
        return;
      Serial.print("\nDesired Number of Iterations: "); 
      Serial.println(numIterations);
      Serial.println("\n");
    }

    /* Execute sweeps */
    configureMRM();
    modeStartTime = millis(); 
    int counter = 1;
    if(mode == SPHERE)
    {
      counter       = -150;
      numIterations =  150;
      displayTableHeaders();
    }
    while( counter <= numIterations && continueLoop == true)
    {
      if(mode == SPHERE)
      {
        continueLoop = sweeps(SPHERE, hotServoAngleInterval, delayTime, counter);
        counter += coldServoAngleInterval;
      }
      else if(mode==AZIMUTHSWEEP)
      {
        continueLoop = sweeps(AZIMUTHSWEEP, hotServoAngleInterval, delayTime, inputAngle);
        counter++;
      }
      else
      {
        continueLoop = sweeps(ELEVATIONSWEEP, hotServoAngleInterval, delayTime, inputAngle);
        counter++;
      }
    }
  }
  else if(mode == POSITIONTICKS || mode == POSITIONDEGREES)          
  {
    /* Prompt User for Valid Azimuth Input */
    if(mode == POSITIONTICKS)
    {
      Serial.println("\nYou chose 4) Move to specific position. (in ticks)\n");
      Serial.println("Enter desired azimuth angle in x in ticks with form <x>. \nValid Range: x E [0,1023].\n");
    }
    else
    {
      Serial.println("\nYou chose 5) Move to specific position (in degrees).\n");
      Serial.println("Enter desired azimuth angle in x in degrees with form <x>. \nValid Range: x E [-150,150].\n");
    }
    /* For Input */
    int     azimuthAngle, elevationAngle;
    int     *azimuthAnglePointer   = &azimuthAngle;
    int     *elevationAnglePointer = &elevationAngle;

    /* Wait for Azimuth Input */
    continueLoop = requestFromUser(mode, azimuthAnglePointer);
    if(!continueLoop)
      return;
    if(mode == POSITIONTICKS)
    {
      Serial.print("Desired Azimuth Angle in Ticks: "); 
      Serial.println(azimuthAngle);
      Serial.println("\nEnter desired elevation angle y in ticks with form <y>.\nValid Range: y E [0,1023].");
    }
    else
    {
      Serial.print("Desired Azimuth Angle in Degrees: "); 
      Serial.println(azimuthAngle);
      Serial.println("\nEnter desired elevation angle y in degrees with form <y>.\nValid Range: y E [-150,150].");
    }

    /* Request Elevation Input */
    continueLoop = requestFromUser(mode, elevationAnglePointer);
    if(!continueLoop)
      return;

    if(mode == POSITIONTICKS)
    {
      Serial.print("Desired Elevation Angle in Ticks: "); 
      Serial.println(elevationAngle);
      /* Execute Positioning */
      positionServos(true, azimuthAngle, elevationAngle);
    }
    else
    {
      Serial.print("Desired Elevation Angle in Degrees: "); 
      Serial.println(elevationAngle);
      /* Execute Positioning */
      positionServos(false, azimuthAngle, elevationAngle);
    }
  }

  else if(mode == READMRMONLY)
  {
    Serial.println("\nYou chose 6) Read MRM Only.\n");
    readMRMOnly();
  }
  else if(mode == SERVOINFO)
  {
    Serial.print("Azimuth position [ticks]: ");
    Serial.println(ax12GetRegister(AZ_SERVO,AX_PRESENT_POSITION_L, 2));
    Serial.println("");
    Serial.print("Elevation position [ticks]: ");
    Serial.println(ax12GetRegister(EL_SERVO,AX_PRESENT_POSITION_L, 2));
    Serial.println("");
    Serial.print("AZ CW Compliance Margin [ticks]: ");
    Serial.println(ax12GetRegister(AZ_SERVO,AX_CW_COMPLIANCE_MARGIN, 1));
        Serial.println("");
    Serial.print("AZ CCW Compliance Margin [ticks]: ");
    Serial.println(ax12GetRegister(AZ_SERVO,AX_CCW_COMPLIANCE_MARGIN, 1));
            Serial.println("");
    Serial.print("AZ_CW_COMPLIANCE_SLOPE[ticks]: ");
    Serial.println(ax12GetRegister(AZ_SERVO,AX_CW_COMPLIANCE_SLOPE, 1));
                Serial.println("");
    Serial.print("EL_CCW_COMPLIANCE_SLOPE [ticks]: ");
    Serial.println(ax12GetRegister(EL_SERVO,AX_CCW_COMPLIANCE_SLOPE, 1));
  }
  else if(mode == CONFIGUREMRM)
  {
    
  }

}
