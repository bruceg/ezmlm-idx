/*$Id$*/
#include "hdr.h"
#include "qmail.h"
#include "makehash.h"
#include "stralloc.h"

extern struct qmail qq;
extern char flagcd;
extern char boundary[HASHLEN];
extern stralloc charset;

void hdr_ctboundary(void)
{
  if (flagcd) {
    qmail_puts(&qq,"\n--");
    qmail_put(&qq,boundary,HASHLEN);
    qmail_puts(&qq,"\n");
    hdr_ctype("text/plain");
    hdr_transferenc();
  } else
    qmail_puts(&qq,"\n");
}
