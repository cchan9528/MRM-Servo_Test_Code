#include <ax12.h>                                   //Servos
//#include <BioloidController.h>                      
#include <Wire.h>                                   //I2C

/* Servos */
#define AZ_SERVO                         1         
#define EL_SERVO                         2

/* Modes */
#define MENUREQUEST                     -2   
#define AZIMUTHSWEEP                     1
#define ELEVATIONSWEEP                   2
#define POSITIONTICKS                    3
#define POSITIONDEGREES                  4
#define READMRMONLY                      5

/* MRM */
#define MRMADDRESS                       0x1E       //Slave Address
#define CONTINUOUSMODE                   0x00
#define SINGLESHOTMODE                   0x01 
#define MODEREGISTER                     0x02
#define FIRSTDATAREGISTER                0x03 
#define STATUSREGISTER                   0x09
int  xRegister, zRegister, yRegister, elServoPos, azServoPos;
byte currentMode;
byte statusRegister;
unsigned long modeStartTime;

/* Input */
const  char startByte                  = '<';
const  char stopByte                   = '>';        // Input 
const  byte bufferSizeLimit            =  7 ;
static char buffer[bufferSizeLimit];                
static byte bufferIndex                =  0 ;        //byte limited to 0-255

//////////////////////////////
// Display Functions
//////////////////////////////

void displayMenuOptions()
{
  Serial.print("\n");
  Serial.println("Enter an option with form <#> where # is a number between 1-5, inclusive, or <?> for menu.");
  Serial.print("\n");
  Serial.println("###########################");
  Serial.println("1) Azimuth Sweep.");                                 // Con 
  Serial.println("2) Elevation Sweep.");                               // Con
  Serial.println("3) Move to specific position (in ticks). No MRM Collection Done.");         // Con
  Serial.println("4) Move to specific position (in degrees).No MRM Collection Done.");       // Dis
  Serial.println("5) Read MRM Values.");                               // Dis; Just read relative to position it's at now
  Serial.println("###########################");
}

void displayTableHeaders()
{
  Serial.println("1. Elevation in Ticks\n2. Azimuth in Ticks\n3. Elevation in Degrees\n4. Azimuth in Degrees\n5. X-Axis\n6. Y-Axis\n7. Z-Axis\n8. Time in Milliseconds");
  Serial.println("1 \t2 \t3 \t4 \t5 \t6 \t7 \t8");
  Serial.println("_ \t_ \t_ \t_ \t_ \t_ \t_ \t_");
}

void displayMRMRegisters()
{
  Serial.print(ax12GetRegister(AZ_SERVO,AX_PRESENT_POSITION_L,2));
  Serial.print("\t");
  Serial.print(ax12GetRegister(EL_SERVO, AX_PRESENT_POSITION_L,2));
  Serial.print("\t");
  Serial.print(xRegister);
  Serial.print("\t");
  Serial.print(yRegister);
  Serial.print("\t");
  Serial.print(zRegister);
  Serial.print("\t");
  Serial.println(millis()-modeStartTime);
}

///////////////////////////
// Input Functions
///////////////////////////

boolean checkForInput()
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



///////////////////////////////
// Servo Conversion Functions
///////////////////////////////

int angleToPos(double anglePosition)
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
  return round((abs(adjustedAngle - 210) -60) /.293);  //+150 ==> 0 degreees, -150 ==> 300 degrees

}

///////////////////////////////
// MRM Functions
///////////////////////////////
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

  //    Wire.write(FIRSTDATAREGISTER);
  //    Wire.requestFrom(MRMADDRESS,6);
  //    Wire.read();
  //    Wire.read();
  //    Wire.read();
  //    Wire.read();
  //    Wire.read();
  //    Wire.read();
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

///////////////////////////////
// Mode Functions
///////////////////////////////

