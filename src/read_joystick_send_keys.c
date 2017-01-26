#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/time.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>

#include "i2cbus.h"

#define SINGLE_CONVERSION_MASK (1 << 15)
#define SINGLE_CONVERSION (1 << 15)
#define CHANNEL_MASK ((0b111) << 12)
#define CHANNEL_0 ((0b100) << 12)
#define CHANNEL_1 ((0b101) << 12)

/* Globals */
static int s_iFd;
static const unsigned int s_kuiAddress = 0x48;
static const unsigned int s_kuiConversionRegister = 0;
static const unsigned int s_kuiConfigRegister = 1;

static int uinp_fd = -1;
struct uinput_user_dev uinp; // uInput device structure
struct input_event event; // Input device structure

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

/* Setup the uinput device */
int setup_uinput_device()
{
  // Temporary variable
  int i=0;
  // Open the input device
  uinp_fd = open("/dev/uinput", O_WRONLY | O_NDELAY);
  if (uinp_fd < 0)
    {
      printf("Unable to open /dev/uinput, error code: %d\n", errno);
      return -1;
    }
  memset(&uinp,0,sizeof(uinp)); // Intialize the uInput device to NULL
  strncpy(uinp.name, "cybercomgamingconsole", UINPUT_MAX_NAME_SIZE);
  uinp.id.version = 4;
  uinp.id.bustype = BUS_USB;
  // Setup the uinput device
  ioctl(uinp_fd, UI_SET_EVBIT, EV_KEY);
  ioctl(uinp_fd, UI_SET_EVBIT, EV_REL);
  ioctl(uinp_fd, UI_SET_RELBIT, REL_X);
  ioctl(uinp_fd, UI_SET_RELBIT, REL_Y);
  for (i=0; i < 256; i++) {
    ioctl(uinp_fd, UI_SET_KEYBIT, i);
  }
  ioctl(uinp_fd, UI_SET_KEYBIT, BTN_MOUSE);
  ioctl(uinp_fd, UI_SET_KEYBIT, BTN_TOUCH);
  ioctl(uinp_fd, UI_SET_KEYBIT, BTN_MOUSE);
  ioctl(uinp_fd, UI_SET_KEYBIT, BTN_LEFT);
  ioctl(uinp_fd, UI_SET_KEYBIT, BTN_MIDDLE);
  ioctl(uinp_fd, UI_SET_KEYBIT, BTN_RIGHT);
  ioctl(uinp_fd, UI_SET_KEYBIT, BTN_FORWARD);
  ioctl(uinp_fd, UI_SET_KEYBIT, BTN_BACK);
  /* Create input device into input sub-system */
  write(uinp_fd, &uinp, sizeof(uinp));
  if (ioctl(uinp_fd, UI_DEV_CREATE))
    {
      printf("Unable to create UINPUT device.");
      return -1;
    }
  return 1;
}

void send_keyevent(unsigned int key, int value)
{
  memset(&event, 0, sizeof(event));
  gettimeofday(&event.time, NULL);
  event.type = EV_KEY;
  event.code = key;
  event.value = value;
  write(uinp_fd, &event, sizeof(event));
  event.type = EV_SYN;
  event.code = SYN_REPORT;
  event.value = 0;
  write(uinp_fd, &event, sizeof(event));
}

void send_keypress(unsigned int key)
{
  send_keyevent(key, 1);
}

void send_keyrelease(unsigned int key)
{
  send_keyevent(key, 0);
}

void send_key(unsigned int key)
{
  // Report key press event
  send_keypress(key);
  // Report key release event
  send_keyrelease(key);
}

int main(void)
{
  int iFd, iPollresult;
  char acBuffer[11];

  // Return an error if device not found.
  if (setup_uinput_device() < 0)
    {
      printf("Unable to find uinput device\n");
      return -1;
    }

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
    //printf("Value channel 0: %u\n", u16Value0);
    usleep(100);
    iResult = s_iReadADCValue(1, &u16Value1);
    if (iResult < 0)
      break;
    //printf("Value channel 1: %u\n", u16Value1);
    usleep(100);
    if (u16Value0 < 12000) {
      send_key(KEY_LEFT);
    } else if (u16Value0 > 26000) {
      send_key(KEY_RIGHT);
    }
    if (u16Value1 < 10000)
      send_key(KEY_DOWN);
    else if (u16Value1 > 28000)
      send_key(KEY_UP);
    usleep(100000);
  }
  iCloseI2cBus(s_iFd);

  /* Destroy the input device */
  ioctl(uinp_fd, UI_DEV_DESTROY);
  /* Close the UINPUT device */
  close(uinp_fd);
  return 0;
}
