/*$Id: decodeQ.c,v 1.3 1998/10/29 21:48:24 lindberg Exp $*/
/*$Name: ezmlm-idx-040 $*/

#include "stralloc.h"
#include "strerr.h"
#include "errtxt.h"
#include "mime.h"

/* takes a string pointed to by cpfrom and adds the next 'n' bytes to        */
/* outdata, replacing any Quoted-Printable codes with the real characters.   */
/* NUL and LF in the input are allowed, but anything that decodes to these   */
/* values is ignored. */

static char char16table[128] = {
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
     0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,
    -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
};

#define char16enc(c)  (((c) & 0x80) ? -1 : char16table[(c)])

static void die_nomem(fatal)
  char *fatal;
{
  strerr_die2x(111,fatal,ERR_NOMEM);
}

void decodeQ(cpfrom,n,outdata,fatal)
char *cpfrom;
unsigned int n;
stralloc *outdata;
char *fatal;
/* does Q decoding of the string pointed to by cpfrom up to the character */
/* before the one pointed to by cpnext, and appends the results to mimeline*/
{
  char *cp,*cpnext,*cpmore;
  char holdch[2];
  char ch1,ch2;		/* need to be signed */

  cpmore = cpfrom;
  cp = cpfrom;
  cpnext = cp + n;
  if (!stralloc_readyplus(outdata,n)) die_nomem(fatal);

  while (cp < cpnext) {
    if (*cp == '_') *cp = ' ';		/* '_' -> space */
    else if (*cp == '=') {		/* "=F8" -> '\xF8' */
					/* copy stuff before */
      if (!stralloc_catb(outdata,cpmore,cp-cpmore)) die_nomem(fatal);
      cpmore = cp;
      ++cp;
      if (*cp == '\n') {		/* skip soft line break */
        ++cp;
        cpmore = cp;
        continue;
      }
      ch1 = char16enc(*cp);
      if (++cp >= cpnext)
        break;
      ch2 = char16enc(*cp);
      if (ch1 >= 0 && ch2 >= 0) {	/* ignore illegals */
        holdch[0] = (ch1 << 4 | ch2) & 0xff;
        if (!stralloc_catb(outdata,holdch,1)) die_nomem(fatal);
        cpmore += 3;
      }
    }
    ++cp;
  }					/* copy stuff after */
  if (!stralloc_catb(outdata,cpmore,cpnext-cpmore)) die_nomem(fatal);
}      