/* Handles AZIMUTHSWEEP AND ELEVATIONSWEEP */
boolean azelSweep(boolean azSweepSelected, int angleInterval, int delayTime, double nonsweepservoPosition)
{
  /* Print Headers */
  displayTableHeaders();

  /* Configure Servo Settings */
  int rpm            = 5;
  int servoSpeed     = round(rpm/.111);           // tick E [0-1023], gives servo speed; .111 rpm/tick
  int positive150Deg = angleToPos(150.0); 
  int negative150Deg = angleToPos(-150.0);  

  /* Move Servos to Start Positions */
  ax12SetRegister2(AZ_SERVO, AX_GOAL_SPEED_L, 500); //format: servo, register, tick value
  ax12SetRegister2(EL_SERVO, AX_GOAL_SPEED_L, 500); //tick value has different meaning depending on register
  if(azSweepSelected)
  {
    SetPosition(AZ_SERVO, positive150Deg);
    SetPosition(EL_SERVO, angleToPos(nonsweepservoPosition));
  }
  else
  {
    SetPosition(EL_SERVO, positive150Deg);
    SetPosition(AZ_SERVO, angleToPos(nonsweepservoPosition));
  }
  delay(1000);

  /* Set Servo Speed */
  ax12SetRegister2(AZ_SERVO, AX_GOAL_SPEED_L, servoSpeed);
  ax12SetRegister2(EL_SERVO, AX_GOAL_SPEED_L, servoSpeed);

  /* Begin Sweep(s) */
  unsigned long startMRMTime;  
  int           convertedInterval= angleInterval / .293;
  int           currentPosition  = ax12GetRegister(AZ_SERVO, AX_PRESENT_POSITION_L, 2); 
  int           destination      = currentPosition; //or else it would skip first measuremnt
  boolean       inputReceived    = false;
  boolean       completedSweep   = false;
  boolean       goingForward     = true;

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
    if(azSweepSelected)
    {
      SetPosition(AZ_SERVO, destination);
    }
    else
    {
      SetPosition(EL_SERVO, destination);
    }
    delay(200);

    /* Record Time to Match Desired Delay For Measurements */
    startMRMTime = millis();
    while((millis() - startMRMTime) <= (delayTime*1000) )
    {
      /* MRM Collection */
      mrmCollection();
      displayMRMRegisters();

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
        destination = 1023;
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
void positionServos(boolean sentTicks, int azimuthPos, int elevationPos)
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
      displayMRMRegisters();

      /* Check For Break */
      inputReceived = checkForInput();
      if(inputReceived && buffer[0] =='!')
      {
        Serial.println("\nExecution stopped.\n");
        return;
      }
  }
}


/////////////////////////////////
/////////////////////////////////
/////////Setup and Loop//////////
/////////////////////////////////
/////////////////////////////////

void setup()
{
  Serial.begin(9600);
  Wire.begin();

  /*Reset Buffer*/
  buffer[0] = '\0';  
}

