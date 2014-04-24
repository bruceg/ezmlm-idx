#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "stralloc.h"
#include "subfd.h"
#include "strerr.h"
#include "error.h"
#include "qmail.h"
#include "env.h"
#include "sender.h"
#include "lock.h"
#include "sig.h"
#include "open.h"
#include "getln.h"
#include "str.h"
#include "fmt.h"
#include "readwrite.h"
#include "wait.h"
#include "exit.h"
#include "substdio.h"
#include "getconf.h"
#include "datetime.h"
#include "now.h"
#include "cookie.h"
#include "getconfopt.h"
#include "messages.h"
#include "byte.h"
#include "case.h"
#include "quote.h"
#include "hdr.h"
#include "die.h"
#include "idx.h"
#include "copy.h"
#include "subdb.h"
#include "mime.h"
#include "wrap.h"
#include "config.h"
#include "auto_version.h"

static int flagmime = MOD_MIME;		/* default is message as attachment */
static int flagmodpostonly = -1;	/* default anyone can post */
					/* =1 for only moderators can */
static int flagself = 0;		/* `modpost` mods approve own posts */
					/* but mod/ is used for moderators */
					/* of other posts. Def=no=0 */
static int flagconfirm = -1;           /* if true, sender must approve its own posts */
static int flagbody = 1;		/* body of message enclosed with mod request */
					/* 0 => headers only */

const char FATAL[] = "ezmlm-store: fatal: ";
const char USAGE[] =
"ezmlm-store: usage: ezmlm-store [-cCmMpPrRsSvV] dir";

static stralloc sendopt = {0};

static struct option options[] = {
  OPT_FLAG(flagbody,'b',1,0),
  OPT_FLAG(flagbody,'B',0,0),
  OPT_FLAG(flagmime,'m',1,0),
  OPT_FLAG(flagmime,'M',0,0),
  OPT_FLAG(flagmodpostonly,'p',0,0), /* anyone can post (still mod'd) */
  OPT_FLAG(flagmodpostonly,'P',1,"modpostonly"), /* only moderators can post */
  OPT_FLAG(flagself,'s',1,0),	     /* modpost and DIR/mod diff fxns */
  OPT_FLAG(flagself,'S',0,0),	     /* same fxn */
  OPT_FLAG(flagconfirm,'y',1,"confirmpost"), /* force post confirmation */
  OPT_FLAG(flagconfirm,'Y',0,0),     /* disable post confirmation */
  OPT_COPY_FLAG(sendopt,'c'),
  OPT_COPY_FLAG(sendopt,'C'),
  OPT_COPY_FLAG(sendopt,'r'),
  OPT_COPY_FLAG(sendopt,'R'),
  OPT_END
};

static stralloc fnmsg = {0};

static void die_msg(void) { strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnmsg.s)); }

char boundary[COOKIE];
stralloc line = {0};
stralloc quoted = {0};

struct qmail qq;

static int subto(const char *s,unsigned int l)
{
  qmail_put(&qq,"T",1);
  qmail_put(&qq,s,l);
  qmail_put(&qq,"",1);
  return (int) l;
}

static void makeacthash(stralloc *act)
/* act is expected to be -reject-ddddd.ttttt or -accept-ddddd.ttttt and
 * has to be 0-terminated. */
/* The routine will add .hash@outhost to act. act will NOT be 0-terminated */
{
  char hash[COOKIE];
  int d;

  d = 2 + str_chr(act->s + 1,'-');
  cookie(hash,key.s,key.len,act->s + d,"","a");
  *(act->s + act->len - 1) = '.';	/* we put a '.' Bad, but works */
  if (!stralloc_catb(act,hash,COOKIE)) die_nomem();
  if (!stralloc_cats(act,"@")) die_nomem();
  if (!stralloc_cat(act,&outhost)) die_nomem();
  set_cphash(hash);
}

static substdio ssin;
static char inbuf[1024];

static substdio ssmsg;
static char msgbuf[1024];

