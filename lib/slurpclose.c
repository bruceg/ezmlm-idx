#include <unistd.h>
#include "stralloc.h"
#include "readwrite.h"
#include "slurpclose.h"
#include "error.h"

int slurpclose(int fd,stralloc *sa,int bufsize)
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