void loop()
{
  int switchCase = -1; 
  boolean inputReceived = false;

  displayMenuOptions();

  /* Get desired mode */
  while(!inputReceived)      
  {
    inputReceived = checkForInput();  

    if(inputReceived)        
    {
      if(buffer[0] == '?')                        //input asking for menu
      {
        switchCase = MENUREQUEST;
      }
      else
      {
        switchCase = atoi(buffer);
        if(!(switchCase >= 1 && switchCase <=5))  //if switchCase out of range
        {
          Serial.println("\nInvalid option. Please enter another or <?> for menu.");
          buffer[0] = '\0';                     //reset buffer just in case
          inputReceived = false;                // check for input again
        }
      }
      //else break the input check
      //guaranteed bcause inputReceived is now true
    }   
  }

  /* Execute desired mode */
  if(switchCase == AZIMUTHSWEEP || switchCase == ELEVATIONSWEEP)           //implement 
  {
    if(switchCase == AZIMUTHSWEEP)
    {
      Serial.println("\nYou chose 1) Azimuth Sweep\n");
      Serial.println("Enter the desired elevation angle x in degrees with form <x>\nValid Range: x E [-150, 150].");
    }
    else
    {
      Serial.println("\nYou chose 2) Elevation Sweep\n");
      Serial.println("Enter the desired azimuth x in degrees with form <x>\nValid Range: x E [-150, 150].");
    }

    /* For Input */
    int              inputAngle;
    int              numIterations;
    int              angleInterval;
    unsigned int     delayTime;
    boolean          continueSweep = true;
    boolean          inputReceived = false;

    /* Wait for angle input */
    while(!inputReceived)
    {
      inputReceived = checkForInput();
      if(inputReceived)
      {
        if(buffer[0] == '?')                        
        {
          continueSweep = false;
        }
        else
        {
          inputAngle = atoi(buffer);
          if(!(inputAngle >= -150 && inputAngle <= 150))  
          {
            Serial.println("\nInvalid elevation angle. Please enter another or <?> for menu.\n");
            buffer[0]          = '\0';                     
            inputReceived = false;                
          }
        }
      }
    }

    if(continueSweep)
    {
      /* Inform User About Previous Input */
      if(switchCase == AZIMUTHSWEEP)
      {
        Serial.print("Desired Elevation Angle: "); 
        Serial.println(inputAngle);
      }
      else
      {
        Serial.print("Desired Azimuth Angle: "); 
        Serial.println(inputAngle);
      }

      /* Wait for desired angle interval */
      Serial.println("\nEnter the angle interval size y in degrees with form <y>. \nValid Range: y E [1, 300].");
      inputReceived = false;
      while(!inputReceived)
      {
        inputReceived = checkForInput();
        if(inputReceived)
        {
          if(buffer[0] == '?')
          {
            continueSweep = false;
          } 
          else
          {
            angleInterval = atoi(buffer);
            if( ! (angleInterval >=1 && angleInterval <=300) )
            {
              Serial.println("Invalid angle interval. Enter another or <?> for menu.\n");
              buffer[0]     = '\0';
              inputReceived = false;
            }
          }
        }
      }
    }

    if(continueSweep)
    {
      /* Inform User About Previous Input */
      Serial.print("Desired sweep angle interval: ");
      Serial.println(angleInterval);

      /* Request Desired Delay Time */
      Serial.println("\nEnter the time delay t in seconds with form <t>. Valid Range: t E [0, 65535].");
      inputReceived = false;
      while(!inputReceived)
      {
        inputReceived = checkForInput();
        if(inputReceived)
        {
          if(buffer[0] == '?')
          {
            continueSweep = false;
          }
          else
          {
            delayTime = atoi(buffer);
            if( ! (delayTime >= 0 && delayTime <=99999))
            {
              Serial.println("Invalid delay time. Enter another or <?> for menu.\n");
              buffer[0]     = '\0';
              inputReceived = false;
            }
          } 
        }
      }
    }

    if(continueSweep)
    {
      /* Inform User About Previous Input */
      Serial.print("Desired delay time: ");
      Serial.println(delayTime);

      /* Wait for desired number of iterations */
      Serial.println("\nPlease enter the number of sweep iterations y with form <y>.\nOne iteration is both forth and back.\nValid Range: y E [0,65535].");
      inputReceived = false;
      while(!inputReceived)
      {
        inputReceived = checkForInput();
        if(inputReceived)
        {
          if(buffer[0] == '?')
          {
            continueSweep = false;
          }
          else
          {
            numIterations = atoi(buffer);
            if(numIterations<0)
            {
              Serial.println("\nInvalid number of iterations. Please enter another or <?> for menu.\n");
              buffer[0]     = '\0';
              inputReceived = false;
            }
          } 
        }
      }
    }

    if(continueSweep)                    
    {

      /* Inform User About Previous Input */
      Serial.print("Desired Number of Iterations: "); 
      Serial.println(numIterations);
      Serial.println("\n");

      /* Execute azelSweep */
      configureMRM();
      modeStartTime = millis(); 
      int counter = 0;
      while( counter < numIterations && continueSweep == true)
      {
        if(switchCase==AZIMUTHSWEEP)
        {
          continueSweep = azelSweep(true, angleInterval, delayTime, inputAngle);
        }
        else
        {
          continueSweep = azelSweep(false, angleInterval, delayTime, inputAngle);
        }
        counter++;
      }
    }
  }
  else if(switchCase == POSITIONTICKS || switchCase == POSITIONDEGREES)          
  {
    /* Prompt User for Valid Azimuth Input */
    if(switchCase == POSITIONTICKS)
    {
      Serial.println("\nYou chose 3) Move to specific position (in ticks)\n");
      Serial.println("Enter desired azimuth angle in x in ticks with form <x>. \nValid Range: x E [0,1023]\n");
    }
    else
    {
      Serial.println("\nYou chose 4) Move to specific position (in degrees)\n");
      Serial.println("Enter desired azimuth angle in x in degrees with form <x>. \nValid Range: x E [-150,150]\n");
    }
    /* For Input */
    int     azimuthAngle, elevationAngle;
    boolean continueRequests  = true;
    boolean azimuthReceived   = false;
    boolean elevationReceived = false;

    /* Wait for Azimuth Input */
    while(!azimuthReceived)
    {
      azimuthReceived = checkForInput();
      if(azimuthReceived)
      {
        if(buffer[0] == '?')                        
        {
          continueRequests = false;
        }
        else
        {
          azimuthAngle = atoi(buffer);
          if(switchCase == POSITIONTICKS)
          {
            if(!(azimuthAngle >= 0 && azimuthAngle <= 1023))  
            {
              Serial.println("\nInvalid azimuthal angle. Please enter another or <?> for menu.\n");
              buffer[0]          = '\0';                     
              azimuthReceived = false;                
            }
          }
          else if(switchCase == POSITIONDEGREES)
          {
            if(!(azimuthAngle >= -150 && azimuthAngle <= 150))  
            {
              Serial.println("\nInvalid azimuthal angle. Please enter another or <?> for menu.\n");
              buffer[0]          = '\0';                     
              azimuthReceived = false;                
            }
          }
        }
      }
    }

    if(continueRequests)
    {
      /* Inform User About Previous Input */
      if(switchCase == POSITIONTICKS)
      {
        Serial.print("Desired Azimuth Angle in Ticks: "); 
        Serial.println(azimuthAngle);
        Serial.println("\nEnter desired elevation angle y in ticks with form <y>.\nValid Range: y E [0,1024].");
      }
      else
      {
        Serial.print("Desired Azimuth Angle in Degrees: "); 
        Serial.println(azimuthAngle);
        Serial.println("\nEnter desired elevation angle y in degrees with form <y>.\nValid Range: y E [-150,150].");
      }

      /* Request Elevation Input */
      while(!elevationReceived)
      {
        elevationReceived = checkForInput();
        if(elevationReceived)
        {
          if(buffer[0] == '?')
          {
            continueRequests = false;
          }
          else
          {
            elevationAngle = atoi(buffer);
            if(switchCase == POSITIONTICKS)
            {
              if(!(elevationAngle >= 0 && elevationAngle <= 1023))
              {
                Serial.println("\nInvalid elevation angle. Please enter another or <?> for menu.\n");
                buffer[0]          = '\0';
                elevationReceived = false;
              }
            }
            else
            {
              if(!(elevationAngle >= 0 && elevationAngle <= 1023))
              {
                Serial.println("\nInvalid elevation angle. Please enter another or <?> for menu.\n");
                buffer[0]          = '\0';
                elevationReceived = false;
              }
            }
          } 
        }
      }
    }

    if(continueRequests)
    {
      /* Inform User About Previous Input */
      if(switchCase == POSITIONTICKS)
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
  }
  else if(switchCase == READMRMONLY)
  {
    readMRMOnly();
  }
} 















