/*$Id$*/

#include "hdr.h"
#include "qmail.h"
#include "makehash.h"
#include "stralloc.h"
#include "str.h"

extern struct qmail qq;
extern char boundary[HASHLEN];
extern stralloc charset;

void hdr_ctype(const char *ctype)
{

  qmail_puts(&qq,"Content-Type: ");
  qmail_puts(&qq,ctype);
  if (str_diffn(ctype, "text/", 5) == 0
      && charset.s != 0
      && charset.len > 0) {
    /* text/something, needs a charset */
    qmail_puts(&qq,"; charset=");
    qmail_puts(&qq,charset.s);
  }
  if (str_diffn(ctype, "multipart/", 10) == 0) {
    /* multipart/something, needs a boundary */
    qmail_puts(&qq,"; boundary=");
    qmail_put(&qq,boundary,HASHLEN);
  }
  qmail_puts(&qq,"\n");
}

void hdr_mime(const char *ctype)
{
  qmail_puts(&qq, "MIME-Version: 1.0\n");
  hdr_ctype(ctype);
}
