/*$Id$*/

#include <sys/types.h>
#include <sys/stat.h>
#include "stralloc.h"
#include "subfd.h"
#include "strerr.h"
#include "error.h"
#include "qmail.h"
#include "env.h"
#include "lock.h"
#include "sig.h"
#include "open.h"
#include "getln.h"
#include "str.h"
#include "fmt.h"
#include "readwrite.h"
#include "auto_bin.h"
#include "fork.h"
#include "wait.h"
#include "exit.h"
#include "substdio.h"
#include "getconf.h"
#include "datetime.h"
#include "now.h"
#include "date822fmt.h"
#include "cookie.h"
#include "sgetopt.h"
#include "errtxt.h"
#include "idx.h"
#include "copy.h"
#include "subscribe.h"
#include "mime.h"

int flagmime = MOD_MIME;	/* default is message as attachment */
int flagpublic = 1;		/* default anyone can post */
				/* =0 for only moderators can */
int flagself = 0;		/* `modpost` mods approve own posts */
				/* but mod/ is used for moderators */
				/* of other posts. Def=no=0 */
char flagcd = '\0';		/* default: don't use quoted-printable */
int flagbody = 1;		/* body of message enclosed with mod request */
				/* 0 => headers only */

#define FATAL "ezmlm-store: fatal: "

void die_usage()
{
  strerr_die1x(100,"ezmlm-store: usage: ezmlm-store [-cCmMpPrRsSvV] dir");
}
void die_nomem() { strerr_die2x(111,FATAL,ERR_NOMEM); }

stralloc fnmsg = {0};

void die_msg() { strerr_die4sys(111,FATAL,ERR_WRITE,fnmsg.s,": "); }

int fdmsg;
int fdmod;
int pid;
int match;

char strnum[FMT_ULONG];
char date[DATE822FMT];
char hash[COOKIE];
char boundary[COOKIE];
datetime_sec when;
struct datetime dt;
struct stat st;

void *psql = (void *) 0;

stralloc fnbase = {0};
stralloc line = {0};
stralloc mailinglist = {0};
stralloc outlocal = {0};
stralloc outhost = {0};
stralloc mydtline = {0};
stralloc returnpath = {0};
stralloc accept = {0};
stralloc action = {0};
stralloc reject = {0};
stralloc quoted = {0};
stralloc key = {0};
stralloc subject = {0};
stralloc moderators = {0};
stralloc charset = {0};
stralloc sendopt = {0};

struct qmail qq;
int qqwrite(fd,buf,len) int fd; char *buf; unsigned int len;
{
  qmail_put(&qq,buf,len);
  return len;
}

int subto(s,l)
char *s;
unsigned int l;
{
  qmail_put(&qq,"T",1);
  qmail_put(&qq,s,l);
  qmail_put(&qq,"",1);
  return (int) l;
}

char qqbuf[1];
substdio ssqq = SUBSTDIO_FDBUF(qqwrite,-1,qqbuf,sizeof(qqbuf));

substdio ssin;
char inbuf[1024];

substdio ssmsg;
char msgbuf[1024];

substdio sstext;
char textbuf[512];

substdio sssub;
char subbuf[512];

void transferenc()
{
	if (flagcd) {
	  qmail_puts(&qq,"\nContent-Transfer-Encoding: ");
          if (flagcd == 'Q')
            qmail_puts(&qq,"Quoted-Printable\n\n");
          else
	    qmail_puts(&qq,"base64\n\n");
        } else
          qmail_puts(&qq,"\n\n");
}

void makehash(act)
stralloc *act;					/* has to be 0-terminated  */
/* act is expected to be -reject-ddddd.ttttt or -accept-ddddd.ttttt        */
/* The routine will add .hash@outhost to act. act will NOT be 0-terminated */
{
  int d;

  d = 2 + str_chr(act->s + 1,'-');
  cookie(hash,key.s,key.len,act->s + d,"","a");
  *(act->s + act->len - 1) = '.';	/* we put a '.' Bad, but works */
  if (!stralloc_catb(act,hash,COOKIE)) die_nomem();
  if (!stralloc_cats(act,"@")) die_nomem();
  if (!stralloc_cat(act,&outhost)) die_nomem();
}

