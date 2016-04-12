#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "error.h"
#include "case.h"
#include "stralloc.h"
#include "str.h"
#include "env.h"
#include "sender.h"
#include "error.h"
#include "sig.h"
#include "fork.h"
#include "wait.h"
#include "strerr.h"
#include "byte.h"
#include "getln.h"
#include "qmail.h"
#include "substdio.h"
#include "readwrite.h"
#include "seek.h"
#include "quote.h"
#include "datetime.h"
#include "now.h"
#include "fmt.h"
#include "getconfopt.h"
#include "cookie.h"
#include "messages.h"
#include "copy.h"
#include "open.h"
#include "lock.h"
#include "wrap.h"
#include "die.h"
#include "idx.h"
#include "mime.h"
#include "config.h"
#include "auto_version.h"

static int flagmime = MOD_MIME;	/* default is message as attachment */

const char FATAL[] = "ezmlm-confirm: fatal: ";
const char INFO[] = "ezmlm-confirm: info: ";
const char USAGE[] =
"ezmlm-confirm: usage: ezmlm-confirm [-cCmMrRvV] dir [/path/ezmlm-send]";

static const char *replyto = (char *) 0;
static stralloc sendopt = {0};

static struct option options[] = {
  OPT_COPY_FLAG(sendopt,'c'),
  OPT_COPY_FLAG(sendopt,'C'),
  OPT_COPY_FLAG(sendopt,'r'),
  OPT_COPY_FLAG(sendopt,'R'),
  OPT_FLAG(flagmime,'m',1,0),
  OPT_FLAG(flagmime,'M',0,0),
  OPT_CSTR(replyto,'t',0),
  OPT_CSTR(replyto,'T',0),
  OPT_END
};

static stralloc to = {0};
static datetime_sec when;
static stralloc line = {0};

static int checkfile(const char *fn,stralloc *fnmsg)
/* looks for DIR/mod/unconfirmed/fn.              */
/* Returns:                                       */
/*          1 found in unconfirmed                */
/*          0 not found                           */
/* Handles errors.                                */
/* ALSO: if found, fnmsg contains the o-terminated*/
/* file name.                                     */
{
  struct stat st;
  stralloc_copys(fnmsg,"mod/unconfirmed/");
  stralloc_cats(fnmsg,fn);
  stralloc_0(fnmsg);
  if (stat(fnmsg->s,&st) == 0)
    return 1;
  if (errno != error_noent)
    strerr_die2sys(111,FATAL,MSG1(ERR_STAT,fnmsg->s));
  return 0;
}

static void maketo(void)
/* expects line to be a return-path line. If it is and the format is valid */
/* to is set to to the sender. Otherwise, to is left untouched. Assuming   */
/* to is empty to start with, it will remain empty if no sender is found.  */
{
  unsigned int x, y;

  if (case_startb(line.s,line.len,"return-path:")) {
    x = 12 + byte_chr(line.s + 12,line.len-12,'<');
    if (x != line.len) {
      y = byte_rchr(line.s + x,line.len-x,'>');
      if (y + x != line.len) {
        stralloc_copyb(&to,line.s+x+1,y-1);
        stralloc_0(&to);
      }			/* no return path-> no addressee. A NUL in the sender */
    }			/* is no worse than a faked sender, so no problem */
  }
}

int main(int argc, char **argv)
{
  const char *sender;
  const char *def;
  const char *local;
  const char *action;
  int fd;
  int match;
  unsigned int start,confnum;
  int child;
  int opt;
  const char *cp;

  char hash[COOKIE];
  stralloc fnbase = {0};
  stralloc fnmsg = {0};

  const char *dir;

  substdio sstext;
  char textbuf[1024];


  (void) umask(022);
  sig_pipeignore();
  when = now();

  stralloc_copys(&sendopt,"-");
  opt = getconfopt(argc,argv,options,1,&dir);

  sender = get_sender();
  if (!sender) die_sender();
  local = env_get("LOCAL");
  if (!local) strerr_die2x(100,FATAL,MSG(ERR_NOLOCAL));
  def = env_get("DEFAULT");
  if (!def) strerr_die2x(100,FATAL,MSG(ERR_NODEFAULT));

  if (!*sender)
    strerr_die2x(100,FATAL,MSG(ERR_BOUNCE));
  if (!sender[str_chr(sender,'@')])
    strerr_die2x(100,FATAL,MSG(ERR_ANONYMOUS));
  if (str_equal(sender,"#@[]"))
    strerr_die2x(100,FATAL,MSG(ERR_BOUNCE));

  /* local should be >= def, but who knows ... */
  cp = local + str_len(local) - str_len(def) - 2;
  if (cp < local) die_badformat();
  action = local + byte_rchr(local,cp - local,'-');
  if (action == cp) die_badformat();
  action++;

  if (!action[0]) die_badformat();
  if (!str_start(action,ACTION_CONFIRM) && !str_start(action,ACTION_DISCARD))
    die_badformat();
  start = str_chr(action,'-');
  if (!action[start]) die_badformat();
  confnum = 1 + start + str_chr(action + start + 1,'.');
  if (!action[confnum]) die_badformat();
  confnum += 1 + str_chr(action + confnum + 1,'.');
  if (!action[confnum]) die_badformat();
  stralloc_copyb(&fnbase,action+start+1,confnum-start-1);
  stralloc_0(&fnbase);
  cookie(hash,key.s,key.len,fnbase.s,"","a");
  if (byte_diff(hash,COOKIE,action+confnum+1))
    die_badformat();

  lockfile("mod/confirmlock");

  if (!checkfile(fnbase.s,&fnmsg)) strerr_die2x(100,FATAL,MSG(ERR_MOD_TIMEOUT));
/* Here, we have an existing filename in fnbase with the complete path */
/* from the current dir in fnmsg. */

  if (str_start(action,ACTION_DISCARD)) {
        unlink(fnmsg.s);
        strerr_die1x(0,"ezmlm-confirm: info: post rejected");
  } else if (str_start(action,ACTION_CONFIRM)) {
    fd = open_read(fnmsg.s);
    if (fd == -1) {
      if (errno !=error_noent)
        strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,fnmsg.s));
      else	/* shouldn't happen since we've got lock */
        strerr_die3x(100,FATAL,fnmsg.s,MSG(ERR_MOD_TIMEOUT));
    }

    substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));
				/* read "Return-Path:" line */
    if (getln(&sstext,&line,&match,'\n') == -1 || !match)
      strerr_die2sys(111,FATAL,MSG(ERR_READ_INPUT));
    maketo();			/* extract SENDER to "to" */
    env_put2("SENDER",to.s);	/* set SENDER */
    if (seek_begin(fd) == -1)	/* rewind, since we read an entire buffer */
      strerr_die2sys(111,FATAL,MSG1(ERR_SEEK,fnmsg.s));

    if ((child = wrap_fork()) == 0) {
      close(0);
      dup(fd);	/* make fnmsg.s stdin */
      if (argc > opt + 1)
	wrap_execvp((const char **)argv + opt);
      else if (argc > opt)
        wrap_execsh(argv[opt]);
      else
        wrap_execbin("/ezmlm-send", &sendopt, dir);
    }
    /* parent */
    close(fd);
    wrap_exitcode(child);

    unlink(fnmsg.s);
  }
  _exit(0);
}
