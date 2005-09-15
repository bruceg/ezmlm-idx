/*$Id$*/

#include <unistd.h>
#include "config.h"
#include "die.h"
#include "errtxt.h"
#include "getconf.h"
#include "idx.h"
#include "slurp.h"
#include "strerr.h"

stralloc charset = {0};
stralloc key = {0};
stralloc listid = {0};
stralloc mailinglist = {0};
stralloc outhost = {0};
stralloc outlocal = {0};
char flagcd = '\0';		/* No transfer encoding by default */

void startup(const char *dir)
{
  if (dir == 0)
    die_usage();

  if (dir[0] != '/')
    strerr_die2x(100,FATAL,ERR_SLASH);

  if (chdir(dir) == -1)
    strerr_die4sys(111,FATAL,ERR_SWITCH,dir,": ");
}

void load_config(const char *dir)
{
  switch(slurp("key",&key,512)) {
    case -1:
      strerr_die4sys(111,FATAL,ERR_READ,dir,"/key: ");
    case 0:
      strerr_die4x(100,FATAL,dir,"/key",ERR_NOEXIST);
  }

  getconf_line(&mailinglist,"mailinglist",1,dir);
  getconf_line(&outhost,"outhost",1,dir);
  getconf_line(&outlocal,"outlocal",1,dir);

  getconf_line(&listid,"listid",0,dir);
  if (getconf_line(&charset,"charset",0,dir)) {
    if (charset.len >= 2 && charset.s[charset.len - 2] == ':') {
      if (charset.s[charset.len - 1] == 'B' ||
		 charset.s[charset.len - 1] == 'Q') {
        flagcd = charset.s[charset.len - 1];
        charset.s[charset.len - 2] = '\0';
      }
    }
  } else
    if (!stralloc_copys(&charset,TXT_DEF_CHARSET)) die_nomem();
  if (!stralloc_0(&charset)) die_nomem();
}
