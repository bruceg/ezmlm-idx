/*$Id: ezmlm-moderate.c,v 1.42 1999/10/09 16:49:56 lindberg Exp $*/
/*$Name: ezmlm-idx-040 $*/

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

int flagmime = MOD_MIME;	/* default is message as attachment */
char flagcd = '\0';		/* default: do not use transfer encoding */

#define FATAL "ezmlm-moderate: fatal: "
#define INFO "ezmlm-moderate: info: "

void die_usage() { strerr_die1x(100,
    "ezmlm-moderate: usage: ezmlm-moderate [-cCmMrRvV] [-t replyto] "
    "dir [/path/ezmlm-send]"); }

void die_nomem() { strerr_die2x(111,FATAL,ERR_NOMEM); }

void die_badformat() { strerr_die2x(100,FATAL,ERR_BAD_REQUEST); }

void die_badaddr() {
  strerr_die2x(100,FATAL,ERR_BAD_ADDRESS);
}

stralloc outhost = {0};
stralloc inlocal = {0};
stralloc outlocal = {0};
stralloc key = {0};
stralloc mydtline = {0};
stralloc mailinglist = {0};
stralloc accept = {0};
stralloc reject = {0};
stralloc to = {0};
stralloc send = {0};
stralloc sendopt = {0};
stralloc comment = {0};
stralloc charset = {0};
datetime_sec when;
struct datetime dt;

char strnum[FMT_ULONG];
char date[DATE822FMT];
char hash[COOKIE];
char boundary[COOKIE];
stralloc line = {0};
stralloc qline = {0};
stralloc text = {0};
stralloc quoted = {0};
stralloc fnbase = {0};
stralloc fnmsg = {0};
stralloc fnnew = {0};
stralloc fnsub = {0};
char subbuf[256];
substdio sssub;

char *dir;

struct stat st;

struct qmail qq;

void code_qput(s,n)
char *s;
unsigned int n;
{
    if (!flagcd)
      qmail_put(&qq,s,n);
    else {
      if (flagcd == 'B')
        encodeB(s,n,&qline,0,FATAL);
      else
        encodeQ(s,n,&qline,FATAL);
      qmail_put(&qq,qline.s,qline.len);
    }
}

void transferenc()
{
	if (flagcd) {
	  qmail_puts(&qq,"\nContent-Transfer-Encoding: ");
          if (flagcd == 'Q')
            qmail_puts(&qq,"Quoted-printable\n\n");
          else
	    qmail_puts(&qq,"base64\n\n");
        } else
          qmail_puts(&qq,"\n\n");
}

int checkfile(fn)
char *fn;
/* looks for DIR/mod/{pending|rejected|accept}/fn.*/
/* Returns:                                       */
/*          1 found in pending                    */
/*          0 not found                           */
/*         -1 found in accepted                   */
/*         -2 found in rejected                   */
/* Handles errors.                                */
/* ALSO: if found, fnmsg contains the o-terminated*/
/* file name.                                     */
{
  
  if (!stralloc_copys(&fnmsg,"mod/pending/")) die_nomem();
  if (!stralloc_cats(&fnmsg,fn)) die_nomem();
  if (!stralloc_0(&fnmsg)) die_nomem();
  if (stat(fnmsg.s,&st) == -1) {
    if (errno != error_noent)
      strerr_die6sys(111,FATAL,ERR_STAT,dir,"/",fnmsg.s,": ");
  } else
      return 1;

  if (!stralloc_copys(&fnmsg,"mod/accepted/")) die_nomem();
  if (!stralloc_cats(&fnmsg,fn)) die_nomem();
  if (!stralloc_0(&fnmsg)) die_nomem();
  if (stat(fnmsg.s,&st) == -1) {
    if (errno != error_noent)
      strerr_die6sys(111,FATAL,ERR_STAT,dir,"/",fnmsg.s,": ");
  } else
      return -1;

  if (!stralloc_copys(&fnmsg,"mod/rejected/")) die_nomem();
  if (!stralloc_cats(&fnmsg,fn)) die_nomem();
  if (!stralloc_0(&fnmsg)) die_nomem();
  if (stat(fnmsg.s,&st) == -1) {
    if (errno != error_noent)
      strerr_die6sys(111,FATAL,ERR_STAT,dir,"/",fnmsg.s,": ");
  } else
      return -2;
  return 0;
}

