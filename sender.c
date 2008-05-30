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

const char *get_sender(void)
{
  const char *s;
  if ((s = env_get("SENDER")) != 0) {
    if ((str_start(s,"SRS0")
	 || str_start(s,"SRS1"))
	&& decode_srs(s))
      s = realsender.s;
  }
  return s;
}
