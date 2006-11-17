/*$Id$*/

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "die.h"
#include "strerr.h"
#include "sub_std.h"
#include "subscribe.h"
#include "auto_lib.h"

checktag_fn checktag = 0;
closesub_fn closesub = 0;
issub_fn issub = 0;
logmsg_fn logmsg = 0;
putsubs_fn putsubs = 0;
tagmsg_fn tagmsg = 0;
searchlog_fn searchlog = 0;
subscribe_fn subscribe = 0;

static const char *no_logmsg(void) { return 0; }

static void no_closesub(void) { }

static const char *wrap_checktag(const char *dir,
				 unsigned long msgnum,
				 unsigned long listno,
				 const char *action,
				 const char *seed,
				 const char *hash)
{
  return std_checktag(msgnum,action,seed,hash);
  (void)dir;
  (void)listno;
}

static unsigned long wrap_putsubs(const char *dir,
				  const char *subdir,
				  unsigned long hash_lo,
				  unsigned long hash_hi,
				  int subwrite(),
				  int flagsql)
{
  return std_putsubs(dir,subdir,hash_lo,hash_hi,subwrite);
  (void)flagsql;
}

static void wrap_tagmsg(const char *dir,
			unsigned long msgnum,
			const char *seed,
			const char *action,
			char *hashout,
			unsigned long bodysize,
			unsigned long chunk)
{
  std_tagmsg(msgnum,seed,action,hashout);
  (void)dir;
  (void)bodysize;
  (void)chunk;
}

static int wrap_subscribe(const char *dir,
			  const char *subdir,
			  const char *username,
			  int flagadd,
			  const char *from,
			  const char *event,
			  int flagmysql,
			  int forcehash)
{
  return std_subscribe(dir,subdir,username,flagadd,from,event,forcehash);
  (void)flagmysql;
}

static stralloc path;

void initsub(const char *dir)
{
  struct stat st;
  void *handle;
  struct sub_plugin *plugin;

  std_makepath(&path,dir,0,"/sql",0);
  if (stat(path.s,&st) == 0) {
    std_makepath(&path,auto_lib,0,"/sub-sql.so",0);
    if ((handle = dlopen(path.s, RTLD_NOW | RTLD_LOCAL)) == 0)
      strerr_die3x(111,FATAL,"Could not load SQL plugin: ",dlerror());
    else if ((plugin = dlsym(handle,"sub_plugin")) == 0)
      strerr_die3x(111,FATAL,"SQL plugin is missing symbols: ",dlerror());
    else {
      checktag = plugin->checktag;
      closesub = plugin->closesub;
      issub = plugin->issub;
      logmsg = plugin->logmsg;
      putsubs = plugin->putsubs;
      tagmsg = plugin->tagmsg;
      searchlog = plugin->searchlog;
      subscribe = plugin->subscribe;
    }
  }
  else {
    checktag = wrap_checktag;
    closesub = no_closesub;
    issub = std_issub;
    logmsg = (logmsg_fn)no_logmsg;
    putsubs = wrap_putsubs;
    tagmsg = wrap_tagmsg;
    searchlog = std_searchlog;
    subscribe = wrap_subscribe;
  }
}
