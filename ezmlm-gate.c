/*$Id$*/

#include "substdio.h"
#include "subfd.h"
#include "stralloc.h"
#include "strerr.h"
#include "error.h"
#include "env.h"
#include "sig.h"
#include "str.h"
#include "seek.h"
#include "fork.h"
#include "wait.h"
#include "exit.h"
#include "getconf.h"
#include "auto_bin.h"
#include "sgetopt.h"
#include "errtxt.h"
#include "idx.h"
#include "subscribe.h"

#define FATAL "ezmlm-gate: fatal: "

void die_usage()
{
  strerr_die1x(100,"ezmlm-gate: usage: ezmlm-gate [-cCmMpPqrRsSvV] "
			"dir [moddir [...]]");
}
void die_nomem() { strerr_die2x(111,FATAL,ERR_NOMEM); }

stralloc line = {0};
stralloc cmds = {0};
stralloc send = {0};
stralloc sendopt = {0};
stralloc storeopt = {0};
void *psql = (void *) 0;

char szchar[2] = "-";
  char *sendargs[4];
  int child;
  int wstat;
  char *pmod;

int mailprog(s)
  char *s;
{
    int r;

    sendargs[0] = "/bin/sh";	/* 100 perm error, 111 temp, 99 dom ok */
    sendargs[1] = "-c";		/* 0 rec ok, others bounce */
    sendargs[2] = s;
    sendargs[3] = (char *)0;
    switch(child = fork()) {
      case -1:
	strerr_die2sys(111,FATAL,ERR_FORK);
      case 0:
	execv(*sendargs,sendargs);
	if (errno == error_txtbsy || errno == error_nomem ||
	  errno == error_io)
	        strerr_die5sys(111,FATAL,ERR_EXECUTE,
		  "/bin/sh -c ",sendargs[2],": ");
        else
		strerr_die5sys(100,FATAL,ERR_EXECUTE,
		  "/bin/sh -c ",sendargs[2],": ");
    }
         /* parent */
    wait_pid(&wstat,child);
    if (wait_crashed(wstat))
	strerr_die2x(111,FATAL,ERR_CHILD_CRASHED);
    switch((r = wait_exitcode(wstat))) {
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

void main(argc,argv)
int argc;
char **argv;
{
  char *dir;
  char *sender;
  char *moddir;
  char *queryext = (char *) 0;
  int opt;
  int ret = 0;
  int dontact = 0;
  unsigned int i,j,k;

  umask(022);
  sig_pipeignore();
	/* storeopts to ezmlm-store only. Others to both (ezmlm-store may */
	/* pass them on to ezmlm-send. */
  if (!stralloc_copys(&sendopt," -")) die_nomem();
  if (!stralloc_copys(&storeopt," -")) die_nomem();

  while ((opt = getopt(argc,argv,
      "0cCmMpPq:Q:sSrRt:T:vV")) != opteof)
    switch(opt) {	/* pass on unrecognized options */
      case 'c':			/* ezmlm-send flags */
      case 'C':
      case 'r':
      case 'Q':
      case 'R':
        szchar[0] = opt;
        if (!stralloc_append(&sendopt,szchar)) die_nomem();
        break;
      case 'm':			/* ezmlm-store flags */
      case 'M':
      case 'p':
      case 'P':
      case 's':
      case 'S':
        szchar[0] = opt;
        if (!stralloc_append(&storeopt,szchar)) die_nomem();
        break;
      case 'q': if (optarg) queryext = optarg; break;
      case 'v':
      case 'V': strerr_die2x(0,"ezmlm-gate version: ",EZIDX_VERSION);
      case '0': dontact = 1; break;
      default:			/* ezmlm-store flags */
        die_usage();
    }

  dir = argv[optind++];
  if (!dir) die_usage();
  if (chdir(dir) == -1)
    strerr_die4sys(111,FATAL,ERR_SWITCH,dir,": ");

  sender = env_get("SENDER");

  pmod = (char *) 0;

  if (queryext) {
    getconf(&cmds,queryext,1,FATAL,dir);
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
      pmod = issub(moddir,sender,(char *) 0,FATAL);
      closesql();
      moddir = argv[optind++];
    }
  }

  sendargs[0] = "/bin/sh";
  sendargs[1] = "-c";
  if (!stralloc_copys(&send,auto_bin)) die_nomem();
  if (pmod) {
    if (!stralloc_cats(&send,"/ezmlm-send")) die_nomem();
    if (sendopt.len > 2)
      if (!stralloc_cat(&send,&sendopt)) die_nomem();

  } else {
    if (!stralloc_cats(&send,"/ezmlm-store")) die_nomem();
    if (storeopt.len > 2)
      if (!stralloc_cat(&send,&storeopt)) die_nomem();
    if (sendopt.len > 2)
      if (!stralloc_cat(&send,&sendopt)) die_nomem();
  }
  if (!stralloc_cats(&send," '")) die_nomem();
  if (!stralloc_cats(&send,dir)) die_nomem();
  if (!stralloc_cats(&send,"'")) die_nomem();
  if (!stralloc_0(&send)) die_nomem();
  sendargs[2] = send.s;
  sendargs[3] = 0;

  if (dontact) {
    char **s;
    for (s = sendargs; *s; s++) {
      substdio_puts(subfderr, *s);
      if (s[1]) substdio_puts(subfderr, " ");
    }
    substdio_putsflush(subfderr, "\n");
    return;
  }

  switch(child = fork()) {
    case -1:
      strerr_die2sys(111,FATAL,ERR_FORK);
    case 0:
      execv(*sendargs,sendargs);
      if (errno == error_txtbsy || errno == error_nomem ||
          errno == error_io)
        strerr_die5sys(111,FATAL,ERR_EXECUTE,"/bin/sh -c ",sendargs[2],": ");
      else
        strerr_die5sys(100,FATAL,ERR_EXECUTE,"/bin/sh -c ",sendargs[2],": ");
   }
         /* parent */
   wait_pid(&wstat,child);
   if (wait_crashed(wstat))
     strerr_die2x(111,FATAL,ERR_CHILD_CRASHED);
   switch(wait_exitcode(wstat)) {
     case 100:
       strerr_die2x(100,FATAL,ERR_CHILD_FATAL);
     case 111:
        strerr_die2x(111,FATAL,ERR_CHILD_TEMP);
     case 0:
       _exit(0);
     default:
       strerr_die2x(111,FATAL,ERR_CHILD_UNKNOWN);
   }
}

