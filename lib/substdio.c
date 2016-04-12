#include "substdio.h"

void substdio_fdbuf(substdio *s,substdio_fn op,int fd,char *buf,int len)
{
  s->x = buf;
  s->fd = fd;
  s->op = op;
  s->p = 0;
  s->n = len;
}
