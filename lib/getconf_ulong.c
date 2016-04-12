#include "stralloc.h"
#include "getconf.h"
#include "die.h"
#include "scan.h"

static stralloc line;

int getconf_ulong(unsigned long *u,const char *fn,int flagrequired)
{
  stralloc_copys(&line,"");
  if (!getconf_line(&line,fn,flagrequired)) return 0;
  stralloc_0(&line);
  return scan_ulong(line.s,u);
}

int getconf_ulong2(unsigned long *n0,unsigned long *n1,
		   const char *fn,int flagrequired)
{
  unsigned int colon;
  stralloc_copys(&line,"");
  if (!getconf_line(&line,fn,flagrequired)) return 0;
  stralloc_0(&line);
  if ((colon = scan_ulong(line.s,n0)) != 0
      && n1 != 0
      && line.s[colon++] == ':')
    colon += scan_ulong(line.s+colon,n1);
  return colon;
}
