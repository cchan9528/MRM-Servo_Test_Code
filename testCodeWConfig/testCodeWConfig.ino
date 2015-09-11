//in next version, abstract the reading from wire out
//in this version, write the rest of the configure mrm code


#include <ax12.h>                                   //Servos
//#include <BioloidController.h>                      
#include <Wire.h>                                   //I2C

/* Servos */
#define AZ_SERVO                         1         
#define EL_SERVO                         2

/* Modes */
#define MENUREQUEST                      0
#define SPHERE                           1
#define AZIMUTHSWEEP                     2
#define ELEVATIONSWEEP                   3
#define POSITIONTICKS                    4
#define POSITIONDEGREES                  5
#define READMRMONLY                      6
#define CONFIGUREMRM                     7

/* Request Variables */
#define MODE                             0
#define HOTANGLEINTERVAL                 1
#define COLDANGLEINTERVAL                2
#define DELAYTIME                        3
#define ANGLETICKS                       4          //need these to align
#define ANGLEDEGREES                     5          //with the #define in modes
#define ITERATIONS                       6
#define MRMOUTPUTRATE                    7
#define MRMSHOTMODE                      8
#define MRMRESOLUTION                    9
#define MRMSAMPLESIZE                    10
#define MRMBIAS                          11

/* MRM */
#define MRMADDRESS                       0x1E       //Slave Address
#define CONTINUOUSMODE                   0x00
#define SINGLESHOTMODE                   0x01 
#define SHOTREGISTER                     0x02
#define OUTPUTREGISTER                   0x00
#define GAINREGISTER                     0x01
#define FIRSTDATAREGISTER                0x03 
#define STATUSREGISTER                   0x09
int  xRegister, zRegister, yRegister, elServoPos, azServoPos;
byte registerA;
unsigned long modeStartTime;

/* Input */
const  char startByte                  = '<';
const  char stopByte                   = '>';        // Input 
const  byte bufferSizeLimit            =  7 ;
static char buffer[bufferSizeLimit];                
static byte bufferIndex                =  0 ;        //byte limited to 0-255

/* Execution */
boolean continueLoop;
//////////////////////////////
// Display Functions
//////////////////////////////

void displayMenuOptions()
{
  Serial.print("\n");
  Serial.println("Enter an option with form <#> where # is a number between 1-5, inclusive, or <?> for menu.");
  Serial.print("\n");
  Serial.println("###########################");
  Serial.println("1) Spherical Sweep.");                                 // Con 
  Serial.println("2) Azimuth Sweep.");                               // Con
  Serial.println("3) Elevation Sweep.");         // Con
  Serial.println("4) Move to specific position (in ticks). No MRM Collection Done.");       // Dis
  Serial.println("5) Move to specific position (in degrees). No MRM Collection Done.");                               // Dis; Just read relative to position it's at now
  Serial.println("6) Read MRM Values.");
  Serial.println("7) Configure MRM");
  Serial.println("###########################");
}

