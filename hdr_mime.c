/*$Id$*/

#include "hdr.h"
#include "qmail.h"
#include "cookie.h"
#include "stralloc.h"

extern struct qmail qq;
extern char boundary[COOKIE];
extern stralloc charset;

void hdr_ctype(const char *ctype)
{

  qmail_puts(&qq,"Content-Type: ");
  qmail_puts(&qq,ctype);
  if (charset.s && charset.len > 0) {
    qmail_puts(&qq,"; charset=");
    qmail_puts(&qq,charset.s);
  }
  if (memcmp(ctype, "multipart/", 10) == 0) {
    /* multipart/something */
    qmail_puts(&qq,";\n\tboundary=");
    qmail_put(&qq,boundary,COOKIE);
  }
  qmail_puts(&qq,"\n");
}

void hdr_mime(const char *ctype)
{
  qmail_puts(&qq, "MIME-Version: 1.0\n");
  hdr_ctype(ctype);
}
