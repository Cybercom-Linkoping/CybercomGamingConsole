#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "i2cbus.h"

static const char* s_kacBusFilename = "/dev/i2c-1";

int iOpenI2cBus(const unsigned int kuiAddress) {
  int iFd;
  if ((iFd = open(s_kacBusFilename, O_RDWR)) < 0) {
    return (errno * (-1));
  }
  // The bus is open, now communicate to the device pointed out by address
  if (ioctl(iFd, I2C_SLAVE, kuiAddress) < 0) {
    return (errno * (-1));
  }
  return iFd;
}

static unsigned int s_uiSwapLowerBytes(const unsigned int kuiVal) {
  const unsigned int kuiTmp = kuiVal & 0xFFFF;
  return ((kuiTmp << 8) | (kuiTmp >> 8)) & 0xFFFF;
}

int iReadI2cSmbusWord(int iFd, const unsigned int kuiRegister, uint16_t *pu16Value) {
  int32_t result;
  assert(kuiRegister < 256);
  if ((result = i2c_smbus_read_word_data(iFd, kuiRegister)) < 0) {
    return (errno * (-1));
  }
  *pu16Value = (uint16_t)s_uiSwapLowerBytes(result);
  return 0;
}

int iReadI2cSmbusSignedWord(int iFd, const unsigned int kuiRegister, int16_t *pi16Value) {
  int32_t result;
  assert(kuiRegister < 256);
  if ((result = i2c_smbus_read_word_data(iFd, kuiRegister)) < 0) {
    return (errno * (-1));
  }
  *pi16Value = (int16_t)s_uiSwapLowerBytes(result);
  return 0;
}

int iWriteI2cSmbusWord(int iFd, const unsigned int kuiRegister, const uint16_t ku16Value) {
  uint16_t u16ValueToWrite;
  int32_t result;
  assert(kuiRegister < 256);
  u16ValueToWrite = (uint16_t)s_uiSwapLowerBytes(ku16Value);
  if ((result = i2c_smbus_write_word_data(iFd, kuiRegister, u16ValueToWrite)) < 0) {
    return (errno * (-1));
  }
  return 0;
}

int iCloseI2cBus(int iFd) {
  int result;
  if ((result = close(iFd)) < 0) {
    return (errno * (-1));
  }
  return 0;
}
