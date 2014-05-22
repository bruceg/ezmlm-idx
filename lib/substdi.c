#include "substdio.h"
#include "byte.h"
#include "error.h"

static int oneread(substdio_fn op,int fd,char *buf,int len)
{
  int r;

  for (;;) {
    r = op(fd,buf,len);
    if (r == -1) if (errno == error_intr) continue;
    return r;
  }
}

static int getthis(substdio *s,char *buf,int len)
{
  int r;
  int q;
 
  r = s->p;
  q = r - len;
  if (q > 0) { r = len; s->p = q; } else s->p = 0;
  byte_copy(buf,r,s->x + s->n);
  s->n += r;
  return r;
}

int substdio_feed(substdio *s)
{
  int r;
  int q;

  if (s->p) return s->p;
  q = s->n;
  r = oneread(s->op,s->fd,s->x,q);
  if (r <= 0) return r;
  s->p = r;
  q -= r;
  s->n = q;
  if (q > 0) /* damn, gotta shift */ byte_copyr(s->x + q,r,s->x);
  return r;
}

int substdio_bget(substdio *s,char *buf,int len)
{
  int r;
 
  if (s->p > 0) return getthis(s,buf,len);
  r = s->n; if (r <= len) return oneread(s->op,s->fd,buf,r);
  r = substdio_feed(s); if (r <= 0) return r;
  return getthis(s,buf,len);
}

int substdio_get(substdio *s,char *buf,int len)
{
  int r;
 
  if (s->p > 0) return getthis(s,buf,len);
  if (s->n <= len) return oneread(s->op,s->fd,buf,len);
  r = substdio_feed(s); if (r <= 0) return r;
  return getthis(s,buf,len);
}

const char *substdio_peek(substdio *s)
{
  return s->x + s->n;
}

void substdio_seek(substdio *s,int len)
{
  s->n += len;
  s->p -= len;
}
