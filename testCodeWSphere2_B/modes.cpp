#include "ax12.h"
#include "modes.h"
#include "input.h"
#include "mrm.h"
#include "display.h"
#include "Arduino.h"

int angleToPos(int anglePosition)
{
  int adjustedAngle =anglePosition;
  if(anglePosition>=150)
  {
    adjustedAngle = 150;
  }
  else if(anglePosition <= -150)
  {
    adjustedAngle = -149.5;
  }
  return round((abs(adjustedAngle - 210) -60) /.293);  //+150 ==> 0 degrees ==> 0 ticks, -150 ==> 300 degrees ==> 1023 ticks

}

/* Handles AZIMUTHSWEEP AND ELEVATIONSWEEP */
bool sweeps(int sweepMode, int hotServoAngleInterval, int delayTime, int coldServoPosition )
{
  /* Print Headers */
  if(sweepMode != SPHERE)
    displayTableHeaders();

  /* Define Upper and Lower Angle Limits */
  int positive150Deg = angleToPos(150.0); 
  int negative150Deg = angleToPos(-150.0);  

  /* Move Servos to Start Positions */
  ax12SetRegister2(AZ_SERVO, AX_GOAL_SPEED_L, 500); //format: servo, register, tick value
  ax12SetRegister2(EL_SERVO, AX_GOAL_SPEED_L, 500); //tick value has different meaning depending on register
  if(sweepMode == AZIMUTHSWEEP || sweepMode == SPHERE)
    {
      SetPosition(AZ_SERVO, positive150Deg);
      SetPosition(EL_SERVO, angleToPos(coldServoPosition));
    }
  else if(sweepMode == ELEVATIONSWEEP)
    {
      SetPosition(EL_SERVO, positive150Deg);
      SetPosition(AZ_SERVO, angleToPos(coldServoPosition));
    }
  delay(1000);

  /* Set Servo Speed */
  int rpm            = 7;
  int servoSpeed     = round(rpm/.111);           // tick E [0-1023], gives servo speed; .111 rpm/tick
  ax12SetRegister2(AZ_SERVO, AX_GOAL_SPEED_L, servoSpeed);
  ax12SetRegister2(EL_SERVO, AX_GOAL_SPEED_L, servoSpeed);

  /* Begin Sweep(s) */
  unsigned long startMRMTime;  
  int           convertedInterval = hotServoAngleInterval / .293;
  int           currentPosition   = ax12GetRegister(AZ_SERVO, AX_PRESENT_POSITION_L, 2); 
  int           destination       = currentPosition; //or else it would skip first measuremnt
  boolean       inputReceived     = false;
  boolean       completedSweep    = false;
  boolean       goingForward      = true;

  while( !completedSweep )
    {
      /* Check if you should break */
      inputReceived = checkForInput();
      if(inputReceived && buffer[0] =='!')
	{
	  Serial.println("\nExecution stopped.");
	  return false;
	}

      /* Sweep selected servo */
      if(sweepMode == AZIMUTHSWEEP || sweepMode == SPHERE)
	{
	  SetPosition(AZ_SERVO, destination);
	}
      else if(sweepMode == ELEVATIONSWEEP)
	{
	  SetPosition(EL_SERVO, destination);
	}

      /* Wait for Until Destination Reached */
      //Note: destination != 1023 ticks exactly b/c conversion
      while(!(ax12GetRegister(AZ_SERVO,AX_PRESENT_POSITION_L,2) == destination));
    
      /* Record Time to Match Desired Delay For Measurements */
      startMRMTime = millis();
      while((millis() - startMRMTime) <= (delayTime*1000) )
	{
	  /* MRM Collection */
	  mrmCollection();
	  displayData();

	  /* Check For Break */
	  inputReceived = checkForInput();
	  if(inputReceived && buffer[0] =='!')
	    {
	      Serial.println("\nExecution stopped.");
	      return false;
	    }
	}

      /* Change the Destination of Sweeping Servo */
      if(goingForward)
	{
	  destination += convertedInterval;
	  if(destination >= 1023)
	    {
	      destination  = 1023;
	      goingForward = false;
	    }
	}
      else
	{
	  destination -= convertedInterval;
	  if(destination < 0)
	    {
	      completedSweep = true; 
	    }
	}
    }
  return true;
}

/* Handles POSITIONTICKS and POSITIONDEGREES */
void positionServos(bool sentTicks, int azimuthPos, int elevationPos)
{
  /* Set Servo Speed */
  ax12SetRegister2(AZ_SERVO, AX_GOAL_SPEED_L, 500); //format: servo, register, tick value
  ax12SetRegister2(EL_SERVO, AX_GOAL_SPEED_L, 500); //tick value has different meaning depending on register

  /* Move Servos to Desired Positions */
  if(sentTicks)
    {
      SetPosition(AZ_SERVO, azimuthPos);
      SetPosition(EL_SERVO, elevationPos);
    }
  else
    {
      SetPosition(AZ_SERVO, angleToPos(azimuthPos));
      SetPosition(EL_SERVO, angleToPos(elevationPos));
    }

  delay(1000);
}

void readMRMOnly()
{
  Serial.println("\nUse the command <!> to stop execution and return to menu.\n");
  boolean inputReceived;
  displayTableHeaders();
  while(1)
    {
      mrmCollection();
      displayData();

      /* Check For Break */
      inputReceived = checkForInput();
      if(inputReceived && buffer[0] =='!')
	{
	  Serial.println("\nExecution stopped.\n");
	  return;
	}
    }
}
