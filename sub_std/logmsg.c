/*$Id$*/
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
