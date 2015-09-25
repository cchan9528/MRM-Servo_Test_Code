#ifndef MODES_H
#define MODES_H

#define MENUREQUEST                      0
#define SPHERE                           1
#define AZIMUTHSWEEP                     2
#define ELEVATIONSWEEP                   3
#define POSITIONTICKS                    4
#define POSITIONDEGREES                  5
#define READMRMONLY                      6
#define SERVOINFO                        7
#define CONFIGUREMRM                     8

#define AZ_SERVO                         1         
#define EL_SERVO                         2

bool sweeps(int sweepMode, int hotServoAngleInterval, int delayTime, int coldServoPosition);
void positionServos(bool sentTicks, int azimuthPos, int elevationPos);
void readMRMOnly();

#endif
