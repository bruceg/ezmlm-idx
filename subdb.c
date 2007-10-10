/*$Id$*/

#include <dlfcn.h>
#include "case.h"
#include "config.h"
#include "cookie.h"
#include "die.h"
#include "env.h"
#include "errtxt.h"
#include "fmt.h"
#include "scan.h"
#include "slurp.h"
#include "stralloc.h"
#include "str.h"
#include "strerr.h"
#include "subdb.h"
#include "auto_lib.h"

static stralloc line = {0};
static stralloc path = {0};
static struct sub_plugin *plugin = 0;
static struct subdbinfo info;

static void parsesubdb(const char *plugin)
/* Parses line into the components. The string should be
 * plugin[:host[:port[:user[:pw[:db[:base_table]]]]]]. On success
 * returns NULL. On error returns error string for temporary error. */
{
  unsigned int j;
  const char *port;

  info.db = "ezmlm";
  info.host = info.user = info.pw = info.base_table = info.conn = 0;
  info.port = 0;

		/* [plugin:]host:port:db:table:user:pw:name */
  port = 0;
  if (!stralloc_append(&line,"\n")) die_nomem();
  if (!stralloc_0(&line)) die_nomem();
  if (line.s[j = str_chr(line.s,'\n')])
    line.s[j] = '\0';
						/* get connection parameters */
  if (plugin != 0) {
    info.plugin = plugin;
    j = 0;
  }
  else {
    info.plugin = line.s;
    if (line.s[j = str_chr(line.s,':')])
      line.s[j++] = '\0';
  }
  info.host = line.s + j;
  if (line.s[j += str_chr(line.s+j,':')]) {
    line.s[j++] = '\0';
    port = line.s + j;
    if (line.s[j += str_chr(line.s+j,':')]) {
      line.s[j++] = '\0';
      info.user = line.s + j;
      if (line.s[j += str_chr(line.s+j,':')]) {
	line.s[j++] = '\0';
        info.pw = line.s + j;
	if (line.s[j += str_chr(line.s+j,':')]) {
	  line.s[j++] = '\0';
          info.db = line.s + j;
	  if (line.s[j += str_chr(line.s+j,':')]) {
	    line.s[j++] = '\0';
	    info.base_table = line.s + j;
	  }
	}
      }
    }
  }
  if (!info.plugin || !*info.plugin)
    strerr_die2x(111,FATAL,ERR_NO_PLUGIN);
  if (port && *port)
    scan_ulong(port,&info.port);
  if (info.host && !*info.host)
    info.host = (char *) 0;
  if (info.user && !*info.user)
    info.user = (char *) 0;
  if (info.pw && !*info.pw)
    info.pw = (char *) 0;
  if (info.db && !*info.db)
    info.db = (char *) 0;
  if (!info.base_table || !*info.base_table)
    info.base_table = "ezmlm";
}

static int loadsubdb(const char *filename, const char *plugin)
{
  line.len = 0;
  switch (slurp(filename,&line,128)) {
  case -1:
    strerr_die3x(111,FATAL,ERR_READ,filename);
  case 0:
    return 0;
  default:
    parsesubdb(plugin);
    return 1;
  }
}

/* Fix up the named subdirectory to strip off the leading base directory
 * if it is an absolute path, or reject it if it falls outside of the
 * base directory. */
static const char *fixsubdir(const char *subdir)
{
  unsigned int dir_len;
  if (subdir != 0) {
    if (subdir[0] == '/') {
      dir_len = str_len(listdir);
      if (str_diffn(subdir,listdir,dir_len) != 0
	  || (subdir[dir_len] != '/'
	      && subdir[dir_len] != 0))
	strerr_die2x(111,FATAL,ERR_NO_ABSOLUTE);
      subdir += dir_len;
      while (*subdir == '/')
	++subdir;
    }
    if (subdir[str_chr(subdir,'/')] == '/')
      strerr_die2x(111,FATAL,ERR_NO_LEVELS);
    if (subdir[0] == 0
	|| (subdir[0] == '.' && subdir[1] == 0))
      subdir = 0;
  }
  return subdir;
}

static const char *opensub(void)
{
  if (plugin)
    return plugin->open(&info);
  return 0;
}

