/* Public domain, from djbdns-1.05. */

#include <sys/types.h>
#include <sys/time.h>
#include "taia.h"

void taia_now(struct taia *t)
{
  struct timeval now;
  gettimeofday(&now,(struct timezone *) 0);
  tai_unix(&t->sec,now.tv_sec);
  t->nano = 1000 * now.tv_usec + 500;
  t->atto = 0;
}
