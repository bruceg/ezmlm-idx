#include "substdio.h"
#include "byte.h"
#include "stralloc.h"
#include "getln.h"

int getln(substdio *ss,stralloc *sa,int *match,int sep)
{
  char *cont;
  unsigned int clen;

  stralloc_ready(sa,0);
  sa->len = 0;

  if (getln2(ss,sa,&cont,&clen,sep) == -1) return -1;
  if (!clen) { *match = 0; return 0; }
  stralloc_catb(sa,cont,clen);
  *match = 1;
  return 0;
}
