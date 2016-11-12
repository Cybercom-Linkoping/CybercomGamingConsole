#include <poll.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>

#define	NUM_BUTTONS 4u

static struct pollfd s_stPollFds[NUM_BUTTONS];
static const unsigned int s_kauiButtonIds[NUM_BUTTONS] = { 412, 413, 414, 415 }\
;
static unsigned	int s_auiButtonValues[NUM_BUTTONS] = { 1, 1, 1, 1 };
static char s_aacPathToValueFile[NUM_BUTTONS][100];

static const char* s_kacPathToExport = "/sys/class/gpio/export";
static const char* s_kacPathToGpio = "/sys/class/gpio/gpio";

static void s_vWriteToFile(const char *kacFilename, const char *kacData)
{
    FILE *fp = fopen(kacFilename, "w");
    if (fp != NULL)
    {
      fputs(kacData, fp);
        fclose(fp);
    }
}

static void s_vGetPathToGpioFile(char *acFilename, const char *kacGpioFile,
				 unsigned int uiButtonId)
{
  sprintf(acFilename, "%s%u/%s", s_kacPathToGpio, uiButtonId, kacGpioFile);
}

static void s_vSetupButtons(void)
{
  char acButtonIdString[4];
  char acPathToEdgeFile[100];
  char acPathToDirectionFile[100];
  
  for (unsigned int i = 0; i < NUM_BUTTONS; ++i)
  {
    unsigned int uiButtonId = s_kauiButtonIds[i];
    sprintf(acButtonIdString, "%u", uiButtonId);
    s_vWriteToFile(s_kacPathToExport, acButtonIdString);
    s_vGetPathToGpioFile(acPathToDirectionFile, "direction", uiButtonId);
    s_vWriteToFile(acPathToDirectionFile, "in");
    s_vGetPathToGpioFile(acPathToEdgeFile, "edge", uiButtonId);
    s_vWriteToFile(acPathToEdgeFile, "both");
    s_vGetPathToGpioFile(s_aacPathToValueFile[i], "value", uiButtonId);
  }
}

int main()
{
  int iFd, iPollresult;
  char acBuffer[10];

  // initialize variables and gpio
  s_vSetupButtons();
  for (unsigned int i = 0; i < NUM_BUTTONS; ++i)
  {
    // open value file for reading
    iFd = open(s_aacPathToValueFile[i], O_RDWR | O_NONBLOCK);
    if (iFd <= 0)
    {
      // should never happen
      printf("Could not open file! %s\n", s_aacPathToValueFile[i]);
      return -1;
    }
    s_stPollFds[i].fd = iFd;
    // set poll flags according to sysfs documentation
    s_stPollFds[i].events = POLLPRI | POLLERR;
  }
  while (true)
  {
    // wait for input
    iPollresult = poll(s_stPollFds, NUM_BUTTONS, -1);
    //printf("iPollresult: %d\n", iPollresult);
    if (iPollresult <= 0)
    {
      // ignore any poll errors
      printf("Error, got poll result: %d\n", iPollresult);
      continue;
    }
      
    for (unsigned int i = 0; i < NUM_BUTTONS; ++i)
    {
      // check button
      if (s_stPollFds[i].revents & POLLPRI)
      {
	// button state has changed, read current value
	read(s_stPollFds[i].fd, acBuffer, sizeof(acBuffer));
	// buffer will always contain a single digit
	sscanf(acBuffer, "%u", &s_auiButtonValues[i]);
	// need to reset file pointer
	lseek(s_stPollFds[i].fd, 0, SEEK_SET);
      }
      // print status
      if (i == 0)
	printf("button ");
      printf("%u is %s", i + 1, s_auiButtonValues[i] ? "not pressed" : "pressed    ");
      if (i < NUM_BUTTONS - 1)
	printf(", ");
    }
    printf("\n");
  }
  // will never reach here
  return 0;
}
