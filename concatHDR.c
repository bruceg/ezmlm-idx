/*$Id$*/

#include "stralloc.h"
#include "strerr.h"
#include "case.h"
#include "byte.h"
#include "mime.h"
#include "errtxt.h"
#include "die.h"
#include "idx.h"

void concatHDR(char *indata,
	       unsigned int n,
	       stralloc *outdata)
/* takes a concatenated string of line and continuation line, trims leading */
/* and trailing LWSP and collapses line breaks and surrounding LWSP to ' '. */
/* indata has to end in \n or \0 or this routine will write beyond indata!  */
/* if indata ends with \0, this will be changed to \n. */

{
  char *cp;
  char *cpout;
  char *cplast;
  if (!stralloc_copys(outdata,"")) die_nomem();
  if (!stralloc_ready(outdata,n)) die_nomem();
  cpout = outdata->s;
  if (n == 0) return;
  cplast = indata + n - 1;
  cp = cplast;
  while (cplast >= indata && (*cplast == '\0' || *cplast == '\n'))
    --cplast;
  if (cp == cplast) die_nomem();		/* just in case */
  *(++cplast) = '\n';				/* have terminal '\n' */
  cp = indata;
  while (cp <= cplast) {
    while (*cp == ' ' || *cp == '\t') ++cp;	/* LWSP before */
    while (*cp != '\n') *(cpout++) = *(cp++);	/* text */
    ++cp;					/* skip \n */ 
    --cpout;					/* last char */
    while (cpout >= outdata->s && (*cpout == ' ' || *cpout == '\t'))
      --cpout;	/* LWSP after */
    *(++cpout) = ' ';				/* replace with single ' ' */
    ++cpout;					/* point to free byte */
  }
  outdata->len = cpout - outdata->s;
}