void displayTableHeaders()
{
  Serial.println("1. Azimuth in Ticks\n2. Elevation in Ticks\n3. Azimuth in Degrees [0,300] (corresponds to [150,150])\n4. Elevation in Degrees [0,300] (corresponds to [-150,150]\n5. X-Axis\n6. Y-Axis\n7. Z-Axis\n8. Time in Milliseconds");
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

boolean requestFromUser(int variableCode, int * neededVariable )
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
        else if(variableCode == MRMSAMPLESIZE)
        {
          if(!(valueNeeded == 1 || valueNeeded == 2 || valueNeeded == 4 || valueNeeded == 8))
          {
            Serial.println("\nInvalid sample group size. Please enter another or <?> for menu.\n");
            invalidInputSent = true;
          }
        }
        else if(variableCode == MRMSHOTMODE)
        {
          if(!(valueNeeded == 0 || valueNeeded == 1))
          {
            Serial.println("\nInvalid shot mode selection. Please enter another or <?> for menu.\n");
            invalidInputSent = true;
          }
        }
        else if(variableCode == MRMOUTPUTRATE)
        {
          if(!(valueNeeded >= 0 && valueNeeded <= 6))
          {
            Serial.println("\nInvalid output rate. Please enter another or <?> for menu.\n");
            invalidInputSent = true;
          }
        }
        else if(variableCode == MRMBIAS)
        {
          if(!(valueNeeded == 0 || valueNeeded == 1 || valueNeeded == 2))
          {
            Serial.println("\nInvalid bias. Please enter another or <?> for menu.\n");
            invalidInputSent = true;
          }
        }
        else if(variableCode == MRMRESOLUTION)
        {
          if(!(valueNeeded == 1370 || valueNeeded == 1090 || valueNeeded == 820 || valueNeeded == 660 || valueNeeded == 440 || valueNeeded == 390 || valueNeeded == 330 || valueNeeded == 230))
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


///////////////////////////////
// Servo Functions
///////////////////////////////

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

///////////////////////////////
// MRM Functions
///////////////////////////////
byte configureMRM(int shotSelect, int dataSizeSelect, int outputSelect, int biasSelect, int gainSelect)
{
  /* Select MRM Mode (see top of code for explanation) */
  Wire.beginTransmission(MRMADDRESS);
  Wire.write(SHOTREGISTER);
  if   (shotSelect == 1) {
    Wire.write(0x01);
  }
  else                   {
    Wire.write(CONTINUOUSMODE);
  }
  Wire.endTransmission();

  /* Select MRM Output Data, Samples Averaged, and Bias to Register */
  registerA = 0b00000000;
  switch(dataSizeSelect)
  {
    case 1: 
      break;
    case 2:
      registerA = registerA | 0b00100000;
      break;
    case 4:
      registerA = registerA | 0b01000000;
      break;
    case 8:
      registerA = registerA | 0b01100000;
      break;
  }
  switch(outputSelect)
  {
    case 0:
      registerA = registerA | 0b00000000;
      break;
    case 1:
      registerA = registerA | 0b00000100;
      break;
    case 2:
      registerA = registerA | 0b00001000;
      break;
    case 3:
      registerA = registerA | 0b00001100;
      break;
    case 4:
      registerA = registerA | 0b00010000;
      break;
    case 5:
      registerA = registerA | 0b00010100;
      break;
    case 6:
      registerA = registerA | 0b00011000;   
  }
  switch(biasSelect)
  {
    case 0:
      break;
    case 1:
      registerA = registerA | 0b00000001;
      break;
    case 2:
      registerA = registerA | 0b00000010;
  }
  Wire.beginTransmission(MRMADDRESS);
  Wire.write(0x00);
  Wire.write(registerA);
  Wire.endTransmission();

  /*Resolution Select*/
  Wire.beginTransmission(MRMADDRESS);
  Wire.write(0x01);
  switch(gainSelect)
  {
  case 1370:
    Wire.write(0b00000000);
    break;
  case 820:
    Wire.write(0b01000000);
    break;
  case 660:
    Wire.write(0b01100000);
    break;
  case 440:
    Wire.write(0b10000000);
    break;
  case 390:
    Wire.write(0b10100000);
    break;
  case 330:
    Wire.write(0b11000000);
    break;
  case 230:
    Wire.write(0b11100000);
    break;
  case 1090:
  default:
    Wire.write(0b00100000);
  }
  Wire.endTransmission();
}

boolean checkMRMRegisters(int shotSelect, int dataSizeSelect, int outputSelect, int biasSelect, int gainSelect)
{
  byte readByte;

  /* Move to Shot Register */
  Wire.beginTransmission(MRMADDRESS);
  Wire.write(SHOTREGISTER);
  Wire.endTransmission();

  /* Read From Output Register and Verify Correctness */
  Wire.requestFrom(MRMADDRESS, 1);
  while(Wire.available())
  {
    readByte = Wire.read();
    if(readByte>=0) { 
      break; 
    }
  }
  if( ((byte)shotSelect) != readByte)
  { 
    return false; 
  }

  /* Move to Output Register */
  Wire.beginTransmission(MRMADDRESS);
  Wire.write(OUTPUTREGISTER);
  Wire.endTransmission();

  /* Read From Output Register and Verify Correctness */
  Wire.requestFrom(MRMADDRESS, 1);
  while(Wire.available())
  {
    readByte = Wire.read();
    if(readByte>=0) { 
      break; 
    }
  }
  if(registerA != readByte) 
  {
    return false; 
  }

  /*Move to Gain/Resolution Register */
  Wire.beginTransmission(MRMADDRESS);
  Wire.write(GAINREGISTER);
  Wire.endTransmission();

  /* Read From Gain/Resolution Register and Verify Correctness*/
  Wire.requestFrom(MRMADDRESS, 1);
  while(Wire.available())
  {
    readByte = Wire.read();
    if(readByte >= 0) {
      break;
    }
  }
  if( ((byte) gainSelect) != readByte)
  { 
    return false; 
  }

  /* Return True if Everything is Correct */
  return true;
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
boolean sweeps(int sweepMode, int hotServoAngleInterval, int delayTime, int coldServoPosition )
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

    /* Sweep selected servo; Wait Until Destination Reached */
    if(sweepMode == AZIMUTHSWEEP || sweepMode == SPHERE)
    {
      SetPosition(AZ_SERVO, destination);
      while(!(ax12GetRegister(AZ_SERVO,AX_PRESENT_POSITION_L,2) == destination)){}
    }
    else if(sweepMode == ELEVATIONSWEEP)
    {
      SetPosition(EL_SERVO, destination);
      while(!(ax12GetRegister(EL_SERVO,AX_PRESENT_POSITION_L,2) == destination)){} 
    }

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

  /*For the Loop*/
  continueLoop = true;
}

void loop()
{
  int mode = -2; 
  int *modePointer = &mode;
  boolean inputReceived = false;

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
    configureMRM(0, 1, 4, 0,1090);
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
      Serial.println("\nEnter desired elevation angle y in ticks with form <y>.\nValid Range: y E [0,1024].");
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

  else if(mode == CONFIGUREMRM)
  {
    Serial.println("\nYou chose 7) Configure MRM. \n");

    int     outputRate, shotMode, resolution, sampleSize, bias;
    int     *outputRatePtr        = &outputRate;
    int     *sampleSizePtr        = &sampleSize;
    int     *biasPtr              = &bias;
    int     *resolutionPtr        = &resolution;
    int     *shotModePtr          = &shotMode;
    boolean continueConfiguration = true;
    boolean mrmConfigured         = false;
    
    Serial.println("Enter shot mode s in form <s>.");
    Serial.println("s E {0,1} where 1 is single-shot, 0 is continuous-shot.");
    Serial.println("0 is Recommended.\n");
    continueConfiguration = requestFromUser(MRMSHOTMODE, shotModePtr); 
    if(!continueConfiguration) {
      return;
    }
    Serial.print("Desired shot mode: ");
    Serial.println(shotMode);
    delay(250);
    
    Serial.println("\nEnter number of samples averaged per output o in form <o>.");
    Serial.println("o E {1,2,4,8}. \n1 sample/output is Recommended.\n");
    continueConfiguration = requestFromUser(MRMSAMPLESIZE, sampleSizePtr); 
    if(!continueConfiguration) {
      return;
    }
    Serial.print("Desired number of samples averaged per output: ");
    Serial.println(sampleSize);
    delay(250);

    Serial.println("\nEnter data output rate r in form <r>. r E [0,6]. ");
    Serial.println("NOTE: r E {.75, 1.5, 3, 7.5, 15, 30, 75} in Hz, respectively.");
    Serial.println("15Hz is Default.\n");
    continueConfiguration = requestFromUser(MRMOUTPUTRATE, outputRatePtr);
    if(!continueConfiguration) {
      return;
     }
     ////////////////////////
     // Ran out of SRAM 
     // 1) change data types to create more memory
     // 2) look to PROGMEM online
     ////////////////////////
     
   // Serial.print("Desired output rate index: ");
//    Serial.println(outputRate);
    
////    Serial.println("\nEnter bias b in form <b>. g E {0,1,2} which corresponds to no bias, positive bias, negative bias, respectively. \n0 is Default.");
////    continueConfiguration = requestFromUser(MRMBIAS, biasPtr);
////    if(!continueConfiguration) {
////      return;
////    }
////    Serial.print("Desired bias index: ");
////    Serial.println(bias);
////
////    Serial.println("\nEnter gain in form <g>. g E {1370,1090,820,660,440,390,330,230} in LSb/Gauss.\nThe associated digital resolution is {.73,.92,1.22,1.52,2.27,2.56,3.03,4.35} in mG/LSb, respectively.\n1090/.92 is Default.\n");
////    continueConfiguration = requestFromUser(MRMRESOLUTION, resolutionPtr); 
////    if(!continueConfiguration) {
////      return;
////    }
////    Serial.print("Desired gain/resolution: ");
////    Serial.println(resolution);
////
////    mrmConfigured = checkMRMRegisters(shotMode, sampleSize, outputRate, bias, resolution);
////    while(!mrmConfigured) { 
////      configureMRM(shotMode, sampleSize, outputRate, bias, resolution); 
////    }
    Serial.println("\n\nMRM configured.");
  }

}


