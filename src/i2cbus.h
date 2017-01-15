#ifndef I2CBUS_H
#define I2CBUS_H

#include <stdint.h>

int iOpenI2cBus(const unsigned int kuiAddress);
int iReadI2cSmbusWord(int iFd, const unsigned int kuiRegister, uint16_t *pu16Value);
int iReadI2cSmbusSignedWord(int iFd, const unsigned int kuiRegister, int16_t *pi16Value);
int iWriteI2cSmbusWord(int iFd, const unsigned int kuiRegister, const uint16_t ku16Value);
int iCloseI2cBus(int iFd);

#endif // I2CBUS_H
