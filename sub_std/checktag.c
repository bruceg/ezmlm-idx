/*$Id: checktag.c,v 1.4 1999/10/12 23:38:36 lindberg Exp $*/
#include "stralloc.h"
#include "scan.h"
#include "fmt.h"
#include "cookie.h"
#include "makehash.h"
#include "strerr.h"
#include "errtxt.h"
#include "subscribe.h"

static stralloc key = {0};
static stralloc line = {0};
static stralloc quoted = {0};
static char strnum[FMT_ULONG];
static char newcookie[COOKIE];

char *checktag (dir,num,listno,action,seed,hash)
/* reads dir/sql. If not present, checks that the hash of seed matches */
/* hash and returns success (NULL). If not match returns "". If error, */
/* returns error string */

char *dir;				/* the db base dir */
unsigned long num;			/* message number */
unsigned long listno;			/* bottom of range => slave */
char *action;
char *seed;				/* cookie base */
char *hash;				/* cookie */
{

    if (!seed) return (char *) 0;		/* no data - accept */

    strnum[fmt_ulong(strnum,num)] = '\0';	/* message nr ->string*/

    switch(slurp("key",&key,32)) {
      case -1:
	return ERR_READ_KEY;
      case 0:
	return ERR_NOEXIST_KEY;
    }

    cookie(newcookie,key.s,key.len,strnum,seed,action);
    if (byte_diff(hash,COOKIE,newcookie)) return "";
    else return (char *) 0;

}
