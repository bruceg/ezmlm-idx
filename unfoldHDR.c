#include "stralloc.h"
#include "strerr.h"
#include "case.h"
#include "byte.h"
#include "errtxt.h"
#include "mime.h"
#include "die.h"
#include "idx.h"

static stralloc tmpdata = {0};

static int trimre(char **cpp,char *cpend,stralloc *prefix)
{
  int r = 0;
  char *cp;
  char *cpnew;
  int junk;
  unsigned int i,j;
  unsigned int serial;

  cp = *cpp;
  serial = prefix->len;		/* pointer to serial number */
  if (serial)
    serial = byte_rchr(prefix->s,prefix->len,'#');

  junk = 1;
  while (junk) {
    junk = 0;
    while (cp <= cpend && (*cp == ' ' || *cp == '\t')) cp++;
    cpnew = cp;
    while (++cpnew <= cpend) {	/* /(..+:\s)/ is a reply indicator */
      if (*cpnew == ' ') {
        if (cpnew < cp + 3) break;	/* at least 3 char before ' ' */
	if (*(cpnew - 1) != ':') break;	/* require ':' before ' ' */
	if (cpnew > cp + 5) {		/* if > 4 char before ':' require */
	  char ch;
	  ch = *(cpnew - 2);		/* XX^3, XX[3], XX(3) */
	  if (ch != ')' && ch != ']' && (ch < '0' || ch > '9'))
	    break;
	}
	junk = 1;
	r |= 1;
	cp = cpnew + 1;
        break;
      }
    }
	/* prefix removal is complicated by the inconsistent handling of ' ' */
	/* when there are rfc2047-encoded words in the subject. We first     */
	/* compare prefix before "serial" ignoring space, then skip the      */
	/* number, then compare after "serial". If both matched we've found  */
	/* the prefix. */
    if (serial) {
      cpnew = cp;
      i = 0;
      while (i < serial && cpnew <= cpend) {
        if (*cpnew != ' ') {
          if (prefix->s[i] == ' ') {
            ++i;
            continue;
          }
          if (*cpnew != prefix->s[i]) break;
          ++i;
        }
        ++cpnew;
      }
      if (i == serial) {		/* match before serial */
        j = prefix->len;
        if (serial != j) {		/* got a '#' */
	  /* skip number/space */
          while (cpnew <= cpend
		 && (*cpnew == ' ' || (*cpnew <= '9' && *cpnew >= '0')))
	    ++cpnew;
          i = serial + 1;
          while (i < j && cpnew <= cpend) {
            if (*cpnew != ' ') {
              if (prefix->s[i] == ' ') {
                ++i;
                continue;
              }
              if (*cpnew != prefix->s[i]) break;
              ++i;
            }
            ++cpnew;
          }
        }
        if (i == j) {
          cp = cpnew;
          junk = 1;
          r |= 2;
        }
      }
    }
  }
  *cpp = cp;
  return r;
}

static int trimend(char *indata,unsigned int *np)
	/* looks at indata of length n from the end removing LWSP & '\n' */
	/* and any trailing '-Reply'. Sets n to new length and returns:  */
	/* 0 - not reply, 1 - reply. */
{
  char *cplast;
  int junk;
  int r = 0;

  if (*np == 0) return 0;
  cplast = indata + *np - 1;	/* points to last char on line */
  junk = 1;
  while (junk) {
    junk = 0;
    while (cplast >= indata &&
             (*cplast == ' ' || *cplast == '\t' ||
              *cplast == '\r' || *cplast == '\n')) 
            --cplast;
    if (cplast - indata  >= 5 && case_startb(cplast - 5,6,"-Reply")) {
      cplast -= 6;
      r = 1;
      junk = 1;
    }
  }
  *np = (unsigned int) (cplast - indata + 1);	/* new length */
  return r;
}

