/*$Id: logmsg.c,v 1.3 1999/10/12 23:38:36 lindberg Exp $*/
#include "stralloc.h"
#include "fmt.h"
#include "subscribe.h"
#include "errtxt.h"

static stralloc logline = {0};
static strnum[FMT_ULONG];

char *logmsg(dir,num,listno,subs,done)
char *dir;
unsigned long num;
unsigned long listno;
unsigned long subs;
int done;
{
      return (char *) 0;	/* no SQL => success */
}
