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
#include "wait.h"
#include "strerr.h"
#include "byte.h"
#include "getln.h"
#include "qmail.h"
#include "substdio.h"
#include "subfd.h"
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
#include "hdr.h"
#include "mime.h"
#include "open.h"
#include "lock.h"
#include "die.h"
#include "idx.h"
#include "wrap.h"
#include "config.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-moderate: fatal: ";
const char INFO[] = "ezmlm-moderate: info: ";
const char USAGE[] =
"ezmlm-moderate: usage: ezmlm-moderate [-cCmMrRvV] [-t replyto] dir [/path/ezmlm-send]";

static const char *replyto = (char *) 0;
static int flagmime = MOD_MIME;	/* default is message as attachment */

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

char boundary[COOKIE];
stralloc line = {0};
stralloc qline = {0};
stralloc quoted = {0};
static stralloc fnmsg = {0};

struct qmail qq;

static void code_qput(const char *s,unsigned int n)
{
    if (!flagcd)
      qmail_put(&qq,s,n);
    else {
      if (flagcd == 'B')
        encodeB(s,n,&qline,0);
      else
        encodeQ(s,n,&qline);
      qmail_put(&qq,qline.s,qline.len);
    }
}

static int checkfile(const char *fn)
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
  struct stat st;
  
  stralloc_copys(&fnmsg,"mod/pending/");
  stralloc_cats(&fnmsg,fn);
  stralloc_0(&fnmsg);
  if (stat(fnmsg.s,&st) == -1) {
    if (errno != error_noent)
      strerr_die2sys(111,FATAL,MSG1(ERR_STAT,fnmsg.s));
  } else
      return 1;

  stralloc_copys(&fnmsg,"mod/accepted/");
  stralloc_cats(&fnmsg,fn);
  stralloc_0(&fnmsg);
  if (stat(fnmsg.s,&st) == -1) {
    if (errno != error_noent)
      strerr_die2sys(111,FATAL,MSG1(ERR_STAT,fnmsg.s));
  } else
      return -1;

  stralloc_copys(&fnmsg,"mod/rejected/");
  stralloc_cats(&fnmsg,fn);
  stralloc_0(&fnmsg);
  if (stat(fnmsg.s,&st) == -1) {
    if (errno != error_noent)
      strerr_die2sys(111,FATAL,MSG1(ERR_STAT,fnmsg.s));
  } else
      return -2;
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
        }		/* no return path-> no addressee. A NUL in the sender */
      }			/* is no worse than a faked sender, so no problem */
    }
}

