/*$Id: author.c,v 1.3 1999/09/29 03:11:44 lindberg Exp $*/
/*$Name:*/

unsigned int author_name(char **sout,char *s,unsigned int l)
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
  char *cpfirst,*cp;
  char *cpcomlast = 0;
  char *cpquotlast = 0;
  char *cpquot = 0;
  char *cpcom = 0;
  char *cplt = 0;

  if (!s || !l) {	/* Yuck - pass the buck */
    *sout = s;
    return 0;
  }
  cp = s; len = l;

  while (len--) {
    ch = *(cp++);
    if (squote) {
      squote = 0;
      continue;
    }
    if (ch == '\\') {
      squote = 1;
      continue;
    }
    if (ch == '"') {		/* "name" <address@host> */
      if (dquote) {
	cpquotlast = cp - 2;
        break;
      } else {
	cpquot = cp;
        dquote = 1;
      }
      continue;
    } else if (dquote) continue;
    if (ch == '(') {
	if (!level) cpcom = cp;
	level++;
    } else if (ch == ')') {
	level--;
	if (!level)
	  cpcomlast = cp - 2;	/* address@host (name) */
    } else if (!level) {
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

  *sout = cpfirst;
  len = cp - cpfirst + 1;
  while (cpfirst <= cp) {
    if (*cpfirst == '@')
      *cpfirst = '.';
    cpfirst++;
  }
  return len;
}

