#include "stralloc.h"
#include "slurp.h"
#include "error.h"
#include "open.h"

int slurp(fn,sa,bufsize)
char *fn;
stralloc *sa;
int bufsize;
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
