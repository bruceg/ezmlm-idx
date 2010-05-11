#include <unistd.h>
#include "byte.h"
#include "config.h"
#include "die.h"
#include "messages.h"
#include "getconf.h"
#include "idx.h"
#include "slurp.h"
#include "str.h"
#include "strerr.h"
#include "wrap.h"

#define NO_FLAGS ('z' - 'a' + 1)

struct flag
{
  int state;
  const char *filename;
};

const char *listdir = 0;
stralloc charset = {0};
stralloc ezmlmrc = {0};
stralloc key = {0};
stralloc listid = {0};
stralloc local = {0};
stralloc outhost = {0};
stralloc outlocal = {0};
char flagcd = '\0';		/* No transfer encoding by default */
static struct flag flags[NO_FLAGS] = {
  { -1, "archived" },		/* a By default, list is archived */
  { -1, "modgetonly" },		/* b */
  { -1, "ezmlmrc" },		/* c */
  { -1, "digest" },		/* d */
  { -1, 0 },			/* e N/A */
  { -1, "prefix" },		/* f */
  { -1, "subgetonly" },		/* g */
  { -1, "nosubconfirm" },	/* h */
  { -1, "threaded" },		/* i */
  { -1, "nounsubconfirm" },	/* j */
  { -1, "deny" },		/* k */
  { -1, "modcanlist" },		/* l */
  { -1, "modpost" },		/* m */
  { -1, "modcanedit" },		/* n */
  { -1, "modpostonly" },	/* o */
  { -1, "public" },		/* p By default, list is public */
  { 1, 0 },			/* q */
  { -1, "remote" },		/* r */
  { -1, "modsub" },		/* s */
  { -1, "addtrailer" },		/* t */
  { -1, "subpostonly" },	/* u */
  { -1, 0 },			/* v unused */
  { -1, "nowarn" },		/* w */
  { -1, "mimeremove" },		/* x */
  { -1, "confirmpost" },	/* y */
  { -1, 0 }			/* z unused */
};

static void parse_flags(const char* start,int len)
{
  while (len > 0) {
    const char ch = *start;
    if (ch >= 'A' && ch <= 'Z')
      flags[ch - 'A'].state = 0;
    else if (ch >= 'a' && ch <= 'z')
      flags[ch - 'a'].state = 1;
    ++start;
    --len;
  }
}

static void load_flags(void)
{
  unsigned int i, j;
  /* Use "key" for temporary storage */
  if (getconf_line(&key,"flags",0)) {
    parse_flags(key.s,key.len);
  }
  else if (getconf(&key,"config",0)) {
    for (i = 0; i < key.len; ++i) {
      for (j = i; j < key.len && key.s[j] != 0; ++j)
	;
      if (key.s[i] == 'F' && key.s[i+1] == ':') {
	parse_flags(key.s+i+2,j-i-2);
	return;
      }
      i = j;
    }
  }
}

static void load_config(void)
{
  load_flags();

  key.len = 0;
  switch(slurp("key",&key,512)) {
    case -1:
      strerr_die2sys(111,FATAL,MSG1(ERR_READ,"key"));
    case 0:
      strerr_die4x(100,FATAL,listdir,"/key",MSG(ERR_NOEXIST));
  }

  /* There are problems with using getconf_line to fetch the ezmlmrc
   * pointer, since the alt location for "ezmlmrc" turns out to be the
   * whole ezmlmrc file itself. */
  switch (slurp("ezmlmrc",&ezmlmrc,64)) {
  case -1:
    strerr_die2sys(111,FATAL,MSG1(ERR_READ,"ezmlmrc"));
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
}

void startup(const char *dir)
{
  if (dir == 0)
    die_usage();

  listdir = dir;
  wrap_chdir(dir);
  load_config();
}

static int getflag(int flagno)
{
  struct flag *flag = &flags[flagno];
  if (flag->state < 0)
    flag->state = (flag->filename == 0) || getconf_isset(flag->filename);
  return flag->state;
}

int flag_isset(char flag)
{
  return
    (flag >= 'A' && flag <= 'Z') ? getflag(flag - 'A')
    : (flag >= 'a' && flag <= 'z') ? getflag(flag - 'a')
    : 0;
}

int flag_isnameset(const char *name)
{
  int i;
  for (i = 0; i < NO_FLAGS; ++i) {
    if (flags[i].filename != 0
	&& str_diff(name, flags[i].filename) == 0)
      return flags[i].state;
  }
  return -1;
}
