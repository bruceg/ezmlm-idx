/*$Id$*/

#include "qmail.h"
#include "substdio.h"

static struct qmail *qqptr;
static int qqwrite(int fd,const char *buf,unsigned int len)
{
  qmail_put(qqptr,buf,len);
  return len;
  (void)fd;
}

int qmail_copy(struct qmail *qq,substdio *ss)
{
  substdio ssqq = SUBSTDIO_FDBUF(qqwrite,-1,0,0);
  qqptr = qq;
  return substdio_copy(&ssqq,ss);
}
