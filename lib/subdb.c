/*$Id$*/

#include <dlfcn.h>
#include "case.h"
#include "die.h"
#include "errtxt.h"
#include "scan.h"
#include "slurp.h"
#include "stralloc.h"
#include "str.h"
#include "strerr.h"
#include "sub_std.h"
#include "subscribe.h"
#include "auto_lib.h"

static stralloc path = {0};
static struct sub_plugin *plugin = 0;
static struct subdbinfo info;
static const char* basedir;

static const char *parsesubdb(const char *filename, const char *assumed)
/* reads the named file , and if the file exists, parses it into the
 * components. The string should be
 * [plugin:]host[:port[:user[:pw[:db[:base_table]]]]].  If the file does
 * not exists, returns "". On success returns NULL. On error returns
 * error string for temporary error.  Note that myp is static and all
 * pointers point to it.*/
{
  unsigned int j;
  const char *port;
  static stralloc myp = {0};

  info.db = "ezmlm";
  info.host = info.user = info.pw = info.base_table = info.conn = 0;
  info.port = 0;

		/* [plugin:]host:port:db:table:user:pw:name */
  myp.len = 0;
  port = 0;
  switch (slurp(filename,&myp,128)) {
	case -1:	if (!stralloc_copys(&myp,ERR_READ)) return ERR_NOMEM;
			if (!stralloc_cats(&myp,filename)) return ERR_NOMEM;
			if (!stralloc_0(&myp)) return ERR_NOMEM;
			return myp.s;
	case 0: return "";
  }
  if (!stralloc_append(&myp,"\n")) return ERR_NOMEM;
  if (myp.s[j = str_chr(myp.s,'\n')])
    myp.s[j] = '\0';
						/* get connection parameters */
  if (!stralloc_0(&myp)) return ERR_NOMEM;
  if (assumed != 0) {
    info.plugin = assumed;
    j = 0;
  }
  else {
    info.plugin = myp.s;
    if (myp.s[j = str_chr(myp.s,':')])
      myp.s[j++] = '\0';
  }
  info.host = myp.s + j;
  if (myp.s[j += str_chr(myp.s+j,':')]) {
    myp.s[j++] = '\0';
    port = myp.s + j;
    if (myp.s[j += str_chr(myp.s+j,':')]) {
      myp.s[j++] = '\0';
      info.user = myp.s + j;
      if (myp.s[j += str_chr(myp.s+j,':')]) {
	myp.s[j++] = '\0';
        info.pw = myp.s + j;
	if (myp.s[j += str_chr(myp.s+j,':')]) {
	  myp.s[j++] = '\0';
          info.db = myp.s + j;
	  if (myp.s[j += str_chr(myp.s+j,':')]) {
	    myp.s[j++] = '\0';
	    info.base_table = myp.s + j;
	  }
	}
      }
    }
  }
  if (!info.plugin || !*info.plugin)
    return ERR_NO_PLUGIN;
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
  return (char *) 0;
}

/* Fix up the named subdirectory to strip off the leading base directory
 * if it is an absolute path, or reject it if it falls outside of the
 * base directory. */
static const char *fixsubdir(const char *subdir)
{
  unsigned int dir_len;
  if (subdir != 0) {
    if (subdir[0] == '/') {
      dir_len = str_len(basedir);
      if (str_diffn(subdir,basedir,dir_len) != 0
	  || (subdir[dir_len] != '/'
	      && subdir[dir_len] != 0))
	strerr_die2x(111,FATAL,"FIXME: absolute subdirectories no longer supported.");
      subdir += dir_len + (subdir[dir_len] == '/');
    }
    if (subdir[str_chr(subdir,'/')] == '/')
      strerr_die2x(111,FATAL,"FIXME: paths in subdir names no longer supported.");
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
  r = (plugin == 0)
    ? std_checktag(msgnum,action,seed,hash)
    : plugin->checktag(&info,msgnum,listno,hash);
  if (listno && r == 0)
    (void) logmsg(msgnum,listno,0L,3);
  return r;
}

void closesub(void) {
  if (plugin != 0)
    plugin->close(&info);
}

const char *issub(const char *subdir,
		  const char *userhost)
{
  const char *r = 0;
  subdir = fixsubdir(subdir);
  if ((r = opensub()) != 0)
    strerr_die2x(111,FATAL,r);
  if (plugin == 0)
    return std_issub(subdir,userhost);
  return plugin->issub(&info,subdir,userhost);
}

const char *logmsg(unsigned long num,
		   unsigned long listno,
		   unsigned long subs,
		   int done)
{
  const char *r = 0;
  if ((r = opensub()) != 0)
    return r;
  if (plugin == 0)
    return 0;
  return plugin->logmsg(&info,num,listno,subs,done);
}

unsigned long putsubs(const char *subdir,
		      unsigned long hash_lo,
		      unsigned long hash_hi,
		      int subwrite(),
		      int flagsql)
{
  const char *r = 0;
  subdir = fixsubdir(subdir);
  if (!flagsql || plugin == 0)
    return std_putsubs(subdir,hash_lo,hash_hi,subwrite);
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
  if (plugin == 0)
    return std_searchlog(subdir,search,subwrite);
  return plugin->searchlog(&info,subdir,search,subwrite);
}

int subscribe(const char *subdir,
	      const char *userhost,
	      int flagadd,
	      const char *comment,
	      const char *event,
	      int flagsql,
	      int forcehash)
{
  const char *r = 0;

  subdir = fixsubdir(subdir);

  if (userhost[str_chr(userhost,'\n')])
    strerr_die2x(100,FATAL,ERR_ADDR_NL);

  if (!flagsql || plugin == 0)
    return std_subscribe(subdir,userhost,flagadd,comment,event,forcehash);
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
  std_tagmsg(msgnum,seed,action,hashout);
  if ((r = opensub()) != 0)
    strerr_die2x(111,FATAL,r);
  if (plugin != 0)
    plugin->tagmsg(&info,msgnum,hashout,bodysize,chunk);
}

void initsub(const char *dir)
{
  void *handle;
  const char *err;

  basedir = dir;
  if ((err = parsesubdb("subdb",0)) == 0
      || (*err == '\0'
	  && (err = parsesubdb("sql","sql")) == 0)) {
    if (!stralloc_copys(&path,auto_lib)) die_nomem();
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
  else if (*err != '\0')
    strerr_die2x(111,FATAL,err);
}
