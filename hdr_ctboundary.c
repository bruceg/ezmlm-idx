/*$Id$*/
#include "hdr.h"
#include "qmail.h"

extern struct qmail qq;
extern char flagcd;

void hdr_ctboundary(void)
{
  if (flagcd) {
    hdr_boundary(0);
    hdr_ctype(CTYPE_TEXT);
    hdr_transferenc();
  } else
    qmail_puts(&qq,"\n");
}
