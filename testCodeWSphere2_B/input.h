#ifndef INPUT_H
#define INPUT_H

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

extern char buffer[7];           

bool checkForInput();
bool requestFromUser(int variableCode, int *neededVariable);

#endif
