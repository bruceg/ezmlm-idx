#include "hdr.h"
#include "qmail.h"

extern struct qmail qq;
extern char flagcd;

void hdr_transferenc(void)
{
  if (flagcd) {
    qmail_puts(&qq,"Content-Transfer-Encoding: ");
    if (flagcd == 'Q')
      qmail_puts(&qq,"Quoted-Printable\n\n");
    else
      qmail_puts(&qq,"base64\n\n");
  } else
    qmail_puts(&qq,"\n\n");
}
