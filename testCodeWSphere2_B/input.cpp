#include "input.h"
#include "modes.h"
#include <hardwareSerial.h>
#include "Arduino.h"

/* Input */
const  char  startByte                  = '<';
const  char  stopByte                   = '>';        // Input 
static byte  bufferIndex                =  0 ;        //byte limited to 0-255
char         buffer[7]                  = "\0";

bool checkForInput()
{
  delay(50);                          //ESSENTIAL DELAY; DON'T REMOVE

  int charsWaiting=Serial.available();

  if(charsWaiting)
    {
      while(charsWaiting)               //if we have to process input
	{
	  char inChar = Serial.read();    //save the character and remove from queue
	  if(inChar == startByte)         // Case: '<'
	    {
	      bufferIndex = 0;              // start overwriting what's in the buffer
	    }
	  else if(inChar == stopByte)     // Case: '>'
	    {
	      buffer[bufferIndex] = '\0';
	      bufferIndex = 0;
	      break;
	    }
	  else                            // Case: else
	    {
	      buffer[bufferIndex]=inChar;
	      bufferIndex++;
	    }
	  charsWaiting--;
	}
      return true;
    }
  return false;
}

bool requestFromUser(int variableCode, int * neededVariable )
{
  boolean inputRecieved     = false;
  boolean invalidInputSent  = false;
  double     valueNeeded;    //this was an int before 
  while(!inputRecieved)
    {
      inputRecieved = checkForInput();
      if(inputRecieved)
	{
	  if(buffer[0] == '?')
	    {
	      return MENUREQUEST;
	    }
	  else
	    {
	      valueNeeded = atoi(buffer);
	      if(variableCode == MODE)
		{
		  if(!(valueNeeded >= 1 && valueNeeded <=7))
		    {
		      Serial.println("\nInvalid option. Please enter another or <?> for menu.");
		      invalidInputSent = true;
		    }
		}
	      else if(variableCode == ANGLEDEGREES)
		{
		  if(!(valueNeeded >= -150 && valueNeeded <= 150))
		    {
		      Serial.println("\nInvalid angle in degrees. Please enter another or <?> for menu.\n");
		      invalidInputSent = true;
		    }
		}
	      else if(variableCode == HOTANGLEINTERVAL || variableCode == COLDANGLEINTERVAL )
		{
		  if(!(valueNeeded >=1 && valueNeeded <=300))
		    {
		      Serial.println("\nInvalid angle interval. Enter another or <?> for menu.\n");
		      invalidInputSent = true;
		    }
		}
	      else if(variableCode == DELAYTIME)
		{
		  if(!(valueNeeded >= 0 && valueNeeded <=99999))
		    {
		      Serial.println("\nInvalid delay time. Enter another or <?> for menu.\n");
		      invalidInputSent = true;
		    }
		}
	      else if(variableCode == ITERATIONS)
		{
		  if(valueNeeded<0)
		    {
		      Serial.println("\nInvalid number of iterations. Please enter another or <?> for menu.\n");
		      invalidInputSent = true;
		    }
		}
	      else if(variableCode == ANGLETICKS)
		{
		  if(!(valueNeeded >= 0 && valueNeeded <= 1023))
		    {
		      Serial.println("\nInvalid angle in ticks. Please enter another or <?> for menu.\n");
		      invalidInputSent = true; 
		    }
		}
	      else if(variableCode == MRMOUTPUTRATE)
		{
		  if(!(valueNeeded == 1 || valueNeeded == 2 || valueNeeded == 4 || valueNeeded == 8))
		    {
		      Serial.println("\nInvalid output rate. Please enter another or <?> for menu.\n");
		      invalidInputSent = true;
		    }
		}
	      else if(variableCode == MRMSHOTMODE)
		{
		  if(!(valueNeeded == 1 || valueNeeded == 2))
		    {
		      Serial.println("\nInvalid shot mode selection. Please enter another or <?> for menu.\n");
		      invalidInputSent = true;
		    }
		}
	      else if(variableCode == MRMRESOLUTION)
		{
		  if(!(valueNeeded == 1370 || valueNeeded == 1090 || valueNeeded == 820 || valueNeeded == 660 || valueNeeded == 440 || valueNeeded == 390 || valueNeeded == 330 || valueNeeded == 230 || valueNeeded == 4.35 || valueNeeded == 3.03 || valueNeeded == 2.56 || valueNeeded == 2.27 || valueNeeded == 1.52 || valueNeeded == 1.22 || valueNeeded == .92 || valueNeeded == .73))
		    {
		      Serial.println("\nInvalid gain/resolution requested. Please enter another or <?> for menu.\n");
		      invalidInputSent = true;
		    }
		}
	      if(invalidInputSent)
		{
		  buffer[0] = '\0';                     //reset buffer just in case
		  inputRecieved = false;                // check for input again
		  invalidInputSent = false;
		}
	    }
	}
    }

  *(neededVariable) = valueNeeded;
  return true;
}
