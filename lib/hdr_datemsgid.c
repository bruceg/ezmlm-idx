#include <unistd.h>
#include "hdr.h"
#include "qmail.h"
#include "stralloc.h"
#include "datetime.h"
#include "fmt.h"
#include "date822fmt.h"
#include "makehash.h"
#include "die.h"
#include "idx.h"

extern struct qmail qq;
extern char flagcd;
extern char boundary[HASHLEN];
extern stralloc line;
extern stralloc outhost;

void hdr_datemsgid(unsigned long when)
{
  char date[DATE822FMT];
  char strnum[FMT_ULONG];
  struct datetime dt;
  qmail_puts(&qq,"Date: ");
  datetime_tai(&dt,when);
  qmail_put(&qq,date,date822fmt(date,&dt));
  qmail_puts(&qq,"Message-ID: <");
  stralloc_copyb(&line,strnum,fmt_ulong(strnum,(unsigned long)when));
  stralloc_append(&line,'.');
  stralloc_catb(&line,strnum,fmt_ulong(strnum,(unsigned long) getpid()));
  stralloc_cats(&line,".ezmlm@");
  stralloc_cats(&line,outhost.s);
  stralloc_0(&line);
  qmail_puts(&qq,line.s);
  qmail_puts(&qq,">\n");
  /* "unique" MIME boundary as hash of messageid */
  makehash(line.s,line.len,boundary);
}
