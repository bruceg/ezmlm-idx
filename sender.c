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

static int is_hex(int ch)
{
  return (ch >= '0' && ch <= '9')
    || (ch >= 'a' && ch <= 'f')
    || (ch >= 'A' && ch <= 'F');
}

static int is_batv_tag(const char *s, int len)
{
  /* The BATV standard says the PRVS tag is KDDDSSSSSS, where K and D
   * are decimal digits, and S is hexadecimal.  However most tags found
   * in the wild only contain DDDSSSSSS.  There is also appears to be an
   * alternate format of DDDSSSSSSSS.  This routine handles all of them
   * simply by looking for a string of hexadecimal characters at least 9
   * characters long. */
  if (len >= 9) {
    for (; len > 0; --len, ++s) {
      if (!is_hex(*s))
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
  if (is_batv_tag(s+5,sep-5)) {
    if (!stralloc_copys(&realsender,s+sep+1)) die_nomem();
    if (!stralloc_0(&realsender)) die_nomem();
    return 1;
  }
  /* Format: prvs=USER=TAG@example.com */
  for (sep = at - 1; sep > 5 && s[sep] != '='; --sep)
    ;
  if (is_batv_tag(s + sep + 1, at - sep - 1)) {
    if (!stralloc_copyb(&realsender,s+5,sep-5)) die_nomem();
    if (!stralloc_cats(&realsender,s+at)) die_nomem();
    if (!stralloc_0(&realsender)) die_nomem();
    return 1;
  }
  return 0;
}

static int decode_btv1(const char* s)
{
  /* Other implementations of BATV use "btv1" instead of "prvs" and two
   * equal signs instead of one as a delimiter, and a different tag value. */
  int at;
  int sep;
  if (s[at = str_rchr(s,'@')] == 0)
    return 0;
  /* Format: btv1==DDDSSSSSSSS==USER@example.com */
  if (s[4] == '=' && s[5] == '=') {
    for (sep = at - 1; sep > 6; --sep) {
      if (s[sep] == '=' && s[sep+1] == '=') {
	if (is_batv_tag(s+6,sep-6)) {
	  if (!stralloc_copys(&realsender,s+sep+2)) die_nomem();
	  if (!stralloc_0(&realsender)) die_nomem();
	  return 1;
	}
	break;
      }
    }
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
    else if (str_start(s, "btv1=")
	     && decode_btv1(s))
      s = realsender.s;
  }
  return s;
}
