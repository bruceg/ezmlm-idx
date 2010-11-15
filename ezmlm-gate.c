#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "substdio.h"
#include "subfd.h"
#include "stralloc.h"
#include "strerr.h"
#include "error.h"
#include "env.h"
#include "sender.h"
#include "sig.h"
#include "str.h"
#include "seek.h"
#include "wait.h"
#include "exit.h"
#include "getconf.h"
#include "auto_bin.h"
#include "getconfopt.h"
#include "messages.h"
#include "die.h"
#include "idx.h"
#include "subdb.h"
#include "wrap.h"
#include "config.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-gate: fatal: ";
const char USAGE[] =
"ezmlm-gate: usage: ezmlm-gate [-cCmMpPqrRsSvV] dir [moddir [...]]";

static const char *queryext = (char *) 0;
static int dontact = 0;
static stralloc sendopt = {0};
static stralloc storeopt = {0};

static struct option options[] = {
  OPT_COPY_FLAG(sendopt,'c'),
  OPT_COPY_FLAG(sendopt,'C'),
  OPT_COPY_FLAG(sendopt,'r'),
  OPT_COPY_FLAG(sendopt,'R'),
  OPT_COPY_FLAG(sendopt,'Q'),
  OPT_COPY_FLAG(storeopt,'m'),
  OPT_COPY_FLAG(storeopt,'M'),
  OPT_COPY_FLAG(storeopt,'p'),
  OPT_COPY_FLAG(storeopt,'P'),
  OPT_COPY_FLAG(storeopt,'s'),
  OPT_COPY_FLAG(storeopt,'S'),
  OPT_COPY_FLAG(storeopt,'y'),
  OPT_COPY_FLAG(storeopt,'Y'),
  OPT_CSTR(queryext,'q',0),
  OPT_FLAG(dontact,'0',1,0),
  OPT_END
};

stralloc line = {0};
stralloc cmds = {0};

char szchar[2] = "-";
  int child;
int ismod;

int mailprog(const char *s)
{
    int r;

    if ((child = wrap_fork()) == 0)
      wrap_execsh(s);
    /* parent */
    switch((r = wrap_waitpid(child))) {
      /* 100 perm error, 111 temp, 99 dom ok */
      /* 0 rec ok, others bounce */
      case 0: case 99: case 100: break;
      case 111:					/* temp error */
        strerr_die2x(111,FATAL,MSG(ERR_CHILD_TEMP));
      default:
        strerr_die2x(100,FATAL,MSG(ERR_REJECT));	/* other errors => bounce */
    }
    if (seek_begin(0) == -1)			/* rewind */
      strerr_die2sys(111,FATAL,MSG(ERR_SEEK_INPUT));
    return r;
}

int main(int argc,char **argv)
{
  const char *dir;
  const char *sender;
  const char *moddir;
  int opt;
  int ret = 0;
  unsigned int i,j;
  const char *program;
  stralloc *opts;

  umask(022);
  sig_pipeignore();

  if (!stralloc_copys(&sendopt,"-")) die_nomem();
  if (!stralloc_copys(&storeopt,"-")) die_nomem();
  opt = getconfopt(argc,argv,options,1,&dir);

	/* storeopts to ezmlm-store only. Others to both (ezmlm-store may */
	/* pass them on to ezmlm-send. */
  if (!stralloc_catb(&storeopt,sendopt.s+1,sendopt.len-1)) die_nomem();

  initsub(0);

  sender = get_sender();

  ismod = 0;

  if (queryext) {
    getconf(&cmds,queryext,1);
    i = 0;
    for (j = 0;j < cmds.len; ++j)
      if (!cmds.s[j]) {
	switch (cmds.s[i]) {
	  case '\0': case '#': break;	/* ignore blank/comment */
	  case '|':
		ret = mailprog(cmds.s + i + 1); break;
	  default:
		ret = mailprog(cmds.s + i); break;
	}
	if (ret) break;
	i = j + 1;
    }
    if (!ret || ret == 99)		/* 111 => temp error */
      ismod = 1;			/* 0, 99 => post */
					/* other => moderate */
  }
  moddir = argv[opt++];
  if (moddir && !ret) {			/* if exit 0 and moddir, add issub */
    ismod = 0;
    while (moddir && !ismod && sender) {
      ismod = issub(moddir,sender,0);
      closesub();
      moddir = argv[opt++];
    }
  }

  if (ismod) {
    program = "/ezmlm-send";
    opts = &sendopt;
  }
  else {
    program = "/ezmlm-store";
    opts = &storeopt;
  }

  if (dontact) {
    substdio_puts(subfderr, auto_bin());
    substdio_puts(subfderr, program);
    substdio_put(subfderr, " ", 1);
    substdio_put(subfderr, opts->s, opts->len);
    substdio_put(subfderr, " ", 1);
    substdio_puts(subfderr, dir);
    substdio_putsflush(subfderr, "\n");
    _exit(0);
  }

  if ((child = wrap_fork()) == 0)
    wrap_execbin(program, opts, dir);
  /* parent */
  wrap_exitcode(child);
  _exit(0);
}
