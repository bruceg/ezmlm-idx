#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "env.h"
#include "sender.h"
#include "strerr.h"
#include "getconfopt.h"
#include "substdio.h"
#include "subfd.h"
#include "messages.h"
#include "error.h"
#include "byte.h"
#include "fmt.h"
#include "str.h"
#include "stralloc.h"
#include "qmail.h"
#include "seek.h"
#include "wrap.h"
#include "slurp.h"
#include "die.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-dispatch: fatal: ";
const char USAGE[] =
"ezmlm-dispatch: usage: ezmlm-dispatch dir [list]";

static struct option options[] = {
  OPT_END
};

static const char *sender;
static stralloc basedir;
static stralloc path;

static struct qmail qq;
static char strnum[FMT_ULONG];
static int did_program;
static int did_forward;

static const char *makepath(const char *fn)
{
  if (!stralloc_copy(&path,&basedir)) die_nomem();
  if (fn) {
    if (!stralloc_append(&path,"/")) die_nomem();
    if (!stralloc_cats(&path,fn)) die_nomem();
  }
  if (!stralloc_0(&path)) die_nomem();
  return path.s;
}

static int is_dir(const char *fn)
{
  struct stat st;
  return wrap_stat(makepath(fn),&st) == 0
    && S_ISDIR(st.st_mode);
}

static int is_file(const char *fn)
{
  struct stat st;
  return wrap_stat(makepath(fn),&st) == 0
    && S_ISREG(st.st_mode);
}

static void forward(const char *rcpt)
{
  char buf[4096];
  const char *dtline;
  const char *err;
  int r;
  if (qmail_open(&qq) == -1)
    strerr_die2sys(111,FATAL,MSG(ERR_QMAIL_QUEUE));
  if ((dtline = env_get("DTLINE")) != 0)
    qmail_puts(&qq,dtline);
  while ((r = read(0,buf,sizeof buf)) > 0)
    qmail_put(&qq,buf,r);
  if (r == -1)
    strerr_die2sys(111,FATAL,MSG(ERR_READ_STDIN));
  qmail_from(&qq,sender);
  qmail_to(&qq,rcpt);
  if (*(err = qmail_close(&qq)) != '\0')
    strerr_die4x(111,FATAL,MSG(ERR_TMP_QMAIL_QUEUE),": ",err + 1);
  strnum[fmt_ulong(strnum,qmail_qp(&qq))] = 0;
  substdio_puts(subfderr,"qp ");
  substdio_puts(subfderr,strnum);
  substdio_putsflush(subfderr,"\n");
  ++did_forward;
}

static int execute_line(const char *fn,const char *line)
{
  int child;
  int code;
  if (seek_begin(0) == -1)
    strerr_die2sys(111,FATAL,MSG(ERR_SEEK_INPUT));
  while (*line
	 && (*line == '\t'
	     || *line == ' '))
      ++line;
  switch (*line) {
  case '#':
    /* ignore comments */
    return 0;
  case '|':
    if ((child = wrap_fork()) == 0)
      wrap_execsh(line+1);
    code = wrap_waitpid(child);
    ++did_program;
    return code;
  case '/':
  case '.':
    strerr_die5x(100,FATAL,basedir.s,"/",fn,": Delivery to files is not supported.");
  default:
    if (*line == '&')
      ++line;
    forward(line);
    return 0;
  }
}

static int execute_file(const char *fn,stralloc *file)
{
  unsigned int start;
  unsigned int end;
  unsigned int len;
  int code;
  for (start = 0; start < file->len; start = end + 1) {
    len = byte_chr(file->s+start,file->len-start,'\n');
    end = start + len;
    file->s[end] = 0;
    if ((code = execute_line(fn,file->s+start)) != 0)
      return code;
  }
  return 0;
}

static void execute(const char *fn,const char *def)
     /* Load and execute a qmail command file. */
{
  stralloc file = {0,0,0};
  int code;
  if (def != 0)
    env_put2("DEFAULT",def);
  else
    env_unset("DEFAULT");
  fn = makepath(fn);
  if (slurp(fn,&file,256) != 1)
    strerr_die2sys(111,FATAL,MSG1(ERR_READ,fn));
  code = execute_file(fn,&file);
  substdio_puts(subfderr,"did 0+");
  substdio_put(subfderr,strnum,fmt_ulong(strnum,did_forward));
  substdio_puts(subfderr,"+");
  substdio_put(subfderr,strnum,fmt_ulong(strnum,did_program));
  substdio_putsflush(subfderr,"\n");
  _exit(code);
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
  if (!is_dir(dir))
    return;
  if (!stralloc_append(&basedir,"/")) die_nomem();
  if (!stralloc_cats(&basedir,dir)) die_nomem();
  /* FIXME: set up $EXT $EXT2 $EXT3 $EXT4 ?  Is it feasable? */
  if (def == 0 || *def == 0)
    execute("editor",0);
  else if (str_diff(def,"owner") == 0)
    execute("owner",0);
  else if (str_diff(def,"digest-owner") == 0)
    execute("owner",0);
  else {
    try_dispatch(def,"digest-return-",14,"digest/bouncer");
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
  int dash;
  
  optind = getconfopt(argc,argv,options,0,0);
  if (argv[optind] == 0)
    die_usage();
  if (!stralloc_copys(&basedir,argv[optind++])) die_nomem();

  sender = get_sender();
  if (!sender)
    die_sender();
  def = env_get("DEFAULT");

  if (argv[optind] != 0) {
    dispatch(argv[optind],def);
    strerr_die3x(100,FATAL,"Not a directory: ",path.s);
  }
  else if (!def || !*def)
    strerr_die2x(100,FATAL,MSG(ERR_NODEFAULT));
  else {
    if (def[str_chr(def,'/')] != 0)
      strerr_die2x(100,FATAL,"Recipient address may not contain '/'");

    dispatch(def,0);

    dash = str_len(def);
    for (;;) {
      while (--dash > 0)
	if (def[dash] == '-')
	  break;
      if (dash <= 0)
	break;
      def[dash] = 0;
      dispatch(def,def+dash+1);
      def[dash] = '-';
    }
    strerr_die3x(100,FATAL,"Could not match recipient name to any list: ",def);
  }
}
