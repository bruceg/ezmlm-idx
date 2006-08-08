/*$Id$*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "substdio.h"
#include "subfd.h"
#include "stralloc.h"
#include "strerr.h"
#include "error.h"
#include "env.h"
#include "sig.h"
#include "str.h"
#include "seek.h"
#include "wait.h"
#include "exit.h"
#include "getconf.h"
#include "auto_bin.h"
#include "sgetopt.h"
#include "errtxt.h"
#include "die.h"
#include "idx.h"
#include "subscribe.h"
#include "wrap.h"
#include "config.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-gate: fatal: ";
const char USAGE[] =
"ezmlm-gate: usage: ezmlm-gate [-cCmMpPqrRsSvV] dir [moddir [...]]";

stralloc line = {0};
stralloc cmds = {0};
stralloc sendopt = {0};
stralloc storeopt = {0};

char szchar[2] = "-";
  int child;
  const char *pmod;

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
        strerr_die2x(111,FATAL,ERR_CHILD_TEMP);
      default:
        strerr_die2x(100,FATAL,ERR_REJECT);	/* other errors => bounce */
    }
    if (seek_begin(0) == -1)			/* rewind */
      strerr_die2sys(111,FATAL,ERR_SEEK_INPUT);
    return r;
}

void main(int argc,char **argv)
{
  char *dir;
  char *sender;
  char *moddir;
  const char *queryext = (char *) 0;
  int opt;
  int ret = 0;
  int dontact = 0;
  unsigned int i,j;
  const char *program;
  stralloc *opts;

  umask(022);
  sig_pipeignore();
	/* storeopts to ezmlm-store only. Others to both (ezmlm-store may */
	/* pass them on to ezmlm-send. */
  if (!stralloc_copys(&sendopt,"-")) die_nomem();
  if (!stralloc_copys(&storeopt,"-")) die_nomem();

  while ((opt = getopt(argc,argv,
      "0cCmMpPq:Q:sSrRt:T:vVyY")) != opteof)
    switch(opt) {	/* pass on unrecognized options */
      case 'c':			/* ezmlm-send flags */
      case 'C':
      case 'r':
      case 'Q':
      case 'R':
        szchar[0] = opt;
        if (!stralloc_append(&sendopt,szchar)) die_nomem();
        if (!stralloc_append(&storeopt,szchar)) die_nomem();
        break;
      case 'm':			/* ezmlm-store flags */
      case 'M':
      case 'p':
      case 'P':
      case 's':
      case 'S':
      case 'y':
      case 'Y':
        szchar[0] = opt;
        if (!stralloc_append(&storeopt,szchar)) die_nomem();
        break;
      case 'q': if (optarg) queryext = optarg; break;
      case 'v':
      case 'V': strerr_die2x(0,"ezmlm-gate version: ",auto_version);
      case '0': dontact = 1; break;
      default:			/* ezmlm-store flags */
        die_usage();
    }

  startup(dir = argv[optind++]);

  sender = env_get("SENDER");

  pmod = (char *) 0;

  if (queryext) {
    getconf(&cmds,queryext,1,dir);
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
      pmod = "";			/* 0, 99 => post */
					/* other => moderate */
  }
  moddir = argv[optind++];
  if (moddir && !ret) {			/* if exit 0 and moddir, add issub */
    pmod = (char *) 0;
    while (moddir && !pmod && sender) {
      pmod = (moddir[0] == '/')
	? issub(moddir,0,sender)
	: issub(dir,moddir,sender);
      closesub();
      moddir = argv[optind++];
    }
  }

  if (pmod) {
    program = "/ezmlm-send";
    opts = &sendopt;
  }
  else {
    program = "/ezmlm-store";
    opts = &storeopt;
  }

  if (dontact) {
    substdio_puts(subfderr, auto_bin);
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
