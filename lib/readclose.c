/* Public domain, from djbdns-1.05. */

#include <unistd.h>
#include "error.h"
#include "readclose.h"

int readclose_append(int fd,stralloc *sa,unsigned int bufsize)
{
  int r;
  for (;;) {
    stralloc_readyplus(sa,bufsize);
    r = read(fd,sa->s + sa->len,bufsize);
    if (r == -1) if (errno == error_intr) continue;
    if (r <= 0) { close(fd); return r; }
    sa->len += r;
  }
}

int readclose(int fd,stralloc *sa,unsigned int bufsize)
{
  stralloc_copys(sa,"");
  return readclose_append(fd,sa,bufsize);
}
