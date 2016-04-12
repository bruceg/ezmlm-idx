#include "substdio.h"
#include "byte.h"
#include "stralloc.h"
#include "getln.h"

int gethdrln(substdio *ss,stralloc *sa,int *match,int sep)
{
  char *cont;
  unsigned int clen;
  char *x;
 
  stralloc_ready(sa,0);
  sa->len = 0;
 
  do {
    if (getln2(ss,sa,&cont,&clen,sep)) return -1;
    if (!clen) { *match = 0; return 0; }
    stralloc_catb(sa,cont,clen);
  } while(sa->len > 1 && (x = substdio_PEEK(ss)) != 0 && (*x == ' ' || *x == '\t'));
  *match = 1;
  return 0;
}
