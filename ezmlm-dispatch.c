/*$Id$*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "env.h"
#include "strerr.h"
#include "sgetopt.h"
#include "substdio.h"
#include "subfd.h"
#include "errtxt.h"
#include "error.h"
#include "byte.h"
#include "fmt.h"
#include "str.h"
#include "stralloc.h"
#include "qmail.h"
#include "seek.h"
#include "wrap.h"
#include "slurp.h"
#include "auto_version.h"

#define FATAL "ezmlm-dispatch: fatal: "

void die_usage(void)
{
  strerr_die1x(100,"ezmlm-dispatch: usage: ezmlm-dispatch dir [list]");
}

void die_nomem(void)
{
  strerr_die2x(111,FATAL,ERR_NOMEM);
}

static const char *sender;
static const char *basedir;
static const char *listdir;

static struct qmail qq;
static stralloc path;
static char strnum[FMT_ULONG];
static int did_program;
static int did_forward;

static void make_path(const char *fn)
{
  if (!stralloc_copys(&path,basedir)) die_nomem();
  if (listdir) {
    if (!stralloc_cats(&path,"/")) die_nomem();
    if (!stralloc_cats(&path,listdir)) die_nomem();
  }
  if (fn) {
    if (!stralloc_cats(&path,"/")) die_nomem();
    if (!stralloc_cats(&path,fn)) die_nomem();
  }
  if (!stralloc_0(&path)) die_nomem();
}

static int is_dir(const char *fn)
{
  struct stat st;
  make_path(fn);
  return wrap_stat(path.s,&st,FATAL) == 0
    && S_ISDIR(st.st_mode);
}

static int is_file(const char *fn)
{
  struct stat st;
  make_path(fn);
  return wrap_stat(path.s,&st,FATAL) == 0
    && S_ISREG(st.st_mode);
}

static void forward(const char *rcpt)
{
  char buf[4096];
  const char *dtline;
  const char *err;
  int r;
  if (qmail_open(&qq,0) == -1)
    strerr_die2sys(111,FATAL,ERR_QMAIL_QUEUE);
  if ((dtline = env_get("DTLINE")) != 0)
    qmail_puts(&qq,dtline);
  while ((r = read(0,buf,sizeof buf)) > 0)
    qmail_put(&qq,buf,r);
  if (r == -1)
    strerr_die3sys(111,FATAL,ERR_READ,": ");
  qmail_from(&qq,sender);
  qmail_to(&qq,rcpt);
  if (*(err = qmail_close(&qq)) != '\0')
    strerr_die3x(111,FATAL,ERR_TMP_QMAIL_QUEUE,err + 1);
  strnum[fmt_ulong(strnum,qmail_qp(&qq))] = 0;
  substdio_puts(subfderr,"qp ");
  substdio_puts(subfderr,strnum);
  substdio_putsflush(subfderr,"\n");
  ++did_forward;
}

static void execute_line(const char *line)
{
  int child;
  int code;
  if (seek_begin(0) == -1)
    strerr_die2sys(111,FATAL,ERR_SEEK_INPUT);
  while (*line
	 && (*line == '\t'
	     || *line == ' '))
      ++line;
  switch (*line) {
  case '#':
    /* ignore comments */
    break;
  case '|':
    if ((child = wrap_fork(FATAL)) == 0)
      wrap_execsh(line+1,FATAL);
    if ((code = wrap_waitpid(child,FATAL)) != 0)
      _exit(code);
    ++did_program;
    break;
  case '/':
  case '.':
    strerr_die3x(100,FATAL,path.s,": Delivery to files is not supported.");
    break;
  default:
    if (*line == '&')
      ++line;
    forward(line);
  }
}

static void execute_file(stralloc *file)
{
  int start;
  int end;
  int len;
  for (start = 0; start < file->len; start = end + 1) {
    len = byte_chr(file->s+start,file->len-start,'\n');
    end = start + len;
    file->s[end] = 0;
    execute_line(file->s+start);
  }
}

static void execute(const char *fn,const char *def)
     /* Load and execute a qmail command file. */
{
  stralloc file = {0,0,0};
  if (def != 0)
    env_put2("DEFAULT",def);
  else
    env_unset("DEFAULT");
  make_path(fn);
  if (slurp(path.s,&file,256) != 1)
    strerr_die4sys(111,FATAL,ERR_READ_INPUT,path.s,": ");
  execute_file(&file);
  substdio_puts(subfderr,"did 0+");
  substdio_put(subfderr,strnum,fmt_ulong(strnum,did_forward));
  substdio_puts(subfderr,"+");
  substdio_put(subfderr,strnum,fmt_ulong(strnum,did_program));
  substdio_putsflush(subfderr,"\n");
  _exit(0);
}

static void try_dispatch(const char *def,const char *prefix,unsigned int len,
			 const char *fn)
     /* Test if def has a certain prefix, and if so execute the
      * associated file. */
{
  if (str_diffn(def,prefix,len) == 0
      && is_file(fn))
    execute(fn,def+len);
}

static void dispatch(const char *dir,const char *def)
     /* Hand off control to one of the command files in the list directory. */
{
  listdir = dir;
  /* FIXME: set up $EXT $EXT2 $EXT3 $EXT4 ?  Is it feasable? */
  if (def == 0)
    execute("editor",0);
  else if (str_diff(def,"owner") == 0)
    execute("owner",0);
  else if (str_diff(def,"digest-owner") == 0)
    execute("owner",0);
  else {
    try_dispatch(def,"digest-return-",14,"bouncer");
    try_dispatch(def,"return-",7,"bouncer");
    try_dispatch(def,"confirm-",8,"confirmer");
    try_dispatch(def,"discard-",8,"confirmer");
    try_dispatch(def,"accept-",7,"moderator");
    try_dispatch(def,"reject-",7,"moderator");
    /* If nothing else matches, call the manager. */
    execute("manager",def);
  }
}

void main(int argc,char **argv)
{
  char *def;
  int opt;
  int dash;
  
  while ((opt = getopt(argc,argv,"vV")) != opteof)
    switch (opt) {
    case 'v':
    case 'V':
      strerr_die2x(0, "ezmlm-dispatch version: ",auto_version);
    default:
      die_usage();
    }

  if ((basedir = argv[optind++]) == 0)
    die_usage();
  if (!is_dir(0))
    strerr_die3x(100,FATAL,"Not a directory: ",basedir);

  sender = env_get("SENDER");
  if (!sender)
    strerr_die2x(100,FATAL,ERR_NOSENDER);
  def = env_get("DEFAULT");
  if (!def || !*def)
    strerr_die2x(100,FATAL,ERR_NODEFAULT);

  if ((listdir = argv[optind++]) != 0) {
    if (!is_dir(0))
      strerr_die3x(100,FATAL,"Not a directory: ",path.s);
    dispatch(def,def);
  }
  else {
    if (def[str_chr(def,'/')] != 0)
      strerr_die2x(100,FATAL,"Recipient address may not contain '/'");

    if (is_dir(def))
      dispatch(def,0);

    dash = str_len(def);
    for (;;) {
      while (--dash > 0)
	if (def[dash] == '-')
	  break;
      if (dash <= 0)
	break;
      def[dash] = 0;
      if (is_dir(def))
	dispatch(def,def+dash+1);
      def[dash] = '-';
    }
    strerr_die3x(100,FATAL,"Could not match recipient name to any list: ",def);
  }
}
