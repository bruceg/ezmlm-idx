/*$Id$*/

#include "stralloc.h"
#include "slurp.h"
#include "slurpclose.h"
#include "error.h"
#include "open.h"

int slurp(const char *fn,stralloc *sa,int bufsize)
{
  int fd;
  fd = open_read(fn);
  if (fd == -1) {
    if (errno == error_noent) return 0;
    return -1;
  }
  if (slurpclose(fd,sa,bufsize) == -1) return -1;
  return 1;
}