int main(int argc,char **argv)
{
  char strnum[FMT_ULONG];
  datetime_sec when;
  struct stat st;

  stralloc fnbase = {0};
  stralloc mydtline = {0};
  stralloc returnpath = {0};
  stralloc accept = {0};
  stralloc action = {0};
  stralloc reject = {0};
  stralloc moderators = {0};

  char hash[COOKIE];
  const char *dir;
  int fdlock;
  const char *sender;
  int match;
  int flaginheader;
  int flagmodpost;
  int flagremote;
  int ismod;
  stralloc mod = {0};
  const char *err;
  unsigned int i;
  int fdmsg;
  int pid;

  (void) umask(022);
  sig_pipeignore();

  sender = get_sender();

  if (sender) {
    if (!*sender || str_equal(sender,"#@[]"))
      strerr_die2x(100,FATAL,MSG(ERR_BOUNCE));
  }

  if (!stralloc_copys(&sendopt,"-")) die_nomem();
  getconfopt(argc,argv,options,1,&dir);
  initsub(0);

  flagmodpost = getconf_line(&moderators,"modpost",0);
  flagremote = getconf_isset("remote");
  if (!flagmodpost && !flagconfirm) /* not msg-mod. Exec ezmlm-send */
    wrap_execbin("/ezmlm-send", &sendopt, dir);

  if (!moderators.len) {
    if (!stralloc_copys(&moderators,"mod")) die_nomem();
  }
  if (!stralloc_0(&moderators)) die_nomem();

  if (sender) {
    ismod = issub(moderators.s,sender,&mod);
    closesub();
				/* sender = moderator? */
  } else
    ismod = 0;

  if (!ismod && flagmodpostonly)
    strerr_die2x(100,FATAL,MSG(ERR_NO_POST));

  fdlock = lockfile("mod/lock");

  if (!stralloc_copys(&mydtline, flagconfirm
    ? "Delivered-To: confirm to "
    : "Delivered-To: moderator for ")) die_nomem();
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
   if (!stralloc_copys(&fnmsg, flagconfirm?"mod/unconfirmed/":"mod/pending/")) die_nomem();
   if (!stralloc_copyb(&fnbase,strnum,fmt_ulong(strnum,when))) die_nomem();
   if (!stralloc_append(&fnbase,".")) die_nomem();
   if (!stralloc_catb(&fnbase,strnum,fmt_ulong(strnum,pid))) die_nomem();
   if (!stralloc_cat(&fnmsg,&fnbase)) die_nomem();
   if (!stralloc_0(&fnmsg)) die_nomem();
   if (stat(fnmsg.s,&st) == -1) if (errno == error_noent) break;
   /* really should never get to this point */
   if (i == 2)
     strerr_die2x(111,FATAL,MSG(ERR_UNIQUE));
   sleep(2);
  }

  if (!stralloc_copys(&action,"-")) die_nomem();
  if (!stralloc_cats(&action,flagconfirm?ACTION_DISCARD:ACTION_REJECT)) die_nomem();
  if (!stralloc_cat(&action,&fnbase)) die_nomem();
  if (!stralloc_0(&action)) die_nomem();
  makeacthash(&action);
  if (!quote(&quoted,&outlocal)) die_nomem();
  if (!stralloc_copy(&reject,&quoted)) die_nomem();
  if (!stralloc_cat(&reject,&action)) die_nomem();
  if (!stralloc_0(&reject)) die_nomem();

  if (!stralloc_copys(&action,"-")) die_nomem();
  if (!stralloc_cats(&action,flagconfirm?ACTION_CONFIRM:ACTION_ACCEPT)) die_nomem();
  if (!stralloc_cat(&action,&fnbase)) die_nomem();
  if (!stralloc_0(&action)) die_nomem();
  makeacthash(&action);
  if (!stralloc_copy(&accept,&quoted)) die_nomem();
  if (!stralloc_cat(&accept,&action)) die_nomem();
  if (!stralloc_0(&accept)) die_nomem();

  set_cptarget(accept.s);	/* for copy () */
  set_cpaction(flagconfirm?ACTION_CONFIRM:ACTION_ACCEPT);
  set_cpconfirm(reject.s,quoted.len);
  set_cpwhen(when);

  fdmsg = open_trunc(fnmsg.s);
  if (fdmsg == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnmsg.s));
  substdio_fdbuf(&ssmsg,write,fdmsg,msgbuf,sizeof(msgbuf));

  if (qmail_open(&qq) == -1)		/* Open mailer */
    strerr_die2sys(111,FATAL,MSG(ERR_QMAIL_QUEUE));

  hdr_add2s("Mailing-List: ",MSG(TXT_MAILING_LIST));
  if (listid.len > 0)
    hdr_add2("List-ID: ",listid.s,listid.len);
  hdr_datemsgid(when);
  if (flagconfirm)
    hdr_from("-owner");
  else
    hdr_add2s("From: ",reject.s);
  hdr_add2s("Reply-To: ",accept.s);
  if (!flagconfirm && !ismod && flagremote) {	/* if remote admin add -allow- address */
    qmail_puts(&qq,"Cc: ");	/* for ezmlm-gate users */
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
    qmail_puts(&qq,"\n");
  }
  qmail_puts(&qq,"To: <");
  if (flagconfirm) {
    if (sender)
      qmail_puts(&qq, sender);
  } else {
    if (!quote(&quoted,&outlocal))
      die_nomem();
    qmail_put(&qq,quoted.s,quoted.len);
    qmail_puts(&qq,"-moderators@");
    qmail_put(&qq,outhost.s,outhost.len);
  }
  qmail_puts(&qq,">\n");
  /* FIXME: Drop the custom subject hack and use hdr_listsubject1 */
  hdr_subject(flagconfirm ? MSG(SUB_CONFIRM_POST) : MSG(SUB_MODERATE));
  if (flagmime) {
    hdr_mime(CTYPE_MULTIPART);
    hdr_boundary(0);
    hdr_ctype(CTYPE_TEXT);
    hdr_transferenc();
  } else
    qmail_puts(&qq,"\n\n");
  copy(&qq,flagconfirm?"text/post-confirm":"text/mod-request",flagcd);
  if (flagcd == 'B') {
    encodeB("",0,&line,2);
    qmail_put(&qq,line.s,line.len);
  }
  if (substdio_put(&ssmsg,returnpath.s,returnpath.len) == -1) die_msg();
  if (substdio_put(&ssmsg,mydtline.s,mydtline.len) == -1) die_msg();
  substdio_fdbuf(&ssin,read,0,inbuf,sizeof(inbuf));

  if (flagmime) {
    hdr_boundary(0);
    hdr_ctype(CTYPE_MESSAGE);
    qmail_puts(&qq, "\n");
  }

  qmail_put(&qq,returnpath.s,returnpath.len);
  qmail_put(&qq,mydtline.s,mydtline.len);
  flaginheader = 1;
  for (;;) {
    if (getln(&ssin,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,MSG(ERR_READ_INPUT));
    if (!match) break;
    if (line.len == 1) flaginheader = 0;
    if (flaginheader) {
      if ((line.len == mydtline.len) &&
		!byte_diff(line.s,line.len,mydtline.s)) {
	close(fdmsg);			/* be nice - clean up */
	unlink(fnmsg.s);
        strerr_die2x(100,FATAL,MSG(ERR_LOOPING));
      }
      if (case_startb(line.s,line.len,"mailing-list:")) {
	close(fdmsg);			/* be nice - clean up */
	unlink(fnmsg.s);
        strerr_die2x(100,FATAL,MSG(ERR_MAILING_LIST));
      }
    }

    if (flagbody || flaginheader)	/* skip body if !flagbody */
      qmail_put(&qq,line.s,line.len);
    if (substdio_put(&ssmsg,line.s,line.len) == -1) die_msg();
  }

  if (flagmime)
    hdr_boundary(1);

/* close archive before qmail. Loss of qmail will result in re-run, and   */
/* worst case this results in a duplicate msg sitting orphaned until it's */
/* cleaned out.                                                           */

  if (substdio_flush(&ssmsg) == -1) die_msg();
  if (fsync(fdmsg) == -1) die_msg();
  if (fchmod(fdmsg,MODE_MOD_MSG | 0700) == -1) die_msg();
  if (close(fdmsg) == -1) die_msg(); /* NFS stupidity */

  close(fdlock);

  if (flagconfirm) {
    qmail_from(&qq,reject.s);			/* envelope sender */
  } else {
    if (!stralloc_copy(&line,&outlocal)) die_nomem();
    if (!stralloc_cats(&line,"-return-@")) die_nomem();
    if (!stralloc_cat(&line,&outhost)) die_nomem();
    if (!stralloc_0(&line)) die_nomem();
    qmail_from(&qq,line.s);			/* envelope sender */
  }
  if (flagconfirm)				/* to sender */
    qmail_to(&qq,sender);
  else if (ismod)				/* to moderator only */
    qmail_to(&qq,mod.s);
  else {
    if (flagself) {				/* to all moderators */
      if (!stralloc_copys(&moderators,"mod")) die_nomem();
      if (!stralloc_0(&moderators)) die_nomem();
    }
    putsubs(moderators.s,0,52,subto);
  }

  if (*(err = qmail_close(&qq)) == '\0') {
      strnum[fmt_ulong(strnum,qmail_qp(&qq))] = 0;
      strerr_die2x(0,"ezmlm-store: info: qp ",strnum);
  } else
      strerr_die4x(111,FATAL,MSG(ERR_TMP_QMAIL_QUEUE),": ",err+1);
}
