/*$Id$*/

#include "str.h"
#include "slurp.h"
#include "scan.h"
#include "stralloc.h"
#include "strerr.h"
#include "errtxt.h"
#include "subscribe.h"

static stralloc myp = {0};
static stralloc ers = {0};
static stralloc fn = {0};
static stralloc ourdb = {0};
static const char *ourtable = (char *) 0;

const char *parsesql(const char *dbname,	/* database directory */
		     const char **table,	/* table root_name */
		     struct sqlinfo *info)
/* reads the file dbname/sql, and if the file exists, parses it into the    */
/* components. The string should be host:port:user:pw:db:table.         If  */
/* the file does not exists, returns "". On success returns NULL. On error  */
/* returns error string for temporary error. If *table is not null, it      */
/* overrides the table in the sql file. If we already opended dbname the    */
/* cached info is used, rather than rereading the file.                     */
/* Note that myp is static and all pointers point to it.*/
{
  unsigned int j;

  info->db = "ezmlm";

  if (!stralloc_copys(&fn,dbname)) return ERR_NOMEM;
  if (fn.len == ourdb.len && !str_diffn(ourdb.s,fn.s,fn.len)) {
    *table = ourtable;
    return 0;
  }
  if (!stralloc_cats(&fn,"/sql")) return ERR_NOMEM;
  if (!stralloc_0(&fn)) return ERR_NOMEM;
		/* host:port:db:table:user:pw:name */

  myp.len = 0;
  switch (slurp(fn.s,&myp,128)) {
	case -1:	if (!stralloc_copys(&ers,ERR_READ)) return ERR_NOMEM;
			if (!stralloc_cat(&ers,&fn)) return ERR_NOMEM;
			if (!stralloc_0(&ers)) return ERR_NOMEM;
			return ers.s;
	case 0: return "";
  }
  if (!stralloc_copy(&ourdb,&fn)) return ERR_NOMEM;
  if (!stralloc_append(&myp,"\n")) return ERR_NOMEM;
  if (myp.s[j = str_chr(myp.s,'\n')])
    myp.s[j] = '\0';
						/* get connection parameters */
  if (!stralloc_0(&myp)) return ERR_NOMEM;
  info->host = myp.s;
  if (myp.s[j = str_chr(myp.s,':')]) {
    myp.s[j++] = '\0';
    info->port = myp.s + j;
    if (myp.s[j += str_chr(myp.s+j,':')]) {
      myp.s[j++] = '\0';
      info->user = myp.s + j;
      if (myp.s[j += str_chr(myp.s+j,':')]) {
	myp.s[j++] = '\0';
        info->pw = myp.s + j;
	if (myp.s[j += str_chr(myp.s+j,':')]) {
	  myp.s[j++] = '\0';
          info->db = myp.s + j;
	  if (myp.s[j += str_chr(myp.s+j,':')]) {
	    myp.s[j++] = '\0';
	    ourtable = myp.s + j;
	  }
	}
      }
    }
  }
  if (info->host && !*info->host)
    info->host = (char *) 0;
  if (info->user && !*info->user)
    info->user = (char *) 0;
  if (info->pw && !*info->pw)
    info->pw = (char *) 0;
  if (info->db && !*info->db)
    info->db = (char *) 0;
  if (!ourtable || !*ourtable)
    return ERR_NO_TABLE;
  *table = ourtable;
  return (char *) 0;
}
