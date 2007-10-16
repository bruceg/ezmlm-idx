/*$Id$*/

#include "hdr.h"
#include "qmail.h"

extern struct qmail qq;

void hdr_subject(const char *subject)
{
  qmail_puts(&qq,"Subject: ");
  qmail_puts(&qq,subject);
  qmail_puts(&qq,"\n");
}
