#include "die.h"
#include "mime.h"

void author_name(stralloc *out,const char *s,unsigned int l)
/* s is a string that contains a from: line argument\n. We parse */
/* s as follows: If there is an unquoted '<' we eliminate everything after */
/* it else if there is a unquoted ';' we eliminate everything after it.    */
/* Then, we eliminate LWSP and '"' from the beginning and end. Note that   */
/* this is not strict rfc822, but all we need is a display handle that     */
/* doesn't show the address. If in the remaining text there is a '@' we put*/
/* in a '.' instead. Also, there are some non-rfc822 from lines out there  */
/* and we still want to maximize the chance of getting a handle, even if it*/
/* costs a little extra work.*/
{
  int squote = 0;
  int dquote = 0;
  int level = 0;
  int flagdone;
  unsigned int len;
  char ch;
  const char *cpfirst,*cp;
  const char *cpcomlast = 0;
  const char *cpquotlast = 0;
  const char *cpquot = 0;
  const char *cpcom = 0;
  const char *cplt = 0;
  char *cpout;

  if (!s || !l)		/* Yuck - pass the buck */
    if (!stralloc_copyb(out,"",0)) die_nomem();
  cp = s; len = l;

  while (len--) {
    ch = *(cp++);
    if (squote)
      squote = 0;
    else if (ch == '\\')
      squote = 1;
    else if (ch == '"') {	/* "name" <address@host> */
      if (dquote) {
	cpquotlast = cp - 2;
        break;
      } else {
	cpquot = cp;
        dquote = 1;
      }
    }
    else if (dquote)
      ;
    else if (ch == '(') {
	if (!level) cpcom = cp;
	level++;
    }
    else if (ch == ')') {
	level--;
	if (!level)
	  cpcomlast = cp - 2;	/* address@host (name) */
    }
    else if (!level) {
      if (ch == '<') {		/* name <address@host> */
	cplt = cp - 2;
	break;
      } else if (ch == ';') break;	/* address@host ;garbage */
    }
  }
  if (cplt) {			/* non-comment '<' */
    cp = cplt;
    cpfirst = s;
  } else if (cpquot && cpquotlast >= cpquot) {
    cpfirst = cpquot;
    cp = cpquotlast;
  } else if (cpcom && cpcomlast >= cpcom) {
    cpfirst = cpcom;
    cp = cpcomlast;
  } else {
    cp = s + l - 1;
    cpfirst = s;
  }
  flagdone = 0;
  for (;;) {		/* e.g. LWSP <user@host> */
    while (cpfirst <= cp &&
	(*cpfirst == ' ' || *cpfirst == '\t' || *cpfirst == '<')) cpfirst++;
    while (cp >= cpfirst &&
	(*cp == ' ' || *cp == '\t' || *cp == '\n' || *cp == '>')) cp--;
    if (cp >= cpfirst || flagdone)
      break;
    cp = s + l - 1;
    cpfirst = s;
    flagdone = 1;
  }

  decodeHDR(cpfirst,cp - cpfirst + 1,out);
  for (cpout = out->s, len = out->len; len > 0; --len, ++cpout)
    if (*cpout == '@')
      *cpout = '.';
}

void author_addr(stralloc *out,const char *s,unsigned int l)
/* s is a string that contains a from: line argument\n. We parse */
/* s as follows: If there is an unquoted '<' we eliminate everything after */
/* it else if there is a unquoted ';' we eliminate everything after it.    */
/* Then, we eliminate LWSP and '"' from the beginning and end. Note that   */
/* this is not strict rfc822, but all we need is a display handle that     */
/* doesn't show the address. If in the remaining text there is a '@' we put*/
/* in a '.' instead. Also, there are some non-rfc822 from lines out there  */
/* and we still want to maximize the chance of getting a handle, even if it*/
/* costs a little extra work.*/
{
  int squote = 0;
  int dquote = 0;
  int level = 0;
  unsigned int len;
  char ch;
  const char *cp;
  const char *cpgt = 0;
  const char *cplt = 0;
  const char *cpat = 0;
  const char *cplast = 0;

  if (!s || !l)		/* Yuck - pass the buck */
    if (!stralloc_copyb(out,"",0)) die_nomem();
  cp = s; len = l;
  cplast = s + len;

  for (cp = s, len = l; len > 0; --len, ++cp) {
    ch = *cp;
    if (squote)
      squote = 0;
    else if (ch == '\\')
      squote = 1;
    else if (ch == '"')		/* "name" <address@host> */
      dquote = !dquote;
    else if (dquote)
      ;
    else if (ch == '(') {
      level++;
    }
    else if (ch == ')') {
      level--;
    }
    else if (!level) {
      if (ch == '<') {		/* name <address@host> */
	cplt = cp;
      }
      else if (ch == '>') {
	cpgt = cp;
	if (cplt)
	  break;
      }
      else if (ch == ';') {	/* address@host ;garbage */
	cplast = cp;
	break;
      }
      else if (ch == '@')
	cpat = cp;
    }
  }
  /* make cplt point to first character of address, cpgt point to one character after end */
  if (cplt && cpgt)
    ++cplt;
  else if (cpat) {
    /* backup strategy, we found an @, grab everything from before and after. */
    for (cplt = cpat - 1; cplt > cp && (*cplt != ' ' && *cplt != '\t'); --cplt)
      ;
    for (cpgt = cpat + 1; cpgt < cplast && (*cpgt != ' ' && *cpgt != '\t'); ++cpgt)
      ;
  }
  else
    cpgt = cplt = cp;		/* Empty result means no address found */
  stralloc_copyb(out,cplt,cpgt-cplt);
}