void main(argc,argv)
int argc;
char **argv;
{
  char *dir;
  int fdlock;
  char *sender;
  int match;
  int flaginheader;
  int flagmodpost;
  int flagremote;
  char *pmod;
  char *err;
  int opt;
  unsigned int i;
  char szchar[2] = "-";
  char *sendargs[4];
  int child,wstat;

  (void) umask(022);
  sig_pipeignore();

  if (!stralloc_copys(&sendopt," -")) die_nomem();
  while ((opt = getopt(argc,argv,"bBcCmMpPrRsSvV")) != opteof)
    switch(opt) {
      case 'b': flagbody = 1; break;
      case 'B': flagbody = 0; break;
      case 'm': flagmime = 1; break;
      case 'M': flagmime = 0; break;
      case 'p': flagpublic = 1; break;	/* anyone can post (still moderated)*/
      case 'P': flagpublic = 0; break;	/* only moderators can post */
      case 's': flagself = 1; break;	/* modpost and DIR/mod diff fxns */
      case 'S': flagself = 0; break;	/* same fxn */
      case 'c':				/* ezmlm-send flags */
      case 'C':
      case 'r':
      case 'R':
        szchar[0] = (char) opt & 0xff;
        if (!stralloc_append(&sendopt,szchar)) die_nomem();
        break;
      case 'v':
      case 'V': strerr_die2x(0,"ezmlm-store version: ",EZIDX_VERSION);
      default:
	die_usage();
    }

  sender = env_get("SENDER");

  if (sender) {
    if (!*sender || str_equal(sender,"#@[]"))
      strerr_die2x(100,FATAL,ERR_BOUNCE);
  }

  dir = argv[optind];
  if (!dir) die_usage();

  if (chdir(dir) == -1)
    strerr_die4sys(111,FATAL,ERR_SWITCH,dir,": ");

  flagmodpost = getconf_line(&moderators,"modpost",0,FATAL,dir);
  flagremote = getconf_line(&line,"remote",0,FATAL,dir);
  if (!flagmodpost) {			/* not msg-mod. Pipe to ezmlm-send */
    sendargs[0] = "/bin/sh";
    sendargs[1] = "-c";
    if (!stralloc_copys(&line,auto_bin)) die_nomem();
    if (!stralloc_cats(&line,"/ezmlm-send")) die_nomem();
    if (sendopt.len > 2)
      if (!stralloc_cat(&line,&sendopt)) die_nomem();
    if (!stralloc_cats(&line," '")) die_nomem();
    if (!stralloc_cats(&line,dir)) die_nomem();
    if (!stralloc_cats(&line,"'")) die_nomem();
    if (!stralloc_0(&line)) die_nomem();
    sendargs[2] = line.s;
    sendargs[3] = 0;
    switch(child = fork()) {
      case -1:
        strerr_die2sys(111,FATAL,ERR_FORK);
      case 0:
        execvp(*sendargs,sendargs);
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
        strerr_die2x(100,FATAL,"Fatal error from child");
      case 111:
         strerr_die2x(111,FATAL,"Temporary error from child");
      case 0:
        _exit(0);
      default:
        strerr_die2x(111,FATAL,"Unknown temporary error from child");
    }
  }

  if (!moderators.len || !(moderators.s[0] == '/')) {
    if (!stralloc_copys(&moderators,dir)) die_nomem();
    if (!stralloc_cats(&moderators,"/mod")) die_nomem();
  }
  if (!stralloc_0(&moderators)) die_nomem();

  if (sender) {
      pmod = issub(moderators.s,sender,(char *) 0,FATAL);
      closesql();
				/* sender = moderator? */
  } else
    pmod = 0;

  if (!pmod && !flagpublic)
    strerr_die2x(100,FATAL,ERR_NO_POST);

  switch(slurp("key",&key,32)) {
    case -1:
      strerr_die4sys(111,FATAL,ERR_READ,dir,"/key: ");
    case 0:
      strerr_die4x(100,FATAL,dir,"/key",ERR_NOEXIST);
  }

  getconf_line(&outhost,"outhost",1,FATAL,dir);
  getconf_line(&outlocal,"outlocal",1,FATAL,dir);
  getconf_line(&mailinglist,"mailinglist",1,FATAL,dir);

  fdlock = open_append("mod/lock");
  if (fdlock == -1)
    strerr_die4sys(111,FATAL,ERR_OPEN,dir,"/mod/lock: ");
  if (lock_ex(fdlock) == -1)
    strerr_die4sys(111,FATAL,ERR_OBTAIN,dir,"/mod/lock: ");

  if (!stralloc_copys(&mydtline,"Delivered-To: moderator for ")) die_nomem();
  if (!stralloc_catb(&mydtline,outlocal.s,outlocal.len)) die_nomem();
  if (!stralloc_append(&mydtline,"@")) die_nomem();
  if (!stralloc_catb(&mydtline,outhost.s,outhost.len)) die_nomem();
  if (!stralloc_cats(&mydtline,"\n")) die_nomem();

  if (!stralloc_copys(&returnpath,"Return-Path: <")) die_nomem();
  if (sender) {
    if (!stralloc_cats(&returnpath,sender)) die_nomem();
    for (i = 14; i < returnpath.len;++i)
      if (returnpath.s[i] == '\n' || !returnpath.s[i] )
        returnpath.s[i] = '_';
		/* NUL and '\n' are bad, but we don't quote since this is */
		/* only for ezmlm-moderate, NOT for SMTP */
  }
  if (!stralloc_cats(&returnpath,">\n")) die_nomem();

 pid = getpid();		/* unique file name */
 for (i = 0;;++i)		/* got lock - nobody else can add files */
  {
   when = now();		/* when is also used later for date! */
   if (!stralloc_copys(&fnmsg,"mod/pending/")) die_nomem();
   if (!stralloc_copyb(&fnbase,strnum,fmt_ulong(strnum,when))) die_nomem();
   if (!stralloc_append(&fnbase,".")) die_nomem();
   if (!stralloc_catb(&fnbase,strnum,fmt_ulong(strnum,pid))) die_nomem();
   if (!stralloc_cat(&fnmsg,&fnbase)) die_nomem();
   if (!stralloc_0(&fnmsg)) die_nomem();
   if (stat(fnmsg.s,&st) == -1) if (errno == error_noent) break;
   /* really should never get to this point */
   if (i == 2)
     strerr_die2x(111,FATAL,ERR_UNIQUE);
   sleep(2);
  }

  if (!stralloc_copys(&action,"-")) die_nomem();
  if (!stralloc_cats(&action,ACTION_REJECT)) die_nomem();
  if (!stralloc_cat(&action,&fnbase)) die_nomem();
  if (!stralloc_0(&action)) die_nomem();
  makehash(&action);
  if (!quote(&quoted,&outlocal)) die_nomem();
  if (!stralloc_copy(&reject,&quoted)) die_nomem();
  if (!stralloc_cat(&reject,&action)) die_nomem();
  if (!stralloc_0(&reject)) die_nomem();

  if (!stralloc_copys(&action,"-")) die_nomem();
  if (!stralloc_cats(&action,ACTION_ACCEPT)) die_nomem();
  if (!stralloc_cat(&action,&fnbase)) die_nomem();
  if (!stralloc_0(&action)) die_nomem();
  makehash(&action);
  if (!stralloc_copy(&accept,&quoted)) die_nomem();
  if (!stralloc_cat(&accept,&action)) die_nomem();
  if (!stralloc_0(&accept)) die_nomem();

  set_cpoutlocal(&outlocal);
  set_cpouthost(&outhost);
  set_cptarget(accept.s);	/* for copy () */
  set_cpconfirm(reject.s);

  fdmsg = open_trunc(fnmsg.s);
  if (fdmsg == -1)
    strerr_die6sys(111,FATAL,ERR_WRITE,dir,"/",fnmsg.s,": ");
  substdio_fdbuf(&ssmsg,write,fdmsg,msgbuf,sizeof(msgbuf));

  if (qmail_open(&qq, (stralloc *) 0) == -1)		/* Open mailer */
    strerr_die2sys(111,FATAL,ERR_QMAIL_QUEUE);

  qmail_puts(&qq,"Mailing-List: ");
  qmail_put(&qq,mailinglist.s,mailinglist.len);
  if (getconf_line(&line,"listid",0,FATAL,dir)) {
    qmail_puts(&qq,"List-ID: ");
    qmail_put(&qq,line.s,line.len);
  }
  qmail_puts(&qq,"\nDate: ");
  datetime_tai(&dt,when);
  qmail_put(&qq,date,date822fmt(date,&dt));
  qmail_puts(&qq,"Message-ID: <");
  if (!stralloc_copyb(&line,strnum,fmt_ulong(strnum,(unsigned long) when)))
     die_nomem();
  if (!stralloc_append(&line,".")) die_nomem();
  if (!stralloc_catb(&line,strnum,
		fmt_ulong(strnum,(unsigned long) getpid()))) die_nomem();
  if (!stralloc_cats(&line,".ezmlm@")) die_nomem();
  if (!stralloc_cat(&line,&outhost)) die_nomem();
  if (!stralloc_0(&line)) die_nomem();
  qmail_puts(&qq,line.s);
		/* "unique" MIME boundary as hash of messageid */
  cookie(boundary,"",0,"",line.s,"");
  qmail_puts(&qq,">\nFrom: ");
  qmail_puts(&qq,reject.s);
  qmail_puts(&qq,"\nReply-To: ");
  qmail_puts(&qq,accept.s);
  if (!pmod && flagremote) {	/* if remote admin add -allow- address */
    qmail_puts(&qq,"\nCc: ");	/* for ezmlm-gate users */
    strnum[fmt_ulong(strnum,(unsigned long) when)] = 0;
    cookie(hash,key.s,key.len-FLD_ALLOW,strnum,sender,"t");
    if (!stralloc_copy(&line,&outlocal)) die_nomem();
    if (!stralloc_cats(&line,"-allow-tc.")) die_nomem();
    if (!stralloc_cats(&line,strnum)) die_nomem();
    if (!stralloc_append(&line,".")) die_nomem();
    if (!stralloc_catb(&line,hash,COOKIE)) die_nomem();
    if (!stralloc_append(&line,"-")) die_nomem();
    i = str_rchr(sender,'@');
    if (!stralloc_catb(&line,sender,i)) die_nomem();
    if (sender[i]) {
      if (!stralloc_append(&line,"=")) die_nomem();
      if (!stralloc_cats(&line,sender + i + 1)) die_nomem();
    }
    qmail_put(&qq,line.s,line.len);
    qmail_puts(&qq,"@");
    qmail_put(&qq,outhost.s,outhost.len);
  }
  qmail_puts(&qq,"\nTo: Recipient list not shown: ;");
  if (!stralloc_copys(&subject,"\nSubject: ")) die_nomem();
  if (!stralloc_cats(&subject,TXT_MODERATE)) die_nomem();
  if (!quote(&quoted,&outlocal)) die_nomem();
  if (!stralloc_cat(&subject,&quoted)) die_nomem();
  if (!stralloc_append(&subject,"@")) die_nomem();
  if (!stralloc_cat(&subject,&outhost)) die_nomem();
  if (flagmime) {
    if (getconf_line(&charset,"charset",0,FATAL,dir)) {
      if (charset.len >= 2 && charset.s[charset.len - 2] == ':') {
        if (charset.s[charset.len - 1] == 'B' ||
		charset.s[charset.len - 1] == 'Q') {
          flagcd = charset.s[charset.len - 1];
          charset.s[charset.len - 2] = '\0';
        }
      }
    } else
      if (!stralloc_copys(&charset,TXT_DEF_CHARSET)) die_nomem();
    if (!stralloc_0(&charset)) die_nomem();
    qmail_puts(&qq,"\nMIME-Version: 1.0\n");
    qmail_puts(&qq,"Content-Type: multipart/mixed;\n\tboundary=");
    qmail_put(&qq,boundary,COOKIE);
    qmail_put(&qq,subject.s,subject.len);
    qmail_puts(&qq,"\n\n--");
    qmail_put(&qq,boundary,COOKIE);
    qmail_puts(&qq,"\nContent-Type: text/plain; charset=");
    qmail_puts(&qq,charset.s);
    transferenc();
  } else {
    qmail_put(&qq,subject.s,subject.len);
    qmail_puts(&qq,"\n\n");
  }
  copy(&qq,"text/mod-request",flagcd,FATAL);
  if (flagcd == 'B') {
    encodeB("",0,&line,2,FATAL);
    qmail_put(&qq,line.s,line.len);
  }
  if (substdio_put(&ssmsg,returnpath.s,returnpath.len) == -1) die_msg();
  if (substdio_put(&ssmsg,mydtline.s,mydtline.len) == -1) die_msg();
  substdio_fdbuf(&ssin,read,0,inbuf,sizeof(inbuf));

  if (flagmime) {
    qmail_puts(&qq,"\n--");
    qmail_put(&qq,boundary,COOKIE);
    qmail_puts(&qq,"\nContent-Type: message/rfc822\n\n");
  }

  qmail_put(&qq,returnpath.s,returnpath.len);
  qmail_put(&qq,mydtline.s,mydtline.len);
  flaginheader = 1;
  for (;;) {
    if (getln(&ssin,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,ERR_READ_INPUT);
    if (!match) break;
    if (line.len == 1) flaginheader = 0;
    if (flaginheader) {
      if ((line.len == mydtline.len) &&
		!byte_diff(line.s,line.len,mydtline.s)) {
	close(fdmsg);			/* be nice - clean up */
	unlink(fnmsg.s);
        strerr_die2x(100,FATAL,ERR_LOOPING);
      }
      if (case_startb(line.s,line.len,"mailing-list:")) {
	close(fdmsg);			/* be nice - clean up */
	unlink(fnmsg.s);
        strerr_die2x(100,FATAL,ERR_MAILING_LIST);
      }
    }

    if (flagbody || flaginheader)	/* skip body if !flagbody */
      qmail_put(&qq,line.s,line.len);
    if (substdio_put(&ssmsg,line.s,line.len) == -1) die_msg();
  }

  if (flagmime) {
    qmail_puts(&qq,"\n--");
    qmail_put(&qq,boundary,COOKIE);
    qmail_puts(&qq,"--\n");
  }

/* close archive before qmail. Loss of qmail will result in re-run, and   */
/* worst case this results in a duplicate msg sitting orphaned until it's */
/* cleaned out.                                                           */

  if (substdio_flush(&ssmsg) == -1) die_msg();
  if (fsync(fdmsg) == -1) die_msg();
  if (fchmod(fdmsg,MODE_MOD_MSG | 0700) == -1) die_msg();
  if (close(fdmsg) == -1) die_msg(); /* NFS stupidity */

  close(fdlock);

  if (!stralloc_copy(&line,&outlocal)) die_nomem();
  if (!stralloc_cats(&line,"-return-@")) die_nomem();
  if (!stralloc_cat(&line,&outhost)) die_nomem();
  if (!stralloc_0(&line)) die_nomem();
  qmail_from(&qq,line.s);			/* envelope sender */
  if (pmod)					/* to moderator only */
    qmail_to(&qq,pmod);
  else {
    if (flagself) {				/* to all moderators */
      if (!stralloc_copys(&moderators,dir)) die_nomem();
      if (!stralloc_cats(&moderators,"/mod")) die_nomem();
      if (!stralloc_0(&moderators)) die_nomem();
    }
    putsubs(moderators.s,0,52,subto,1,FATAL);
  }

  if (*(err = qmail_close(&qq)) == '\0') {
      strnum[fmt_ulong(strnum,qmail_qp(&qq))] = 0;
      strerr_die2x(0,"ezmlm-store: info: qp ",strnum);
  } else
      strerr_die3x(111,FATAL,ERR_TMP_QMAIL_QUEUE,err+1);
}
