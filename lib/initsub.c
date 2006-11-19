/*$Id$*/

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "case.h"
#include "die.h"
#include "errtxt.h"
#include "str.h"
#include "strerr.h"
#include "sub_std.h"
#include "subscribe.h"
#include "auto_lib.h"

static stralloc path = {0};
static struct sub_plugin *plugin = 0;

const char *checktag(const char *dir,
		     unsigned long msgnum,
		     unsigned long listno,
		     const char *action,
		     const char *seed,
		     const char *hash)
{
  const char *r = 0;
  if (!plugin || (r = plugin->opensub(dir,0)) != 0) {
    if (r && *r)
      return r;
    return std_checktag(msgnum,action,seed,hash);
  }
  return plugin->checktag(dir,msgnum,listno,action,seed,hash);
}

void closesub(void) {
  if (plugin != 0)
    plugin->closesub();
}

const char *issub(const char *dir,
		  const char *subdir,
		  const char *userhost)
{
  const char *r = 0;
  if (!plugin || (r = plugin->opensub(dir,subdir)) != 0) {
    if (r && *r)
      strerr_die2x(111,FATAL,r);
    return std_issub(dir,subdir,userhost);
  }
  return plugin->issub(dir,subdir,userhost);
}

const char *logmsg(const char *dir,
		   unsigned long num,
		   unsigned long listno,
		   unsigned long subs,
		   int done)
{
  const char *r = 0;
  if (!plugin || (r = plugin->opensub(dir,0)) != 0) {
    if (r && *r)
      return r;
    return 0;
  }
  return plugin->logmsg(dir,num,listno,subs,done);
}

unsigned long putsubs(const char *dir,
		      const char *subdir,
		      unsigned long hash_lo,
		      unsigned long hash_hi,
		      int subwrite(),
		      int flagsql)
{
  const char *r = 0;
  if (!plugin || !flagsql || (r = plugin->opensub(dir,subdir)) != 0) {
    if (r && *r)
      strerr_die2x(111,FATAL,r);
    return std_putsubs(dir,subdir,hash_lo,hash_hi,subwrite);
  }
  return plugin->putsubs(dir,subdir,hash_lo,hash_hi,subwrite);
}

void searchlog(const char *dir,
	       const char *subdir,
	       char *search,
	       int subwrite())
{
  unsigned char *cps;
  unsigned char ch;
  unsigned int searchlen;
  const char *r = 0;

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

  if (!plugin || (r = plugin->opensub(dir,subdir)) != 0) {
    if (r && *r)
      strerr_die2x(111,FATAL,r);
    return std_searchlog(dir,subdir,search,subwrite);
  }
  return plugin->searchlog(dir,subdir,search,subwrite);
}

int subscribe(const char *dir,
	      const char *subdir,
	      const char *userhost,
	      int flagadd,
	      const char *comment,
	      const char *event,
	      int flagsql,
	      int forcehash)
{
  const char *r = 0;

  if (userhost[str_chr(userhost,'\n')])
    strerr_die2x(100,FATAL,ERR_ADDR_NL);

  if (!plugin || !flagsql || (r = plugin->opensub(dir,subdir)) != 0) {
    if (r && *r)
      strerr_die2x(111,FATAL,r);
    return std_subscribe(dir,subdir,userhost,flagadd,comment,event,forcehash);
  }
  return plugin->subscribe(dir,subdir,userhost,flagadd,comment,event,forcehash);
}

void tagmsg(const char *dir,
	    unsigned long msgnum,
	    const char *seed,
	    const char *action,
	    char *hashout,
	    unsigned long bodysize,
	    unsigned long chunk)
{
  const char *r = 0;
  std_tagmsg(msgnum,seed,action,hashout);
  if (!plugin || (r = plugin->opensub(dir,0)) != 0) {
    if (r && *r)
      strerr_die2x(111,FATAL,r);
    return;
  }
  return plugin->tagmsg(dir,msgnum,seed,action,hashout,bodysize,chunk);
}

void initsub(const char *dir)
{
  struct stat st;
  void *handle;

  std_makepath(&path,dir,0,"/sql",0);
  if (stat(path.s,&st) == 0) {
    std_makepath(&path,auto_lib,0,"/sub-sql.so",0);
    if ((handle = dlopen(path.s, RTLD_NOW | RTLD_LOCAL)) == 0)
      strerr_die3x(111,FATAL,"Could not load SQL plugin: ",dlerror());
    else if ((plugin = dlsym(handle,"sub_plugin")) == 0)
      strerr_die3x(111,FATAL,"SQL plugin is missing symbols: ",dlerror());
  }
}
