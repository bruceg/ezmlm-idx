/*$Id$*/

#include <unistd.h>
#include "hdr.h"
#include "qmail.h"
#include "stralloc.h"
#include "datetime.h"
#include "fmt.h"
#include "date822fmt.h"
#include "makehash.h"

extern void die_nomem(void);
extern struct qmail qq;
extern char flagcd;
extern char boundary[HASHLEN];
extern stralloc line;
extern char strnum[FMT_ULONG];
extern stralloc outhost;

void hdr_datemsgid(unsigned long when)
{
  char date[DATE822FMT];
  struct datetime dt;
  qmail_puts(&qq,"Date: ");
  datetime_tai(&dt,when);
  qmail_put(&qq,date,date822fmt(date,&dt));
  qmail_puts(&qq,"Message-ID: <");
  if (!stralloc_copyb(&line,strnum,fmt_ulong(strnum,(unsigned long)when)))
    die_nomem();
  if (!stralloc_append(&line,".")) die_nomem();
  if (!stralloc_catb(&line,strnum,
		     fmt_ulong(strnum,(unsigned long) getpid()))) die_nomem();
  if (!stralloc_cats(&line,".ezmlm@")) die_nomem();
  if (!stralloc_cats(&line,outhost.s)) die_nomem();
  if (!stralloc_0(&line)) die_nomem();
  qmail_puts(&qq,line.s);
  qmail_puts(&qq,">\n");
  /* "unique" MIME boundary as hash of messageid */
  makehash(line.s,line.len,boundary);
}
