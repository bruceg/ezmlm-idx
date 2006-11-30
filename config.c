/*$Id$*/

#include <unistd.h>
#include "byte.h"
#include "config.h"
#include "die.h"
#include "errtxt.h"
#include "getconf.h"
#include "idx.h"
#include "slurp.h"
#include "strerr.h"
#include "wrap.h"

const char *listdir = 0;
stralloc charset = {0};
stralloc ezmlmrc = {0};
stralloc key = {0};
stralloc listid = {0};
stralloc local = {0};
stralloc mailinglist = {0};
stralloc outhost = {0};
stralloc outlocal = {0};
char flagcd = '\0';		/* No transfer encoding by default */
int flags[NO_FLAGS] = {0};

void startup(const char *dir)
{
  if (dir == 0)
    die_usage();

  listdir = dir;
  wrap_chdir(dir);
}

static void load_flags(void)
{
  unsigned int i;

  byte_zero((char*)flags,sizeof flags);
  flags['a' - 'a'] = 1;		/* By default, list is archived (-a) */
  flags['p' - 'a'] = 1;		/* By default, list is public (-p) */
  
  /* Use "key" for temporary storage */
  if (getconf_line(&key,"flags",0)) {
    for (i = 0; i < key.len; ++i) {
      const char ch = key.s[i++];
      if (ch >= 'A' && ch <= 'Z')
	flags[ch - 'A'] = 0;
      else if (ch >= 'a' && ch <= 'z')
	flags[ch - 'a'] = 1;
    }
  }
}

void load_config(void)
{
  load_flags();

  switch(slurp("key",&key,512)) {
    case -1:
      strerr_die4sys(111,FATAL,ERR_READ,listdir,"/key: ");
    case 0:
      strerr_die4x(100,FATAL,listdir,"/key",ERR_NOEXIST);
  }

  /* There are problems with using getconf_line to fetch the ezmlmrc
   * pointer, since the alt location for "ezmlmrc" turns out to be the
   * whole ezmlmrc file itself. */
  switch (slurp("ezmlmrc",&ezmlmrc,64)) {
  case -1:
    strerr_die4sys(111,FATAL,ERR_READ,listdir,"/ezmlmrc: ");
  case 0:
    ezmlmrc.len = 0;
  }
  ezmlmrc.len = byte_chr(ezmlmrc.s,ezmlmrc.len,'\n');

  getconf_line(&outhost,"outhost",1);
  getconf_line(&outlocal,"outlocal",1);
  if (!stralloc_copy(&local,&outlocal)) die_nomem();

  getconf_line(&listid,"listid",0);
  if (getconf_line(&charset,"charset",0)) {
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

  // FIXME: need to handle escapes in mailinglist
  getconf_line(&mailinglist,"mailinglist",1);
}
