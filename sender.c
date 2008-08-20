#include "die.h"
#include "env.h"
#include "str.h"
#include "stralloc.h"

static stralloc realsender;

static int decode_srs(const char *s)
{
  /* Format: SRS0=HHH=TT=DOMAIN=USER@example.com */
  /* Format: SRS1=HHH=forward.com==HHH=TT=DOMAIN=USER@example.com */
  char sep;
  int at;
  int sep1;
  int sep2;
  if ((sep = *s++) == 0)
    return 0;
  if (s[at = str_rchr(s,'@')] == 0)
    return 0;
  for (sep2 = at - 1; sep2 > 1 && s[sep2] != '='; sep2--)
    ;
  /* s+sep2 = "=USER@..." */
  for (sep1 = sep2 - 1; sep1 > 1 && s[sep1] != '='; sep1--)
    ;
  /* s+sep1 = "=DOMAIN=USER@..." */
  if (sep2 <= 0)
    return 0;
  ++sep1;
  ++sep2;

  if (!stralloc_copyb(&realsender,s+sep2,at-sep2)) die_nomem();
  if (!stralloc_append(&realsender,"@")) die_nomem();
  if (!stralloc_catb(&realsender,s+sep1,sep2-sep1-1)) die_nomem();
  if (!stralloc_0(&realsender)) die_nomem();
  return 1;
}

static int is_prvs_tag(const char *s, int len)
{
  /* The BATV standard says the PRVS tag is KDDDSSSSSS, where K and D
   * are decimal digits, and S is hexadecimal.  However most tags found
   * in the wild only contain DDDSSSSSS. */
  if (len == 10 && *s >= '0' && *s <= '9') {
    --len;
    ++s;
  }
  if (len == 9) {
    for (; len > 6; --len, ++s) {
      if (!(*s >= '0' && *s <= '9'))
	return 0;
    }
    for (; len > 0; --len, ++s) {
      if (!(*s >= '0' && *s <= '9')
	  && !(*s >= 'a' && *s <= 'f'))
	return 0;
    }
    return 1;
  }
  return 0;
}

static int decode_prvs(const char *s)
{
  /* The BATV standard says the user part should have the format
   * "tag-type=tag-val=loc-core" where "loc-core" is the original local
   * address, but all the examples I could find in actual use had the
   * last two parts reversed.  What a mess.  So, I have to check if
   * either the first or second part is a valid prvs tag, and use the
   * other one. */
  int at;
  int sep;
  if (s[at = str_rchr(s,'@')] == 0)
    return 0;
  /* Format: prvs=TAG=USER@example.com */
  for (sep = 5; sep < at && s[sep] != '='; ++sep)
    ;
  if (sep >= at)
    return 0;
  if (is_prvs_tag(s+5,sep-5)) {
    if (!stralloc_copys(&realsender,s+sep+1)) die_nomem();
    if (!stralloc_0(&realsender)) die_nomem();
    return 1;
  }
  /* Format: prvs=USER=TAG@example.com */
  for (sep = at - 1; sep > 5 && s[sep] != '='; --sep)
    ;
  if (is_prvs_tag(s + sep + 1, at - sep - 1)) {
    if (!stralloc_copyb(&realsender,s+5,sep-5)) die_nomem();
    if (!stralloc_cats(&realsender,s+at)) die_nomem();
    if (!stralloc_0(&realsender)) die_nomem();
    return 1;
  }
  return 0;
}

const char *get_sender(void)
{
  const char *s;
  if ((s = env_get("SENDER")) != 0) {
    if ((str_start(s,"SRS0")
	 || str_start(s,"SRS1"))
	&& decode_srs(s))
      s = realsender.s;
    else if (str_start(s, "prvs=")
	     && decode_prvs(s))
      s = realsender.s;
  }
  return s;
}
