/* Public domain, from djbdns-1.05. */

#include <unistd.h>
#include "taia.h"
#include "env.h"
#include "byte.h"
#include "str.h"
#include "openreadclose.h"
#include "dns.h"

static stralloc data = {0};

static int init(stralloc *rules)
{
  char host[256];
  const char *x;
  unsigned int i;
  unsigned int j;
  int k;

  stralloc_copys(rules,"");

  x = env_get("DNSREWRITEFILE");
  if (!x) x = "/etc/dnsrewrite";

  k = openreadclose(x,&data,64);
  if (k == -1) return -1;

  if (k) {
    stralloc_append(&data,'\n');
    i = 0;
    for (j = 0;j < data.len;++j)
      if (data.s[j] == '\n') {
        stralloc_catb(rules,data.s + i,j - i);
        while (rules->len) {
          if (rules->s[rules->len - 1] != ' ')
          if (rules->s[rules->len - 1] != '\t')
          if (rules->s[rules->len - 1] != '\r')
            break;
          --rules->len;
        }
        stralloc_0(rules);
        i = j + 1;
      }
    return 0;
  }

  x = env_get("LOCALDOMAIN");
  if (x) {
    stralloc_copys(&data,x);
    stralloc_append(&data,' ');
    stralloc_copys(rules,"?:");
    i = 0;
    for (j = 0;j < data.len;++j)
      if (data.s[j] == ' ') {
        stralloc_cats(rules,"+.");
        stralloc_catb(rules,data.s + i,j - i);
        i = j + 1;
      }
    stralloc_0(rules);
    stralloc_cats(rules,"*.:");
    stralloc_0(rules);
    return 0;
  }

  k = openreadclose("/etc/resolv.conf",&data,64);
  if (k == -1) return -1;

  if (k) {
    stralloc_append(&data,'\n');
    i = 0;
    for (j = 0;j < data.len;++j)
      if (data.s[j] == '\n') {
        if (byte_equal("search ",7,data.s + i) || byte_equal("search\t",7,data.s + i) || byte_equal("domain ",7,data.s + i) || byte_equal("domain\t",7,data.s + i)) {
          stralloc_copys(rules,"?:");
          i += 7;
          while (i < j) {
            k = byte_chr(data.s + i,j - i,' ');
            k = byte_chr(data.s + i,k,'\t');
            if (!k) { ++i; continue; }
            stralloc_cats(rules,"+.");
            stralloc_catb(rules,data.s + i,k);
            i += k;
          }
          stralloc_0(rules);
          stralloc_cats(rules,"*.:");
          stralloc_0(rules);
          return 0;
        }
        i = j + 1;
      }
  }

  host[0] = 0;
  if (gethostname(host,sizeof host) == -1) return -1;
  host[(sizeof host) - 1] = 0;
  i = str_chr(host,'.');
  if (host[i]) {
    stralloc_copys(rules,"?:");
    stralloc_cats(rules,host + i);
    stralloc_0(rules);
  }
  stralloc_cats(rules,"*.:");
  stralloc_0(rules);

  return 0;
}

static int ok = 0;
static unsigned int uses;
static struct taia deadline;
static stralloc rules = {0}; /* defined if ok */

int dns_resolvconfrewrite(stralloc *out)
{
  struct taia now;

  taia_now(&now);
  if (taia_less(&deadline,&now)) ok = 0;
  if (!uses) ok = 0;

  if (!ok) {
    if (init(&rules) == -1) return -1;
    taia_uint(&deadline,600);
    taia_add(&deadline,&now,&deadline);
    uses = 10000;
    ok = 1;
  }

  --uses;
  stralloc_copy(out,&rules);
  return 0;
}
