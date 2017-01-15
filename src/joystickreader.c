#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "i2cbus.h"

#define SINGLE_CONVERSION_MASK (1 << 15)
#define SINGLE_CONVERSION (1 << 15)
#define CHANNEL_MASK ((0b111) << 12)
#define CHANNEL_0 ((0b100) << 12)
#define CHANNEL_1 ((0b101) << 12)

static int s_iFd;
static const unsigned int s_kuiAddress = 0x48;
static const unsigned int s_kuiConversionRegister = 0;
static const unsigned int s_kuiConfigRegister = 1;

static uint16_t s_u16GetConfigValueForSingleConversion(uint8_t u8Channel) {
  assert(u8Channel < 2);
  switch (u8Channel) {
  case 0:
    return SINGLE_CONVERSION | CHANNEL_0;
  case 1:
    return SINGLE_CONVERSION | CHANNEL_1;
  default:
    assert(false);
  }
}

static int s_iStartSingleConversion(uint8_t u8Channel) {
  int iResult;
  uint16_t u16RegValue;
  uint16_t u16Mask = SINGLE_CONVERSION_MASK | CHANNEL_MASK;
  uint16_t u16Value = s_u16GetConfigValueForSingleConversion(u8Channel);
  iResult = iReadI2cSmbusWord(s_iFd, s_kuiConfigRegister, &u16RegValue);
  if (iResult != 0)
    return iResult;
  return iWriteI2cSmbusWord(s_iFd, s_kuiConfigRegister,
			    ((u16RegValue & ~u16Mask) | u16Value));
}

static int s_iReadADCValue(uint8_t u8Channel, uint16_t *pu16Value) {
  int iResult;
  uint16_t u16Value;
  iResult = s_iStartSingleConversion(u8Channel);
  if (iResult != 0)
    return iResult;
  do {
    iResult = iReadI2cSmbusWord(s_iFd, s_kuiConfigRegister, &u16Value);
    if (iResult != 0)
      return iResult;
  } while (!(u16Value & SINGLE_CONVERSION_MASK));
  return iReadI2cSmbusWord(s_iFd, s_kuiConversionRegister, pu16Value);
}

int main() {
  int iResult;
  uint16_t u16Value0;
  uint16_t u16Value1;
  iResult = iOpenI2cBus(s_kuiAddress);
  if (iResult < 0)
    return iResult;
  s_iFd = iResult;
  while (true) {
    iResult = s_iReadADCValue(0, &u16Value0);
    if (iResult < 0)
      break;
    printf("Value channel 0: %u\n", u16Value0);
    usleep(200);
    iResult = s_iReadADCValue(1, &u16Value1);
    if (iResult < 0)
      break;
    printf("Value channel 1: %u\n", u16Value1);
    usleep(200);
    if (u16Value0 < 12000) {
      if (u16Value1 < 12000)
	printf("lower left corner\n");
      else if (u16Value1 > 26000)
	printf("upper left corner\n");
      else
	printf("left\n");
    } else if (u16Value0 > 26000) {
      if (u16Value1 < 12000)
	printf("lower right corner\n");
      else if (u16Value1 > 26000)
	printf("upper right corner\n");
      else
	printf("right\n");
    } else {
      if (u16Value1 < 12000)
	printf("down\n");
      else if (u16Value1 > 26000)
	printf("up\n");
      else
	printf("middle\n");
    }
    sleep(1);
  }
  iCloseI2cBus(s_iFd);
  return 0;
}
