#include "stralloc.h"
#include "strerr.h"
#include "error.h"
#include "case.h"
#include "byte.h"
#include "uint32.h"
#include "mime.h"
#include "errtxt.h"
#include "die.h"
#include "idx.h"

void decodeHDR(const char *indata,
	       unsigned int n,
	       stralloc *outdata)
/* decodes indata depending on charset. May put '\n' and '\0' into out */
/* data and can take them as indata. */
{
  unsigned int pos;
  const char *cp,*cpnext,*cpstart,*cpenc,*cptxt,*cpend,*cpafter;

  cpnext = indata;
  cpafter = cpnext + n;
  cpstart = cpnext;
  if (!stralloc_copys(outdata,"")) die_nomem();
  if (!stralloc_ready(outdata,n)) die_nomem();
  for (;;) {
    cpstart = cpstart + byte_chr(cpstart,cpafter-cpstart,'=');
    if (cpstart == cpafter)
      break;
    ++cpstart;
    if (*cpstart != '?')
      continue;
    ++cpstart;
    cpenc = cpstart + byte_chr(cpstart,cpafter-cpstart,'?');
    if (cpenc == cpafter)
      continue;
    cpenc++;
    cptxt = cpenc + byte_chr(cpenc,cpafter-cpenc,'?');
    if (cptxt == cpafter)
      continue;
    cptxt++;
    cpend = cptxt + byte_chr(cptxt,cpafter-cptxt,'?');
    if (cpend == cpafter || *(cpend + 1) != '=')
      continue;
	/* We'll decode anything. On lists with many charsets, this may */
	/* result in unreadable subjects, but that's the case even if   */
	/* no decoding is done. This way, the subject will be optimal   */
	/* for threading, but charset info is lost. We aim to correctly */
	/* decode us-ascii and all iso-8859/2022 charsets. Exacly how   */
	/* these will be displayed depends on dir/charset.              */
    cp = cpnext;
    			/* scrap lwsp between coded strings */
    while (*cp == ' ' || *cp == '\t')
      cp++;
    if (cp != cpstart - 2)
      if (!stralloc_catb(outdata,cpnext, cpstart - cpnext - 2))
		die_nomem();
   cpnext = cp + 1;
   cpstart = cpnext;
          switch (*cpenc) {
            case 'b':
            case 'B':
              pos = outdata->len;
              decodeB(cptxt,cpend-cptxt,outdata);
              cpnext = cpend + 2;
              cpstart = cpnext;
              break;
            case 'q':
            case 'Q':
              decodeQ(cptxt,cpend-cptxt,outdata);
              cpnext = cpend + 2;
              cpstart = cpnext;
              break;
            default:		/* shouldn't happen, but let's be reasonable */
              cpstart = cpend + 2;
              break;
          }
  }
  if (!stralloc_catb(outdata,cpnext,indata-cpnext+n)) die_nomem();
}