int qqwrite(fd,buf,len) int fd; char *buf; unsigned int len;
{
  qmail_put(&qq,buf,len);
  return len;
}
char qqbuf[1];
substdio ssqq = SUBSTDIO_FDBUF(qqwrite,-1,qqbuf,sizeof(qqbuf));

char inbuf[512];
substdio ssin = SUBSTDIO_FDBUF(read,0,inbuf,sizeof(inbuf));

substdio sstext;
char textbuf[1024];

void maketo()
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
        }		/* no return path-> no addressee. A NUL in the sender */
      }			/* is no worse than a faked sender, so no problem */
    }
}

void main(argc,argv)
int argc;
char **argv;
{
  char *sender;
  char *def;
  char *local;
  char *action;
  int flaginheader;
  int flagcomment;
  int flaggoodfield;
  int flagdone;
  int fd, fdlock;
  int match;
  char *err;
  char encin = '\0';
  char szchar[2] = "-";
  char *replyto = (char *) 0;
  unsigned int start,confnum;
  unsigned int pos,i;
  int child;
  int opt;
  char *sendargs[4];
  char *cp,*cpnext,*cpfirst,*cplast,*cpafter;
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
      case 'V': strerr_die2x(0,"ezmlm-moderate version: ",EZIDX_VERSION);
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
  if (!str_start(action,ACTION_ACCEPT) && !str_start(action,ACTION_REJECT))
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

  switch(checkfile(fnbase.s)) {
    case 0:
      strerr_die2x(100,FATAL,ERR_MOD_TIMEOUT);
    case -1:			/* only error if new request != action taken */
      if (str_start(action,ACTION_ACCEPT))
        strerr_die2x(0,INFO,ERR_MOD_ACCEPTED);
      else
        strerr_die2x(100,FATAL,ERR_MOD_ACCEPTED);
    case -2:
      if (str_start(action,ACTION_REJECT))
        strerr_die2x(0,INFO,ERR_MOD_REJECTED);
      else
        strerr_die2x(100,FATAL,ERR_MOD_REJECTED);
    default:
      break;
  }
/* Here, we have an existing filename in fnbase with the complete path */
/* from the current dir in fnmsg. */

  if (str_start(action,ACTION_REJECT)) {

    if (qmail_open(&qq, (stralloc *) 0) == -1)
      strerr_die2sys(111,FATAL,ERR_QMAIL_QUEUE);


				/* Build recipient from msg return-path */
    fd = open_read(fnmsg.s);
    if (fd == -1) {
      if (errno != error_noent)
        strerr_die4sys(111,FATAL,ERR_OPEN,fnmsg.s,": ");
      else
        strerr_die2x(100,FATAL,ERR_MOD_TIMEOUT);
    }
    substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));

    if (getln(&sstext,&line,&match,'\n') == -1 || !match)
      strerr_die2sys(111,FATAL,ERR_READ_INPUT);
    maketo();			/* extract SENDER from return-path */
						/* Build message */
    qmail_puts(&qq,"Mailing-List: ");
    qmail_put(&qq,mailinglist.s,mailinglist.len);
    if(getconf_line(&line,"listid",0,FATAL,dir)) {
      qmail_puts(&qq,"\nList-ID: ");
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
    if (!quote(&quoted,&outlocal)) die_nomem();
    qmail_put(&qq,quoted.s,quoted.len);
    qmail_puts(&qq,"-owner@");
    qmail_put(&qq,outhost.s,outhost.len);
    if (replyto) {
      qmail_puts(&qq,"\nReply-To: ");
      qmail_puts(&qq,replyto);
    }
    qmail_puts(&qq, "\nTo: ");
    qmail_puts(&qq,to.s);
    qmail_puts(&qq,"\nSubject: ");
    qmail_puts(&qq,TXT_RETURNED_POST);
    qmail_put(&qq,quoted.s,quoted.len);
    qmail_puts(&qq,"@");
    qmail_put(&qq,outhost.s,outhost.len);

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
      qmail_puts(&qq,"\n\n--");
      qmail_put(&qq,boundary,COOKIE);
      qmail_puts(&qq,"\nContent-Type: text/plain; charset=");
      qmail_puts(&qq,charset.s);
      transferenc();
    }
    copy(&qq,"text/top",flagcd,FATAL);
    copy(&qq,"text/mod-reject",flagcd,FATAL);

    flagcomment = 0;
    flaginheader = 1;
    if (!stralloc_copys(&text,"")) die_nomem();
    if (!stralloc_ready(&text,1024)) die_nomem(); 
    for (;;) {		/* copy moderator's rejection comment */
      if (getln(&ssin,&line,&match,'\n') == -1)
        strerr_die2sys(111,FATAL,ERR_READ_INPUT);
      if (!match) break;
      if (flaginheader) {
        if (case_startb(line.s,line.len,"Content-Transfer-Encoding:")) {
          pos = 26;
          while (line.s[pos] == ' ' || line.s[pos] == '\t') ++pos;
          if (case_startb(line.s+pos,line.len-pos,"base64"))
            encin = 'B';
          else if (case_startb(line.s+pos,line.len-pos,"quoted-printable"))
            encin = 'Q';
        }
        if (line.len == 1)
          flaginheader = 0;
      } else
        if (!stralloc_cat(&text,&line)) die_nomem();
    }	/* got body */
    if (encin) {
      if (encin == 'B')
        decodeB(text.s,text.len,&line,FATAL);
      else
        decodeQ(text.s,text.len,&line,FATAL);
      if (!stralloc_copy(&text,&line)) die_nomem();
    }
    cp = text.s;
    cpafter = text.s + text.len;
    if (!stralloc_copys(&line,"\n>>>>> -------------------- >>>>>\n"))
			die_nomem();
    flaggoodfield = 0;
    flagdone = 0;
    while ((cpnext = cp + byte_chr(cp,cpafter-cp,'\n')) != cpafter) {
      i = byte_chr(cp,cpnext-cp,'%');
      if (i <= 5 && cpnext-cp >= 8) {
				/* max 5 "quote characters" and space for %%% */
        if (cp[i+1] == '%' && cp[i+2] == '%') {
          if (!flaggoodfield) {					/* Start tag */
            if (!stralloc_copyb(&quoted,cp,i)) die_nomem();	/* quote chars*/
            flaggoodfield = 1;
            cp = cpnext + 1;
            cpfirst = cp;
            continue;
          } else {						/* end tag */
            if (flagdone)	/* 0 no comment lines, 1 comment line */
              flagdone = 2;	/* 2 at least 1 comment line & end tag */
            break;
          }
        }
      }
      if (flaggoodfield) {
        cplast = cpnext - 1;
        if (*cplast == '\r')	/* CRLF -> '\n' for base64 encoding */
          *cplast = '\n';
        else
          ++cplast;
			/* NUL is now ok, so the test for it was removed */
        flagdone = 1;
        i = cplast - cp + 1;
        if (quoted.len && quoted.len <= i &&
		!str_diffn(cp,quoted.s,quoted.len)) {	/* quote chars */
          if (!stralloc_catb(&line,cp+quoted.len,i-quoted.len)) die_nomem();
        } else
          if (!stralloc_catb(&line,cp,i)) die_nomem();	/* no quote chars */
      }
      cp = cpnext + 1;
    }
    if (flagdone == 2) {
    if (!stralloc_cats(&line,"<<<<< -------------------- <<<<<\n")) die_nomem();
      code_qput(line.s,line.len);
    }
    if (flagcd == 'B') {
      encodeB("",0,&line,2,FATAL);
      qmail_put(&qq,line.s,line.len);
    }
    if (flagmime) {
      qmail_puts(&qq,"\n--");
      qmail_put(&qq,boundary,COOKIE);
      qmail_puts(&qq,"\nContent-Type: message/rfc822\n\n");
    } else
      qmail_puts(&qq,"\n");
    if (seek_begin(fd) == -1)
      strerr_die4sys(111,FATAL,ERR_SEEK,fnmsg.s,": ");

    substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));
    if (substdio_copy(&ssqq,&sstext) != 0)
      strerr_die4sys(111,FATAL,ERR_READ,fnmsg.s,": ");
    close(fd);

    if (flagmime) {
      qmail_puts(&qq,"\n--");
      qmail_put(&qq,boundary,COOKIE);
      qmail_puts(&qq,"--\n");
    }

    if (!stralloc_copy(&line,&outlocal)) die_nomem();
    if (!stralloc_cats(&line,"-return-@")) die_nomem();
    if (!stralloc_cat(&line,&outhost)) die_nomem();
    if (!stralloc_0(&line)) die_nomem();
    qmail_from(&qq,line.s);
    if (to.len)
      qmail_to(&qq,to.s);

    if (!stralloc_copys(&fnnew,"mod/rejected/")) die_nomem();
    if (!stralloc_cats(&fnnew,fnbase.s)) die_nomem();
    if (!stralloc_0(&fnnew)) die_nomem();

