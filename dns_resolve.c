/* Public domain, from djbdns-1.05. */

#include "sys/iopause.h"
#include "taia.h"
#include "byte.h"
#include "dns.h"

struct dns_transmit dns_resolve_tx = {0};

int dns_resolve(const char *q,const char qtype[2])
{
  struct taia stamp;
  struct taia deadline;
  char servers[64];
  iopause_fd x[1];
  int r;

  if (dns_resolvconfip(servers) == -1) return -1;
  if (dns_transmit_start(&dns_resolve_tx,servers,1,q,qtype,"\0\0\0\0") == -1) return -1;

  for (;;) {
    taia_now(&stamp);
    taia_uint(&deadline,120);
    taia_add(&deadline,&deadline,&stamp);
    dns_transmit_io(&dns_resolve_tx,x,&deadline);
    iopause(x,1,&deadline,&stamp);
    r = dns_transmit_get(&dns_resolve_tx,x,&stamp);
    if (r == -1) return -1;
    if (r == 1) return 0;
  }
}
