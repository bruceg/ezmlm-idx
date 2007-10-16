#include "msgtxt.h"
#include "mime.h"
#include "stralloc.h"
#include "strerr.h"
#include "die.h"
#include "idx.h"

static const char hexchar[16] = "0123456789ABCDEF";

void encodeQ(const char *indata,unsigned int n,stralloc *outdata)
	/* converts any character with the high order bit set to */
	/* quoted printable. In: n chars of indata, out: stralloc outdata*/
{
  char *cpout;
  char ch;
  unsigned int i;
  const char *cpin;

  cpin = indata;
  i = 0;
	/* max 3 outchars per inchar  & 2 char newline per 72 chars */
  if (!stralloc_copys(outdata,"")) die_nomem();
  if (!stralloc_ready(outdata,n * 3 + n/36)) die_nomem();	/* worst case */
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
