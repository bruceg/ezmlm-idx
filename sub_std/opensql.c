/*$Id$*/
#include "stralloc.h"
#include "strerr.h"
#include "errtxt.h"
#include "subscribe.h"

static stralloc myp = {0};
static stralloc ers = {0};
static stralloc fn = {0};
static stralloc ourdb = {0};
static char *ourtable = (char *) 0;

char *opensql(dbname,table)
char *dbname;	/* database directory */
char **table;	/* table root_name */

{
	return 0;
}

void closesql()
/* close connection to SQL server, if open */
{
	return;
}

