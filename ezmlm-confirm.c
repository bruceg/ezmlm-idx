/*$Id: ezmlm-confirm.c 36 2003-12-18 19:26:32Z bruce $*/

#include <sys/types.h>
#include <sys/stat.h>
#include "error.h"
#include "case.h"
#include "stralloc.h"
#include "str.h"
#include "env.h"
#include "error.h"
#include "sig.h"
#include "fork.h"
#include "wait.h"
#include "slurp.h"
#include "getconf.h"
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
#include "date822fmt.h"
#include "fmt.h"
#include "sgetopt.h"
#include "auto_bin.h"
#include "cookie.h"
#include "errtxt.h"
#include "copy.h"
#include "idx.h"
#include "mime.h"
#include "auto_version.h"

static int flagmime = MOD_MIME;	/* default is message as attachment */

#define FATAL "ezmlm-confirm: fatal: "
#define INFO "ezmlm-confirm: info: "

static void die_usage() { strerr_die1x(100,
    "ezmlm-confirm: usage: ezmlm-confirm [-cCmMrRvV] "
    "dir [/path/ezmlm-send]"); }

void die_nomem() { strerr_die2x(111,FATAL,ERR_NOMEM); }

void die_badformat() { strerr_die2x(100,FATAL,ERR_BAD_REQUEST); }

void die_badaddr() { strerr_die2x(100,FATAL,ERR_BAD_ADDRESS); }

static stralloc outhost = {0};
static stralloc inlocal = {0};
static stralloc outlocal = {0};
static stralloc key = {0};
static stralloc mailinglist = {0};
static stralloc to = {0};
static stralloc send = {0};
static stralloc sendopt = {0};
static datetime_sec when;

static char hash[COOKIE];
static stralloc line = {0};
static stralloc fnbase = {0};
static stralloc fnmsg = {0};

static const char *dir;

static struct stat st;

static struct qmail qq;

static int checkfile(const char *fn)
/* looks for DIR/mod/unconfirmed/fn.              */
/* Returns:                                       */
/*          1 found in unconfirmed                */
/*          0 not found                           */
/* Handles errors.                                */
/* ALSO: if found, fnmsg contains the o-terminated*/
/* file name.                                     */
{
  if (!stralloc_copys(&fnmsg,"mod/unconfirmed/")) die_nomem();
  if (!stralloc_cats(&fnmsg,fn)) die_nomem();
  if (!stralloc_0(&fnmsg)) die_nomem();
  if (stat(fnmsg.s,&st) == 0)
    return 1;
  if (errno != error_noent)
    strerr_die6sys(111,FATAL,ERR_STAT,dir,"/",fnmsg.s,": ");
  return 0;
}

static int qqwrite(fd,buf,len) int fd; char *buf; unsigned int len;
{
  qmail_put(&qq,buf,len);
  return len;
}
static char qqbuf[1];
substdio ssqq = SUBSTDIO_FDBUF(qqwrite,-1,qqbuf,sizeof(qqbuf));

static substdio sstext;
static char textbuf[1024];

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
        if (!stralloc_copyb(&to,line.s+x+1,y-1)) die_nomem();
        if (!stralloc_0(&to)) die_nomem();
      }			/* no return path-> no addressee. A NUL in the sender */
    }			/* is no worse than a faked sender, so no problem */
  }
}

