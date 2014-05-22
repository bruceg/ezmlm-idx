#include "stralloc.h"
#include "strerr.h"
#include "sys/uint32.h"
#include "messages.h"
#include "die.h"
#include "idx.h"

	/* Characters and translation as per rfc2047. */
static const char char64table[128] = {
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
    52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-1,-1,-1,
    -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
    -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
};

#define char64enc(c)  (((c) & 0x80) ? -1 : char64table[(c)])

void decodeB(const char *cpfrom,unsigned int n,stralloc *outdata)
/* does B decoding of the string pointed to by cpfrom up to the character */
/* before the one pointed to by cpnext, and appends the results to mimeline*/
{
  uint32 hold32;
  char holdch[4] = "???";
  int i,j;
  char c;	/* needs to be signed */
  const char *cp;
  const char *cpnext;

  cp = cpfrom;
  cpnext = cp + n;
  i = 0;
  hold32 = 0L;
  if (!stralloc_readyplus(outdata,n)) die_nomem();
  for (;;) {
    if (i == 4) {
      for (j = 2; j >= 0; --j) {
        holdch[j] = hold32 & 0xff;
        hold32 = hold32 >> 8;
      }
      if (!stralloc_cats(outdata,holdch)) die_nomem();
      if (cp >= cpnext)
        break;
      hold32 = 0L;
      i = 0;
    }
    if (cp >= cpnext) {	/* pad */
      c = 0;
    } else {
      c = char64enc((unsigned)*cp);
      ++cp;
    }
    if (c < 0)		/* ignore illegal characters */
      continue;
    else {
      hold32 = (hold32 << 6) | (c & 0x7f);
      ++i;
    }
  }
}

