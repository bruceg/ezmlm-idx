#include "byte.h"
#include "die.h"
#include "dmarc.h"
#include "dns.h"
#include "str.h"

static stralloc ddomain;

int dmarc_fetch(stralloc *out,const char *domain)
{
  stralloc_copys(&ddomain,"_dmarc.");
  stralloc_cats(&ddomain,domain);
  if (dns_txt(out,&ddomain) < 0) return -1;
  /* Check if it's really a DMARC record */
  if (out->len < 10) return 0;
  if (byte_diff(out->s,9,"v=DMARC1;")) return 0;
  return 1;
}

int dmarc_get(const stralloc *rec,const char *key,stralloc *out)
{
  const char *rp;
  const unsigned int keylen = str_len(key);
  const char *end = rec->s + rec->len;
  const char *tp;

  rp = rec->s;
  while (end - rp > keylen + 1) {
    if (byte_equal(rp,keylen,key) && (rp[keylen] == '=' || rp[keylen] == ' ')) {
      rp += keylen;
      /* Skip space before = */
      while (rp < end && *rp == ' ')
	++rp;
      /* Skip = */
      if (rp >= end || *rp++ != '=')
	return 0;
      /* Skip space after = */
      while (rp < end && *rp == ' ')
	++rp;
      /* Skip over to trailing ; */
      tp = rp;
      while (tp < end && *tp != ';')
	++tp;
      /* Trim space before ; */
      while (tp > rp && tp[-1] == ' ')
	--tp;
      /* Finally, can return */
      stralloc_copyb(out,rp,tp - rp);
      return 1;
    }
    while (rp < end && *rp++ != ';')
      ;
    while (rp < end && *rp == ' ')
      ++rp;
  }
  return 0;
}

int dmarc_p_reject(const char *domain)
{
  static stralloc data;
  int r;

  r = dmarc_fetch(&data,domain);
  if (r <= 0)
    return r;
  if (!dmarc_get(&data,"p",&data))
    return 0;
  return data.len == 6 && byte_equal(data.s,6,"reject");
}
