/*$Id$*/

#include "stralloc.h"
#include "strerr.h"
#include "errtxt.h"
#include "mime.h"
#include "die.h"
#include "idx.h"

/* takes a string pointed to by cpfrom and adds the next 'n' bytes to        */
/* outdata, replacing any Quoted-Printable codes with the real characters.   */
/* NUL and LF in the input are allowed, but anything that decodes to these   */
/* values is ignored. */

static const char char16table[128] = {
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
     0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,
    -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
};

#define char16enc(c)  (((c) & 0x80) ? -1 : char16table[(int)(c)])

void decodeQ(const char *cpfrom,unsigned int n,stralloc *outdata)
/* does Q decoding of the string pointed to by cpfrom up to the character */
/* before the one pointed to by cpnext, and appends the results to outdata */
{
  const char *cp,*cpnext;
  char ch1,ch2;		/* need to be signed */
  char ch;

  cp = cpfrom;
  cpnext = cp + n;
  if (!stralloc_readyplus(outdata,n)) die_nomem();

  while (cp < cpnext) {
    ch = *cp++;
    if (ch == '_') ch = ' ';		/* '_' -> space */
    if (ch == '=') {			/* "=F8" -> '\xF8' */
      if (*cp == '\n') {		/* skip soft line break */
        ++cp;
        continue;
      }
      ch1 = char16enc(*cp); ++cp;
      if (cp >= cpnext)
        break;
      ch2 = char16enc(*cp); ++cp;
      if (ch1 >= 0 && ch2 >= 0) {	/* ignore illegals */
        ch = (ch1 << 4 | ch2) & 0xff;
      }
    }
    if (!stralloc_append(outdata,&ch)) die_nomem();
  }
}      


