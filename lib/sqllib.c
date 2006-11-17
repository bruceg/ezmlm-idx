/*$Id$*/

#include "str.h"
#include "slurp.h"
#include "scan.h"
#include "stralloc.h"
#include "strerr.h"
#include "errtxt.h"
#include "sub_std.h"
#include "sqllib.h"

static stralloc myp = {0};
static stralloc ers = {0};
static stralloc fn = {0};

const char *parsesql(const char *dir,
		     const char *subdir,
		     struct sqlinfo *info)
/* reads the file dbname/sql, and if the file exists, parses it into the    */
/* components. The string should be host:port:user:pw:db:table.         If  */
/* the file does not exists, returns "". On success returns NULL. On error  */
/* returns error string for temporary error. */
/* Note that myp is static and all pointers point to it.*/
{
  unsigned int j;
  const char *port;

  info->db = "ezmlm";
  info->host = info->user = info->pw = info->table = 0;
  info->port = 0;

  std_makepath(&fn,dir,subdir,"/sql",0);

		/* host:port:db:table:user:pw:name */
  myp.len = 0;
  port = 0;
  switch (slurp(fn.s,&myp,128)) {
	case -1:	if (!stralloc_copys(&ers,ERR_READ)) return ERR_NOMEM;
			if (!stralloc_cat(&ers,&fn)) return ERR_NOMEM;
			if (!stralloc_0(&ers)) return ERR_NOMEM;
			return ers.s;
	case 0: return "";
  }
  if (!stralloc_append(&myp,"\n")) return ERR_NOMEM;
  if (myp.s[j = str_chr(myp.s,'\n')])
    myp.s[j] = '\0';
						/* get connection parameters */
  if (!stralloc_0(&myp)) return ERR_NOMEM;
  info->host = myp.s;
  if (myp.s[j = str_chr(myp.s,':')]) {
    myp.s[j++] = '\0';
    port = myp.s + j;
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
	    info->table = myp.s + j;
	  }
	}
      }
    }
  }
  if (port && *port)
    scan_ulong(port,&info->port);
  if (info->host && !*info->host)
    info->host = (char *) 0;
  if (info->user && !*info->user)
    info->user = (char *) 0;
  if (info->pw && !*info->pw)
    info->pw = (char *) 0;
  if (info->db && !*info->db)
    info->db = (char *) 0;
  if (!info->table || !*info->table)
    return ERR_NO_TABLE;
  return (char *) 0;
}
