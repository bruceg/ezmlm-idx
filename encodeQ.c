/* $Id$*/

#include "errtxt.h"
#include "mime.h"
#include "stralloc.h"
#include "strerr.h"

static void die_nomem(fatal)
  char *fatal;
{
  strerr_die2x(111,fatal,ERR_NOMEM);
}

static const char hexchar[] = "0123456789ABCDEF";

void encodeQ(indata,n,outdata,fatal)
char *indata;
unsigned int n;
stralloc *outdata;
char *fatal;

	/* converts any character with the high order bit set to */
	/* quoted printable. In: n chars of indata, out: stralloc outdata*/

{
  register char *cpout;
  register char ch;
  unsigned int i;
  char *cpin;

  cpin = indata;
  i = 0;
	/* max 3 outchars per inchar  & 2 char newline per 72 chars */
  if (!stralloc_copys(outdata,"")) die_nomem(fatal);
  if (!stralloc_ready(outdata,n * 3 + n/36)) die_nomem(fatal);	/* worst case */
  cpout = outdata->s;
  while (n--) {
    ch = *cpin++;
    if (ch != ' ' && ch != '\n' && ch != '\t' &&
          (ch > 126 || ch < 33 || ch == 61)) {
      *(cpout++) = '=';
      *(cpout++) = hexchar[(ch >> 4) & 0xf];
      *(cpout++) = hexchar[ch & 0xf];
      i += 3;
    } else {
      if (ch == '\n')
        i = 0;
      *(cpout++) = ch;
    }
    if (i >= 72) {
      *(cpout++) = '=';
      *(cpout++) = '\n';
      i = 0;
    }
  }
  outdata->len = (unsigned int) (cpout - outdata->s);
}