int main(int argc,char **argv)
{
  const char *sender;
  const char *def;
  char *local;
  const char *action;
  int flaginheader;
  int flaggoodfield;
  int flagdone;
  int fd;
  int match;
  const char *err;
  char encin = '\0';
  unsigned int start,confnum;
  unsigned int pos,i;
  int child;
  int opt;
  char *cp,*cpnext,*cplast,*cpafter;
  char strnum[FMT_ULONG];
  char hash[COOKIE];
  substdio sstext;
  char textbuf[1024];
  datetime_sec when;
  stralloc text = {0};
  stralloc fnbase = {0};
  stralloc fnnew = {0};
  const char *dir;

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
  if (!str_start(action,ACTION_ACCEPT) && !str_start(action,ACTION_REJECT))
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

  lockfile("mod/lock");

  switch(checkfile(fnbase.s)) {
    case 0:
      strerr_die2x(100,FATAL,MSG(ERR_MOD_TIMEOUT));
    case -1:			/* only error if new request != action taken */
      if (str_start(action,ACTION_ACCEPT))
        strerr_die2x(0,INFO,MSG(ERR_MOD_ACCEPTED));
      else
        strerr_die2x(100,FATAL,MSG(ERR_MOD_ACCEPTED));
    case -2:
      if (str_start(action,ACTION_REJECT))
        strerr_die2x(0,INFO,MSG(ERR_MOD_REJECTED));
      else
        strerr_die2x(100,FATAL,MSG(ERR_MOD_REJECTED));
    default:
      break;
  }
/* Here, we have an existing filename in fnbase with the complete path */
/* from the current dir in fnmsg. */

  if (str_start(action,ACTION_REJECT)) {

    if (qmail_open(&qq) == -1)
      strerr_die2sys(111,FATAL,MSG(ERR_QMAIL_QUEUE));


				/* Build recipient from msg return-path */
    fd = open_read(fnmsg.s);
    if (fd == -1) {
      if (errno != error_noent)
        strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,fnmsg.s));
      else
        strerr_die2x(100,FATAL,MSG(ERR_MOD_TIMEOUT));
    }
    substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));

    if (getln(&sstext,&line,&match,'\n') == -1 || !match)
      strerr_die2sys(111,FATAL,MSG(ERR_READ_INPUT));
    maketo();			/* extract SENDER from return-path */
						/* Build message */
    hdr_add2s("Mailing-List: ",MSG(TXT_MAILING_LIST));
    if (listid.len > 0)
      hdr_add2("List-ID: ",listid.s,listid.len);
    hdr_datemsgid(when);
    hdr_from("-owner");
    if (replyto)
      hdr_add2s("Reply-To: ",replyto);
    hdr_add2s("To: ",to.s);
    hdr_subject(MSG(SUB_RETURNED_POST));

    if (flagmime) {
      hdr_mime(CTYPE_MULTIPART);
      hdr_boundary(0);
      hdr_ctype(CTYPE_TEXT);
      hdr_transferenc();
    }
    copy(&qq,"text/top",flagcd);
    copy(&qq,"text/mod-reject",flagcd);

    flaginheader = 1;
    stralloc_copys(&text,"");
    stralloc_ready(&text,1024); 
    for (;;) {		/* copy moderator's rejection comment */
      if (getln(subfdin,&line,&match,'\n') == -1)
        strerr_die2sys(111,FATAL,MSG(ERR_READ_INPUT));
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
        stralloc_cat(&text,&line);
    }	/* got body */
    if (encin) {
      if (encin == 'B')
        decodeB(text.s,text.len,&line);
      else
        decodeQ(text.s,text.len,&line);
      stralloc_copy(&text,&line);
    }
    cp = text.s;
    cpafter = text.s + text.len;
    stralloc_copys(&line,"\n>>>>> -------------------- >>>>>\n");
    flaggoodfield = 0;
    flagdone = 0;
    while ((cpnext = cp + byte_chr(cp,cpafter-cp,'\n')) != cpafter) {
      i = byte_chr(cp,cpnext-cp,'%');
      if (i <= 5 && cpnext-cp-i >= 3) {
				/* max 5 "quote characters" and space for %%% */
        if (cp[i+1] == '%' && cp[i+2] == '%') {
          if (!flaggoodfield) {					/* Start tag */
            stralloc_copyb(&quoted,cp,i);	/* quote chars*/
            flaggoodfield = 1;
            cp = cpnext + 1;
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
          stralloc_catb(&line,cp+quoted.len,i-quoted.len);
        } else
          stralloc_catb(&line,cp,i);	/* no quote chars */
      }
      cp = cpnext + 1;
    }
    if (flagdone == 2) {
    stralloc_cats(&line,"<<<<< -------------------- <<<<<\n");
      code_qput(line.s,line.len);
    }
    if (flagcd == 'B') {
      encodeB("",0,&line,2);
      qmail_put(&qq,line.s,line.len);
    }
    if (flagmime) {
      hdr_boundary(0);
      hdr_ctype(CTYPE_MESSAGE);
    }
    qmail_puts(&qq,"\n");
    if (seek_begin(fd) == -1)
      strerr_die2sys(111,FATAL,MSG1(ERR_SEEK,fnmsg.s));

    substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));
    if (qmail_copy(&qq,&sstext,-1) != 0)
      strerr_die2sys(111,FATAL,MSG1(ERR_READ,fnmsg.s));
    close(fd);

    if (flagmime)
      hdr_boundary(1);

    stralloc_copy(&line,&outlocal);
    stralloc_cats(&line,"-return-@");
    stralloc_cat(&line,&outhost);
    stralloc_0(&line);
    qmail_from(&qq,line.s);
    if (to.len)
      qmail_to(&qq,to.s);

    stralloc_copys(&fnnew,"mod/rejected/");
    stralloc_cats(&fnnew,fnbase.s);
    stralloc_0(&fnnew);

/* this is strictly to track what happened to a message to give informative */
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
        strerr_die4x(111,FATAL,MSG(ERR_TMP_QMAIL_QUEUE),": ",err + 1);

  } else if (str_start(action,ACTION_ACCEPT)) {
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
      stralloc_copys(&fnnew,"mod/accepted/");

      stralloc_cats(&fnnew,fnbase.s);
      stralloc_0(&fnnew);
/* ignore errors */
      fd = open_trunc(fnnew.s);
      if (fd != -1)
        close(fd);
      unlink(fnmsg.s);
   }
  _exit(0);
}
