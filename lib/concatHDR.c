#include "stralloc.h"
#include "strerr.h"
#include "case.h"
#include "byte.h"
#include "mime.h"
#include "messages.h"
#include "die.h"
#include "idx.h"

void concatHDR(const char *indata,
	       unsigned int n,
	       stralloc *outdata)
/* takes a concatenated string of line and continuation line, trims leading */
/* and trailing LWSP and collapses line breaks and surrounding LWSP to ' '. */
{
  const char *cp;
  char *cpout;
  const char *cplast;
  /* Skip leading whitespace */
  while (n > 0 && (*indata == ' ' || *indata == '\t' || *indata == '\n')) {
    ++indata;
    --n;
  }
  if (!stralloc_copys(outdata,"")) die_nomem();
  if (!stralloc_ready(outdata,n)) die_nomem();
  cpout = outdata->s;
  if (n == 0) return;
  cplast = indata + n - 1;
  cp = cplast;
  /* Trim trailing whitespace */
  while (cplast >= indata && (*cplast == '\0' || *cplast == '\n' || *cplast == '\t' || *cplast == ' '))
    --cplast;
  if (cp == cplast) die_nomem();		/* just in case */
  cp = indata;
  while (cp <= cplast) {
    while (cp <= cplast && (*cp == ' ' || *cp == '\t')) /* LWSP before */
      ++cp;
    while (cp <= cplast && *cp != '\n')         /* text */
      *(cpout++) = *(cp++);
    ++cp;					/* skip \n */ 
    --cpout;					/* last char */
    while (cpout >= outdata->s && (*cpout == ' ' || *cpout == '\t'))
      --cpout;	/* LWSP after */
    *(++cpout) = ' ';				/* replace with single ' ' */
    ++cpout;					/* point to free byte */
  }
  outdata->len = cpout - outdata->s;
}
