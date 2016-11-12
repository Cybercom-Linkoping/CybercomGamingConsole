#include <poll.h>
#include <stdio.h>
#include <fcntl.h>

static struct pollfd s_fileset;

int main()
{
  int pollresult;
  int fd = open("/sys/class/gpio/gpio415/value", O_RDWR | O_NONBLOCK);
  char buffer[256];
  
  if (fd <= 0)
  {
    printf("Could not open file!\n");
    return -1;
  }
  s_fileset.fd = fd;
  s_fileset.events = POLLPRI | POLLERR;
  pollresult = poll(&s_fileset, 1, -1);
  printf("poll result is: %d, revents: %d\n",
	 pollresult, s_fileset.revents);
  int n = read(fd, buffer, 256);
  printf("read %d bytes: %s\n", n, buffer);
  pollresult = poll(&s_fileset, 1, -1);
  printf("poll result is: %d, revents: %d\n",
	 pollresult, s_fileset.revents);
  close(fd);
  return 0;
}
