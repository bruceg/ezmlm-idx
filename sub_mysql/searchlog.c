/*$Id$*/

#include <unistd.h>
#include "getln.h"
#include "case.h"
#include "scan.h"
#include "stralloc.h"
#include "str.h"
#include "open.h"
#include "datetime.h"
#include "date822fmt.h"
#include "substdio.h"
#include "readwrite.h"
#include "strerr.h"
#include "error.h"
#include "errtxt.h"
#include "subscribe.h"
#include "die.h"
#include "idx.h"
#include <mysql.h>

extern MYSQL *mysql;

extern void die_write(void);

static stralloc line = {0};

void searchlog(const char *dir,		/* work directory */
	       const char *subdir,
	       char *search,		/* search string */
	       int subwrite())		/* output fxn */
/* opens dir/Log, and outputs via subwrite(s,len) any line that matches   */
/* search. A '_' is search is a wildcard. Any other non-alphanum/'.' char */
/* is replaced by a '_'. mysql version. Falls back on "manual" search of  */
/* local Log if no mysql connect info. */

{

  unsigned char *cps;
  unsigned char ch;
  unsigned int searchlen;
  const char *ret;

  MYSQL_RES *result;
  MYSQL_ROW row;
  const char *table;
  unsigned long *lengths;

  if (!search) search = (char*)"";	/* defensive */
  searchlen = str_len(search);
  case_lowerb(search,searchlen);
  cps = (unsigned char *) search;
  while ((ch = *(cps++))) {	/* search is potentially hostile */
    if (ch >= 'a' && ch <= 'z') continue;
    if (ch >= '0' && ch <= '9') continue;
    if (ch == '.' || ch == '_') continue;
    *(cps - 1) = '_';		/* will match char specified as well */
  }

  if ((ret = opensub(dir,subdir,&table))) {
    if (*ret) strerr_die2x(111,FATAL,ret);

    std_searchlog(dir,subdir,search,subwrite);

  } else {

/* SELECT (*) FROM list_slog WHERE fromline LIKE '%search%' OR address   */
/* LIKE '%search%' ORDER BY tai; */
/* The '*' is formatted to look like the output of the non-mysql version */
/* This requires reading the entire table, since search fields are not   */
/* indexed, but this is a rare query and time is not of the essence.     */

    if (!stralloc_cats(&line,"SELECT CONCAT(FROM_UNIXTIME(UNIX_TIMESTAMP(tai)),"
	"'-0000: ',UNIX_TIMESTAMP(tai),' ',edir,etype,' ',address,' ',"
	"fromline) FROM ")) die_nomem();
    if (!stralloc_cats(&line,table)) die_nomem();
    if (!stralloc_cats(&line,"_slog ")) die_nomem();
    if (*search) {	/* We can afford to wait for LIKE '%xx%' */
      if (!stralloc_cats(&line,"WHERE fromline LIKE '%")) die_nomem();
      if (!stralloc_cats(&line,search)) die_nomem();
      if (!stralloc_cats(&line,"%' OR address LIKE '%")) die_nomem();
      if (!stralloc_cats(&line,search)) die_nomem();
      if (!stralloc_cats(&line,"%'")) die_nomem();
    }	/* ordering by tai which is an index */
      if (!stralloc_cats(&line," ORDER by tai")) die_nomem();

    if (mysql_real_query(mysql,line.s,line.len))	/* query */
	strerr_die2x(111,FATAL,mysql_error(mysql));
    if (!(result = mysql_use_result(mysql)))
	strerr_die2x(111,FATAL,mysql_error(mysql));
    while ((row = mysql_fetch_row(result))) {
    if (!(lengths = mysql_fetch_lengths(result)))
	strerr_die2x(111,FATAL,mysql_error(mysql));
      if (subwrite(row[0],lengths[0]) == -1) die_write();
    }
    if (!mysql_eof(result))
	strerr_die2x(111,FATAL,mysql_error(mysql));
    mysql_free_result(result);
  }
}