const char *checktag(unsigned long msgnum,
		     unsigned long listno,
		     const char *action,
		     const char *seed,
		     const char *hash)
{
  const char *r = 0;
  if ((r = opensub()) != 0)
    return r;
  r = plugin->checktag(&info,msgnum,listno,action,seed,hash);
  if (listno && r == 0)
    (void) logmsg(msgnum,listno,0L,3);
  return r;
}

void closesub(void) {
  if (plugin != 0)
    plugin->close(&info);
}

int issub(const char *subdir,
	  const char *userhost,
	  stralloc *recorded)
{
  const char *r = 0;
  subdir = fixsubdir(subdir);
  if ((r = opensub()) != 0)
    strerr_die2x(111,FATAL,r);
  return plugin->issub(&info,subdir,userhost,recorded);
}

const char *logmsg(unsigned long num,
		   unsigned long listno,
		   unsigned long subs,
		   int done)
{
  const char *r = 0;
  if (plugin == 0)
    return 0;
  if ((r = opensub()) != 0)
    return r;
  return plugin->logmsg(&info,num,listno,subs,done);
}

unsigned long putsubs(const char *subdir,
		      unsigned long hash_lo,
		      unsigned long hash_hi,
		      int subwrite())
{
  const char *r = 0;
  subdir = fixsubdir(subdir);
  if ((r = opensub()) != 0)
    strerr_die2x(111,FATAL,r);
  return plugin->putsubs(&info,subdir,hash_lo,hash_hi,subwrite);
}

void searchlog(const char *subdir,
	       char *search,
	       int subwrite())
{
  unsigned char *cps;
  unsigned char ch;
  unsigned int searchlen;
  const char *r = 0;

  subdir = fixsubdir(subdir);

  if (!search) search = (char*)"";      /* defensive */
  searchlen = str_len(search);
  case_lowerb(search,searchlen);
  cps = (unsigned char *) search;
  while ((ch = *(cps++))) {     /* search is potentially hostile */
    if (ch >= 'a' && ch <= 'z') continue;
    if (ch >= '0' && ch <= '9') continue;
    if (ch == '.' || ch == '_') continue;
    *(cps - 1) = '_';           /* will match char specified as well */
  }

  if ((r = opensub()) != 0)
    strerr_die2x(111,FATAL,r);
  return plugin->searchlog(&info,subdir,search,subwrite);
}

int subscribe(const char *subdir,
	      const char *userhost,
	      int flagadd,
	      const char *comment,
	      const char *event,
	      int forcehash)
{
  const char *r = 0;

  subdir = fixsubdir(subdir);

  if (userhost[str_chr(userhost,'\n')])
    strerr_die2x(100,FATAL,ERR_ADDR_NL);

  if ((r = opensub()) != 0)
    strerr_die2x(111,FATAL,r);
  return plugin->subscribe(&info,subdir,userhost,flagadd,comment,event,forcehash);
}

void tagmsg(unsigned long msgnum,
	    const char *seed,
	    const char *action,
	    char *hashout,
	    unsigned long bodysize,
	    unsigned long chunk)
{
  const char *r = 0;
  char strnum[FMT_ULONG];
  strnum[fmt_ulong(strnum,msgnum)] = '\0';	/* message nr ->string*/
  cookie(hashout,key.s,key.len,strnum,seed,action);
  if ((r = opensub()) != 0)
    strerr_die2x(111,FATAL,r);
  if (plugin != 0)
    plugin->tagmsg(&info,msgnum,hashout,bodysize,chunk);
}

void initsub(const char *subdbline)
{
  void *handle;

  if (subdbline == 0) {
    if (!loadsubdb("subdb",0))
      if (!loadsubdb("sql","sql"))
	parsesubdb("std");
  }
  else {
    if (!stralloc_copys(&line,subdbline)) die_nomem();
    parsesubdb(0);
  }
  if (!stralloc_copys(&path,auto_lib())) die_nomem();
  if (!stralloc_cats(&path,"/sub-")) die_nomem();
  if (!stralloc_cats(&path,info.plugin)) die_nomem();
  if (!stralloc_cats(&path,".so")) die_nomem();
  if (!stralloc_0(&path)) die_nomem();
  if ((handle = dlopen(path.s, RTLD_NOW | RTLD_LOCAL)) == 0)
    strerr_die5x(111,FATAL,"Could not load plugin ",path.s,": ",
		 dlerror());
  else if ((plugin = dlsym(handle,"sub_plugin")) == 0)
    strerr_die5x(111,FATAL,"Plugin ",path.s," is missing symbols: ",
		 dlerror());
}
