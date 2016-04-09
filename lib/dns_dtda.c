/* Public domain, from djbdns-1.05. */

#include "stralloc.h"
#include "dns.h"

void dns_domain_todot_cat(stralloc *out,const char *d)
{
  char ch;
  char ch2;
  unsigned char ch3;
  char buf[4];

  if (!*d) {
    stralloc_append(out,'.');
    return;
  }

  for (;;) {
    ch = *d++;
    while (ch--) {
      ch2 = *d++;
      if ((ch2 >= 'A') && (ch2 <= 'Z'))
	ch2 += 32;
      if (((ch2 >= 'a') && (ch2 <= 'z')) || ((ch2 >= '0') && (ch2 <= '9')) || (ch2 == '-') || (ch2 == '_')) {
        stralloc_append(out,ch2);
      }
      else {
	ch3 = ch2;
	buf[3] = '0' + (ch3 & 7); ch3 >>= 3;
	buf[2] = '0' + (ch3 & 7); ch3 >>= 3;
	buf[1] = '0' + (ch3 & 7);
	buf[0] = '\\';
	stralloc_catb(out,buf,4);
      }
    }
    if (!*d) break;
    stralloc_append(out,'.');
  }
}
