#include "lock.h"
#include "msgtxt.h"
#include "strerr.h"
#include "die.h"
#include "idx.h"
#include "open.h"

int lockfile(const char *path)
{
  int fd;
  fd = open_append(path);
  if (fd == -1)
    strerr_die4sys(111,FATAL,MSG("ERR_OPEN"),path,": ");
  if (lock_ex(fd) == -1)
    strerr_die4sys(111,FATAL,MSG("ERR_OBTAIN"),path,": ");
  return fd;
}
