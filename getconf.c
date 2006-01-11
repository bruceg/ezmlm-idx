/*$Id$*/

#include "stralloc.h"
#include "byte.h"
#include "slurp.h"
#include "strerr.h"
#include "getconf.h"
#include "altpath.h"
#include "die.h"
#include "scan.h"
#include "config.h"
#include "copy.h"
#include "idx.h"

static stralloc data = {0};
static stralloc xdata = {0};

int getconf(stralloc *sa,const char *fn,int flagrequired,
	    const char *dir)
{
  int i;
  unsigned int j;
  int k;

  if (!stralloc_copys(&data,""))
    die_nomem();
  switch (alt_slurp(fn,&data,128)) {
    case -1:
      strerr_die6sys(111,FATAL,"unable to read ",dir,"/",fn,": ");
    case 0:
      if (!flagrequired)
	return 0;
      strerr_die5x(100,FATAL,dir,"/",fn," does not exist");
  }
  if (!stralloc_append(&data,"\n")) die_nomem();
  copy_xlate(&xdata,&data,'H');
  if (!stralloc_copys(sa,"")) die_nomem();
  i = 0;
  for (j = 0;j < xdata.len;++j)
    if (xdata.s[j] == '\n') {
      k = j;
      while ((k > i) && ((xdata.s[k-1] == ' ') || (xdata.s[k-1] == '\t'))) --k;
      if ((k > i) && (xdata.s[i] != '#')) {
        if (!stralloc_catb(sa,xdata.s + i,k - i)) die_nomem();
        if (!stralloc_0(sa)) die_nomem();
      }
      i = j + 1;
    }
  return 1;
}

int getconf_line(stralloc *sa,const char *fn,int flagrequired,
		 const char *dir)
{
  if (!getconf(sa,fn,flagrequired,dir)) return 0;
  sa->len = byte_chr(sa->s,sa->len,0);
  return 1;
}

int getconf_ulong(unsigned long *u,const char *fn,int flagrequired,
		  const char *dir)
{
  static stralloc line;
  if (!stralloc_copys(&line,"")) die_nomem();
  if (!getconf_line(&line,fn,flagrequired,dir)) return 0;
  if (!stralloc_0(&line)) die_nomem();
  return scan_ulong(line.s,u);
}