void main(int argc, char **argv)
{
  const char *sender;
  const char *def;
  const char *local;
  const char *action;
  int fd, fdlock;
  int match;
  char szchar[2] = "-";
  const char *replyto = (char *) 0;
  unsigned int start,confnum;
  int child;
  int opt;
  const char *cp;
  int wstat;

  (void) umask(022);
  sig_pipeignore();
  when = now();

  if (!stralloc_copys(&sendopt," -")) die_nomem();
  while ((opt = getopt(argc,argv,"cCmMrRt:T:vV")) != opteof)
    switch(opt) {	/* pass on ezmlm-send options */
      case 'c':			/* ezmlm-send flags */
      case 'C':
      case 'r':
      case 'R':
        szchar[0] = (char) opt & 0xff;
        if (!stralloc_append(&sendopt,szchar)) die_nomem();
        break;
      case 'm': flagmime = 1; break;
      case 'M': flagmime = 0; break;
      case 't':
      case 'T': if (optarg) replyto = optarg; break;
      case 'v':
      case 'V': strerr_die2x(0,"ezmlm-moderate version: ",auto_version);
      default:
	die_usage();
    }

  dir = argv[optind++];
  if (!dir) die_usage();

  sender = env_get("SENDER");
  if (!sender) strerr_die2x(100,FATAL,ERR_NOSENDER);
  local = env_get("LOCAL");
  if (!local) strerr_die2x(100,FATAL,ERR_NOLOCAL);
  def = env_get("DEFAULT");

  if (!*sender)
    strerr_die2x(100,FATAL,ERR_BOUNCE);
  if (!sender[str_chr(sender,'@')])
    strerr_die2x(100,FATAL,ERR_ANONYMOUS);
  if (str_equal(sender,"#@[]"))
    strerr_die2x(100,FATAL,ERR_BOUNCE);

  if (chdir(dir) == -1)
    strerr_die4sys(111,FATAL,ERR_SWITCH,dir,": ");

  switch(slurp("key",&key,32)) {
    case -1:
      strerr_die4sys(111,FATAL,ERR_READ,dir,"/key: ");
    case 0:
      strerr_die4x(100,FATAL,dir,"/key",ERR_NOEXIST);
  }
  getconf_line(&mailinglist,"mailinglist",1,FATAL,dir);
  getconf_line(&outhost,"outhost",1,FATAL,dir);
  getconf_line(&outlocal,"outlocal",1,FATAL,dir);
  set_cpoutlocal(&outlocal);	/* for copy() */
  set_cpouthost(&outhost);	/* for copy() */

  if (def) {			/* qmail>=1.02 */
	/* local should be >= def, but who knows ... */
    cp = local + str_len(local) - str_len(def) - 2;
    if (cp < local) die_badformat();
    action = local + byte_rchr(local,cp - local,'-');
    if (action == cp) die_badformat();
    action++;
  } else {			/* older versions of qmail */
    getconf_line(&inlocal,"inlocal",1,FATAL,dir);
    if (inlocal.len > str_len(local)) die_badaddr();
    if (case_diffb(inlocal.s,inlocal.len,local)) die_badaddr();
    action = local + inlocal.len;
    if (*(action++) != '-') die_badaddr();
  }

  if (!action[0]) die_badformat();
  if (!str_start(action,ACTION_CONFIRM) && !str_start(action,ACTION_DISCARD))
    die_badformat();
  start = str_chr(action,'-');
  if (!action[start]) die_badformat();
  confnum = 1 + start + str_chr(action + start + 1,'.');
  if (!action[confnum]) die_badformat();
  confnum += 1 + str_chr(action + confnum + 1,'.');
  if (!action[confnum]) die_badformat();
  if (!stralloc_copyb(&fnbase,action+start+1,confnum-start-1)) die_nomem();
  if (!stralloc_0(&fnbase)) die_nomem();
  cookie(hash,key.s,key.len,fnbase.s,"","a");
  if (byte_diff(hash,COOKIE,action+confnum+1))
    die_badformat();

  fdlock = open_append("mod/lock");
  if (fdlock == -1)
    strerr_die4sys(111,FATAL,ERR_OPEN,dir,"/mod/lock: ");
  if (lock_ex(fdlock) == -1)
    strerr_die4sys(111,FATAL,ERR_OBTAIN,dir,"/mod/lock: ");

  if (!checkfile(fnbase.s)) strerr_die2x(100,FATAL,ERR_MOD_TIMEOUT);
/* Here, we have an existing filename in fnbase with the complete path */
/* from the current dir in fnmsg. */

  if (str_start(action,ACTION_DISCARD)) {
        unlink(fnmsg.s);
        strerr_die1x(0,"ezmlm-confirm: info: post rejected");
  } else if (str_start(action,ACTION_CONFIRM)) {
    fd = open_read(fnmsg.s);
    if (fd == -1) {
      if (errno !=error_noent)
        strerr_die4sys(111,FATAL,ERR_OPEN,fnmsg.s,": ");
      else	/* shouldn't happen since we've got lock */
        strerr_die3x(100,FATAL,fnmsg.s,ERR_MOD_TIMEOUT);
    }

    substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));
				/* read "Return-Path:" line */
    if (getln(&sstext,&line,&match,'\n') == -1 || !match)
      strerr_die2sys(111,FATAL,ERR_READ_INPUT);
    maketo();			/* extract SENDER to "to" */
    env_put2("SENDER",to.s);	/* set SENDER */
    if (seek_begin(fd) == -1)	/* rewind, since we read an entire buffer */
      strerr_die4sys(111,FATAL,ERR_SEEK,fnmsg.s,": ");

    if ((child = wrap_fork(FATAL)) == 0) {
      close(0);
      dup(fd);	/* make fnmsg.s stdin */
      if (argc > optind)
        wrap_execsh(argv[optind], FATAL);
      else
        wrap_execbin("/ezmlm-send", &sendopt, dir, FATAL);
    }
    /* parent */
    close(fd);
    wrap_exitcode(child, FATAL);

    unlink(fnmsg.s);
    _exit(0);
  }
}