/* this is strictly to track what happended to a message to give informative */
/* messages to the 2nd-nth moderator that acts on the same message. Since    */
/* this isn't vital we ignore errors. Also, it is no big ideal if unlinking  */
/* the old file fails. In the worst case it gets acted on again. If we issue */
/*  a temp error the reject will be redone, which is slightly worse.         */

    if (*(err = qmail_close(&qq)) == '\0') {
        fd = open_trunc(fnnew.s);
        if (fd != -1)
          close(fd);
        unlink(fnmsg.s);
        strnum[fmt_ulong(strnum,qmail_qp(&qq))] = 0;
        strerr_die2x(0,"ezmlm-moderate: info: qp ",strnum);
    } else
        strerr_die3x(111,FATAL,ERR_TMP_QMAIL_QUEUE,err + 1);

  } else if (str_start(action,ACTION_ACCEPT)) {
        fd = open_read(fnmsg.s);
        if (fd == -1)
          if (errno !=error_noent)
            strerr_die4sys(111,FATAL,ERR_OPEN,fnmsg.s,": ");
          else	/* shouldn't happen since we've got lock */
            strerr_die3x(100,FATAL,fnmsg.s,ERR_MOD_TIMEOUT);

    substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));
				/* read "Return-Path:" line */
    if (getln(&sstext,&line,&match,'\n') == -1 || !match)
      strerr_die2sys(111,FATAL,ERR_READ_INPUT);
    maketo();			/* extract SENDER to "to" */
    env_put2("SENDER",to.s);	/* set SENDER */
    if (seek_begin(fd) == -1)	/* rewind, since we read an entire buffer */
      strerr_die4sys(111,FATAL,ERR_SEEK,fnmsg.s,": ");

