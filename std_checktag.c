/*$Id$*/

#include "stralloc.h"
#include "scan.h"
#include "fmt.h"
#include "cookie.h"
#include "makehash.h"
#include "strerr.h"
#include "slurp.h"
#include "byte.h"
#include "errtxt.h"
#include "subscribe.h"

static stralloc key = {0};
static char strnum[FMT_ULONG];
static char newcookie[COOKIE];

const char *std_checktag(unsigned long num,	/* message number */
			 const char *action,
			 const char *seed,	/* cookie base */
			 const char *hash)	/* cookie */
/* Checks that the hash of seed matches hash and returns success
 * (NULL). If not match returns "". If error, returns error string */
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
