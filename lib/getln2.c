#include "substdio.h"
#include "stralloc.h"
#include "byte.h"
#include "getln.h"

int getln2(substdio *ss,stralloc *sa,
	   /*@out@*/char **cont,/*@out@*/unsigned int *clen,int sep)
{
  char *x;
  unsigned int i;
  int n;
 
  for (;;) {
    n = substdio_feed(ss);
    if (n < 0) return -1;
    if (n == 0) { *clen = 0; return 0; }
    x = substdio_PEEK(ss);
    i = byte_chr(x,n,sep);
    if (i < (unsigned int)n) { substdio_SEEK(ss,*clen = i + 1); *cont = x; return 0; }
    if (!stralloc_readyplus(sa,n)) return -1;
    i = sa->len;
    sa->len = i + substdio_get(ss,sa->s + i,n);
  }
}