/* ##### NO REASON TO USE SH HERE ##### */
        sendargs[0] = "/bin/sh";
        sendargs[1] = "-c";
        if (argc > optind) {
          sendargs[2] = argv[optind];
        } else {
          if (!stralloc_copys(&send,auto_bin)) die_nomem();
          if (!stralloc_cats(&send,"/ezmlm-send")) die_nomem();
          if (sendopt.len > 2)
            if (!stralloc_cat(&send,&sendopt)) die_nomem();
          if (!stralloc_cats(&send," '")) die_nomem();
          if (!stralloc_cats(&send,dir)) die_nomem();
          if (!stralloc_cats(&send,"'")) die_nomem();
          if (!stralloc_0(&send)) die_nomem();
          sendargs[2] = send.s;
        }
        sendargs[3] = 0;

    switch(child = fork()) {
      case -1:
        strerr_die2sys(111,FATAL,ERR_FORK);
      case 0:		/* child */
        close(0);
        dup(fd);	/* make fnmsg.s stdin */
        execv(*sendargs,sendargs);
        if (errno == error_txtbsy || errno == error_nomem ||
            errno == error_io)
          strerr_die5sys(111,FATAL,ERR_EXECUTE,"/bin/sh -c ",sendargs[2],": ");
        else
          strerr_die5sys(100,FATAL,ERR_EXECUTE,"/bin/sh -c ",sendargs[2],": ");
       }
         /* parent */
      wait_pid(&wstat,child);
      close(fd);
      if (wait_crashed(wstat))
        strerr_die3x(111,FATAL,sendargs[2],ERR_CHILD_CRASHED);
      switch(wait_exitcode(wstat)) {
        case 100:
          strerr_die2x(100,FATAL,"Fatal error from child");
        case 111:
           strerr_die2x(111,FATAL,"Temporary error from child");
        case 0:
          break;
        default:
          strerr_die2x(111,FATAL,"Unknown temporary error from child");
      }
      if (!stralloc_copys(&fnnew,"mod/accepted/")) die_nomem();

      if (!stralloc_cats(&fnnew,fnbase.s)) die_nomem();
      if (!stralloc_0(&fnnew)) die_nomem();
/* ignore errors */
      fd = open_trunc(fnnew.s);
      if (fd != -1)
        close(fd);
      unlink(fnmsg.s);
      _exit(0);
   }
}
