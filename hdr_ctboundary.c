/*$Id:$*/
#include "hdr.h"
#include "qmail.h"
#include "cookie.h"
#include "stralloc.h"

extern struct qmail qq;
extern char flagcd;
extern char boundary[COOKIE];
extern stralloc charset;

void hdr_ctboundary(void)
{
  if (flagcd) {
    qmail_puts(&qq,"\n--");
    qmail_put(&qq,boundary,COOKIE);
    qmail_puts(&qq,"\nContent-Type: text/plain; charset=");
    qmail_puts(&qq,charset.s);
    hdr_transferenc();
  } else
    qmail_puts(&qq,"\n");
}
