#include "hdr.h"
#include "qmail.h"
#include "stralloc.h"
#include "quote.h"
#include "die.h"

extern struct qmail qq;
extern stralloc quoted;
extern stralloc outlocal;
extern stralloc outhost;

void hdr_listsubject3(const char *a,const char *b,const char *c)
{
  if (!quote(&quoted,&outlocal)) die_nomem();
  qmail_puts(&qq,"Subject: ");
  if (a != 0) qmail_puts(&qq,a);
  if (b != 0) qmail_puts(&qq,b);
  if (c != 0) qmail_puts(&qq,c);
  qmail_put(&qq,quoted.s,quoted.len);
  qmail_puts(&qq,"@");
  qmail_put(&qq,outhost.s,outhost.len);
  qmail_puts(&qq,"\n");
}
