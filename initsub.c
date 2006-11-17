/*$Id$*/

#include "subscribe.h"
#include "sub_std.h"

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

void initsub(const char *dir)
{
  checktag = wrap_checktag;
  closesub = no_closesub;
  issub = std_issub;
  logmsg = (logmsg_fn)no_logmsg;
  putsubs = wrap_putsubs;
  tagmsg = wrap_tagmsg;
  searchlog = std_searchlog;
  subscribe = wrap_subscribe;
  (void)dir;
}
