/* Public domain, from djbdns-1.05. */

#include <sys/types.h>
#include <fcntl.h>
#include "ndelay.h"

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

int ndelay_on(int fd)
{
  return fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0) | O_NONBLOCK);
}
