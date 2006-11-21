/*$Id$*/

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
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
static struct sqlinfo info;
static const char* basedir;

static stralloc myp = {0};
static stralloc ers = {0};
static stralloc table = {0};

static const char *parsesql(const char *subdir,
			    struct sqlinfo *info)
/* reads the file dbname/sql, and if the file exists, parses it into the    */
/* components. The string should be host:port:user:pw:db:table.         If  */
/* the file does not exists, returns "". On success returns NULL. On error  */
/* returns error string for temporary error. */
/* Note that myp is static and all pointers point to it.*/
{
  unsigned int j;
  const char *port;

  info->db = "ezmlm";
  info->host = info->user = info->pw = info->table = info->conn = 0;
  info->port = 0;

		/* host:port:db:table:user:pw:name */
  myp.len = 0;
  port = 0;
  switch (slurp("sql",&myp,128)) {
	case -1:	if (!stralloc_copys(&ers,ERR_READ)) return ERR_NOMEM;
			if (!stralloc_cats(&ers,"sql")) return ERR_NOMEM;
			if (!stralloc_0(&ers)) return ERR_NOMEM;
			return ers.s;
	case 0: return "";
  }
  if (!stralloc_append(&myp,"\n")) return ERR_NOMEM;
  if (myp.s[j = str_chr(myp.s,'\n')])
    myp.s[j] = '\0';
						/* get connection parameters */
  if (!stralloc_0(&myp)) return ERR_NOMEM;
  info->host = myp.s;
  if (myp.s[j = str_chr(myp.s,':')]) {
    myp.s[j++] = '\0';
    port = myp.s + j;
    if (myp.s[j += str_chr(myp.s+j,':')]) {
      myp.s[j++] = '\0';
      info->user = myp.s + j;
      if (myp.s[j += str_chr(myp.s+j,':')]) {
	myp.s[j++] = '\0';
        info->pw = myp.s + j;
	if (myp.s[j += str_chr(myp.s+j,':')]) {
	  myp.s[j++] = '\0';
          info->db = myp.s + j;
	  if (myp.s[j += str_chr(myp.s+j,':')]) {
	    myp.s[j++] = '\0';
	    info->table = myp.s + j;
	  }
	}
      }
    }
  }
  if (port && *port)
    scan_ulong(port,&info->port);
  if (info->host && !*info->host)
    info->host = (char *) 0;
  if (info->user && !*info->user)
    info->user = (char *) 0;
  if (info->pw && !*info->pw)
    info->pw = (char *) 0;
  if (info->db && !*info->db)
    info->db = (char *) 0;
  if (!info->table || !*info->table)
    return ERR_NO_TABLE;
  if (subdir != 0
      && subdir[0] != 0
      && (subdir[0] != '.' || subdir[1] != 0)) {
    if (!stralloc_copys(&table,info->table)) return ERR_NOMEM;
    if (!stralloc_append(&table,"_")) return ERR_NOMEM;
    if (!stralloc_cats(&table,subdir)) return ERR_NOMEM;
    if (!stralloc_0(&table)) return ERR_NOMEM;
    info->table = table.s;
  }
  return (char *) 0;
}

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
  }
  return subdir;
}

static const char *opensub(const char *subdir,
			   struct sqlinfo *info)
{
  const char *err;
  if (plugin) {
    if ((err = parsesql(subdir,info)) != 0)
      return err;
    return plugin->open(info);
  }
  return 0;
}

const char *checktag(unsigned long msgnum,
		     unsigned long listno,
		     const char *action,
		     const char *seed,
		     const char *hash)
{
  const char *r = 0;
  if ((r = opensub(0,&info)) != 0)
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
  if ((r = opensub(subdir,&info)) != 0)
    strerr_die2x(111,FATAL,r);
  if (plugin == 0)
    return std_issub(subdir,userhost);
  return plugin->issub(&info,userhost);
}

const char *logmsg(unsigned long num,
		   unsigned long listno,
		   unsigned long subs,
		   int done)
{
  const char *r = 0;
  if ((r = opensub(0,&info)) != 0)
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
  if ((r = opensub(subdir,&info)) != 0)
    strerr_die2x(111,FATAL,r);
  return plugin->putsubs(&info,hash_lo,hash_hi,subwrite);
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

  if ((r = opensub(subdir,&info)) != 0)
    strerr_die2x(111,FATAL,r);
  if (plugin == 0)
    return std_searchlog(subdir,search,subwrite);
  return plugin->searchlog(&info,search,subwrite);
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
  if ((r = opensub(subdir,&info)) != 0)
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
  if ((r = opensub(0,&info)) != 0)
    strerr_die2x(111,FATAL,r);
  if (plugin != 0)
    plugin->tagmsg(&info,msgnum,hashout,bodysize,chunk);
}

void initsub(const char *dir)
{
  struct stat st;
  void *handle;

  basedir = dir;
  if (stat("sql",&st) == 0) {
    std_makepath(&path,auto_lib,"/sub-sql.so",0);
    if ((handle = dlopen(path.s, RTLD_NOW | RTLD_LOCAL)) == 0)
      strerr_die3x(111,FATAL,"Could not load SQL plugin: ",dlerror());
    else if ((plugin = dlsym(handle,"sub_plugin")) == 0)
      strerr_die3x(111,FATAL,"SQL plugin is missing symbols: ",dlerror());
  }
}
