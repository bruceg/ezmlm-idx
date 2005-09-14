/*$Id$*/

#include <unistd.h>
#include "config.h"
#include "die.h"
#include "errtxt.h"
#include "getconf.h"
#include "slurp.h"
#include "strerr.h"

stralloc key = {0};
stralloc listid = {0};
stralloc mailinglist = {0};
stralloc outhost = {0};
stralloc outlocal = {0};

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
  getconf_line(&listid,"listid",0,dir);
  getconf_line(&outhost,"outhost",1,dir);
  getconf_line(&outlocal,"outlocal",1,dir);
}
