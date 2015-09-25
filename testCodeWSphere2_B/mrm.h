#ifndef MRM_H
#define MRM_H

/* MRM */
#define MRMADDRESS                       0x1E       //Slave Address
#define CONTINUOUSMODE                   0x00
#define SINGLESHOTMODE                   0x01 
#define MODEREGISTER                     0x02
#define FIRSTDATAREGISTER                0x03 
#define STATUSREGISTER                   0x09
#define X                                1
#define Y                                2
#define Z                                3

void configureMRM();
void mrmCollection();
int getMRMRegister(unsigned char letter);

#endif