int unfoldHDR(char *indata,
	      unsigned int n,
	      stralloc *outdata,
	      const char *charset,
	      stralloc *prefix,
	      int flagtrimsub)
	/* takes a header as indata. Removal of reply-indicators is done */
	/* but removal of line breaks and Q and B decoding should have   */
	/* been done. Returns a */
	/* single line header without trailing \n or \0. Mainly, we      */
	/* remove redundant shift codes   */
	/* returns 0 = no reply no prefix */
	/*         1 = reply no prefix    */
	/*         2 = no reply, prefix   */
	/*         3 = reply & pefix      */
{
  int r = 0;
  char *cp,*cpesc,*cpnext,*cpend,*cpout;
  char state,cset,newcset;
  int reg,newreg;

  cp = indata;		/* JIS X 0201 -> ISO646 us-ascii */
  cpend = cp + n - 1;
  cpnext = cp;
  if (!stralloc_copys(&tmpdata,"")) die_nomem();
  if (!stralloc_ready(&tmpdata,n)) die_nomem();

  if(!case_diffb(charset,11,"iso-2022-jp")) {
	/* iso-2022-jp-2 (rfc1554) and its subset iso-2022-jp. The reg #s */
	/* are from the rfc. Don't ask why they have multiple length G0   */
	/* charset designations ... JIS X 0201-roman is identical to      */
	/* iso646 us-ascii except for currency and tilde. Making them the */
	/* same increases hits without significant loss. JIS X 0208-1978  */
	/* is superceded by JIS X 0208-1983 and converted here as well.   */

    while (cp < cpend) {
      if (*cp++ != ESC) continue;
      if (*cp == '(') {
        if (++cp > cpend) break;
        if (*cp == 'J') *cp = 'B';
        ++cp;
      } else if (*cp == '$') {
        if (++cp > cpend) break;
        if (*cp == '@') *cp = 'B';
        ++cp;
      }
    }
		/* eliminate redundant ESC seqs */
    cp = indata;
    cpnext = cp;
    reg = 6;
    while (cp < cpend) {
      if (*cp++ != ESC) continue;
      cpesc = cp - 1;
      if (*cp == '$') {
        if (++cp > cpend) break;
        if (*cp == 'B') newreg = 87;
        else if (*cp == 'A') newreg = 58;
        else if (*cp == '(') {
          if (++cp > cpend) break;
          if (*cp == 'C') newreg = 149;
          else if (*cp == 'D') newreg = 159;
          else continue;
        } else continue;
      } else if (*cp == '(') {
        if (++cp > cpend) break;
        if (*cp == 'B') newreg = 6;
        else continue;
      } else continue;
      if (++cp > cpend) break;
      while (*cp == ' ' || *cp == '\t')
        if (++cp >= cpend) break;	/* skip space */
      if (*cp == ESC)			/* maybe another G0 designation */
        if (*(cp+1) == '(' || *(cp+1) == '$') {	 /* yep! */
          if (!stralloc_catb(&tmpdata,cpnext,cpesc-cpnext)) die_nomem();
          cpnext = cp;
	  continue;
      }
      if (reg == newreg) {
        if (!stralloc_catb(&tmpdata,cpnext,cpesc-cpnext)) die_nomem();
        cpnext = cp;
      } else {
        reg = newreg;
      }		/* copy remainder of line */
    }
    if (!stralloc_catb(&tmpdata,cpnext,cpend - cpnext + 1)) die_nomem();
    if (reg != 6) {	/* need to return to us-ascii at the end of the line */
      if (!stralloc_cats(&tmpdata,TOASCII)) die_nomem();
    } else {		/* maybe "-Reply at the end?" */
      r = trimend(tmpdata.s,&(tmpdata.len));
    }

  } else if (!case_diffb(charset,11,"iso-2022-cn") ||
             !case_diffb(charset,11,"iso-2022-kr")) {
	/* these use SI/SO and ESC $ ) x as the SO designation. In -cn and */
	/* -cn-ext, 'x' can be a number of different letters. In -kr it's  */
	/* always 'C'. This routine may work also for other iso-2022 sets  */
	/* also handles iso-2022-cn-ext */
    cpesc = (char *) 0;	/* points to latest ESC */
    state = SI;		/* us-ascii */
    --cp;		/* set up for loop */

    while (++cp <= cpend) {
      if (*cp == SI || *cp == SO) {
        if (state == *cp) {		 /* already in state. Skip shift seq */
          if (!stralloc_catb(&tmpdata,cpnext,cp-cpnext-1)) die_nomem();
          cpnext = cp;
        } else				/* set new state */
          state = *cp;
        if (++cp > cpend) break;
        continue;
      }
      if (*cp != ESC) continue;
      if (cp + 3 > cpend) break;	/* not space for full SO-designation */
      cpesc = cp;
      if (*cp != '$') continue;
      if (++cp > cpend) break;
      if (*cp != ')') continue;
      if (++cp > cpend) break;
      newcset = *cp;
      if (++cp > cpend) break;
      while (cp <= cpend && (*cp == ' ' || *cp == '\t')) ++cp;
      if (cp + 3 > cpend) break;	/* no space for full SO-designation */
      if ((*cp == ESC && *(cp+1) == '$' && *(cp+2) == ')')
		|| (newcset == cset)) {
			/* skip if a second SO-designation right after or */
			/* this SO-designation is already active, skip */
        if (!stralloc_catb(&tmpdata,cpnext,cpesc-cpnext)) die_nomem();
        --cp;		/* "unpeek" so that next iteration will see char */
        cpnext = cpesc + 4;
        continue;
      } else {
        cset = newcset;
        continue;
      }
    }
			/* get remainder of line */
    if (!stralloc_catb(&tmpdata,cpnext,cpend - cpnext + 1)) die_nomem();
    if (state != SI) {	/* need to end in ascii */
      if (!stralloc_cats(&tmpdata,TOSI)) die_nomem();
    }
    else		/* ascii end; maybe "-Reply" at the end? */
      r = trimend(tmpdata.s,&(tmpdata.len));

  } else {		/* other character sets = no special treatment */
    r = trimend(cp,&n);		/* -reply */
    if (!stralloc_copyb(&tmpdata,cp,n)) die_nomem();
  }

  cp = tmpdata.s;
  n = tmpdata.len;
  cpend = cp + n - 1;
  if (flagtrimsub) {	 /* remove leading reply indicators & prefix*/
    r |= trimre(&cp,cpend,prefix);
    n = (unsigned int) (cpend-cp+1);
  }
			/* there shouldn't be '\0' or '\n', but make sure as */
			/* it would break the message index */
  if (!stralloc_copys(outdata,"")) die_nomem();
  if (!stralloc_ready(outdata,n)) die_nomem();
  outdata->len = n;
  cpout = outdata->s;
  while (n--) {		/* '\n' and '\0' would break the subject index */
    if (!*cp || *cp == '\n') *cpout = ' ';
    else *cpout = *cp;
    ++cp; ++cpout;
  }
  return r;
}

