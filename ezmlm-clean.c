/*$Id$*/
#include <sys/types.h>
#include <sys/stat.h>
#include "error.h"
#include "stralloc.h"
#include "str.h"
#include "env.h"
#include "sig.h"
#include "slurp.h"
#include "getconf.h"
#include "strerr.h"
#include "byte.h"
#include "getln.h"
#include "case.h"
#include "qmail.h"
#include "substdio.h"
#include "readwrite.h"
#include "seek.h"
#include "quote.h"
#include "datetime.h"
#include "now.h"
#include "date822fmt.h"
#include "direntry.h"
#include "cookie.h"
#include "sgetopt.h"
#include "fmt.h"
#include "errtxt.h"
#include "copy.h"
#include "idx.h"
#include "mime.h"
#include "auto_version.h"

int flagmime = MOD_MIME;	/* default is message as attachment */
int flagreturn = 1;		/* default return timed-out messages */
char flagcd = '\0';		/* default: no transferencoding */
stralloc fnmsg = {0};

/* When ezmlm-clean is run, messages and message stubs in pending/      */
/* rejected/accepted/unconfirmed are erased if they are older than      */
/* delay hours.                                                         */
/* Timeouts in h for messages. If modtime has a number, it is made to be*/
/* in the range DELAY_MIN..DELAY_MAX. If the number is 0 or there is no */
/* number, DELAY_DEFAULT is used. Messages that are read-only are       */
/* ignored. Messages in 'pending' that have the execute bit set result  */
/* in an informative reply to the poster. Any defects in the message    */
/* format, inability to open the file, etc, result in a maillog entry   */
/* whereafter the message is erased. */

/* The defines are in "idx.h" */

#define FATAL "ezmlm-clean: fatal: "

void die_read()
{
  strerr_die4x(111,FATAL,ERR_READ,fnmsg.s,": ");
}

void die_usage()
{
  strerr_die1x(100,"ezmlm-clean: usage: ezmlm-clean [-mMrRvV] dir");
}

void die_nomem() { strerr_die2x(111,FATAL,ERR_NOMEM); }

datetime_sec when;
unsigned int older;
struct datetime dt;

char textbuf[1024];
substdio sstext;

struct qmail qq;
int qqwrite(fd,buf,len) int fd; char *buf; unsigned int len;
{
  qmail_put(&qq,buf,len);
  return len;
}
char qqbuf[1];
substdio ssqq = SUBSTDIO_FDBUF(qqwrite,-1,qqbuf,sizeof(qqbuf));

char *dir;
char strnum[FMT_ULONG];
char date[DATE822FMT];
char boundary[COOKIE];
datetime_sec hashdate;

stralloc outhost = {0};
stralloc outlocal = {0};
stralloc mailinglist = {0};
stralloc listid = {0};
stralloc quoted = {0};
stralloc line = {0};
stralloc modtime = {0};
stralloc to = {0};
stralloc charset = {0};

int flagconf;
int fd;
int match;
unsigned long msgnum = 0;
			/* counter to make message-id unique, since we may */
			/* send out several msgs. This is not bullet-proof.*/
			/* Duplication occurs if we do x>1 msg && another  */
			/* ezmlm started within x seconds, and with the    */
			/* same pid. Very unlikely.                        */

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
void readconfigs()
/* gets outlocal, outhost, etc. This is done only if there are any timed-out*/
/* messages found, that merit a reply to the author. */
{

  getconf_line(&mailinglist,"mailinglist",1,FATAL,dir);
  getconf_line(&listid,"listid",0,FATAL,dir);
  getconf_line(&outhost,"outhost",1,FATAL,dir);
  getconf_line(&outlocal,"outlocal",1,FATAL,dir);
  set_cpouthost(&outlocal);
  set_cpoutlocal(&outlocal);
}

void sendnotice(d)
char *d;
/* sends file pointed to by d to the address in the return-path of the  */
/* message. */
{
  unsigned int x,y;
  const char *err;

      if (!flagconf) {
        readconfigs();
      }
      if (qmail_open(&qq, (stralloc *) 0) == -1)
        strerr_die2x(111,FATAL,ERR_QMAIL_QUEUE);

      fd = open_read(d);
      if (fd == -1)
        strerr_die4sys(111,FATAL,ERR_OPEN,d,": ");
      substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));
      if (getln(&sstext,&line,&match,'\n') == -1) die_read();
      if (!match) die_read();
      if (!case_startb(line.s,line.len,"return-path:")) die_read();
      x = 12 + byte_chr(line.s + 12,line.len-12,'<');
      y = byte_rchr(line.s + x,line.len-x,'>');
      if (x != line.len && x+y != line.len) {
        if (!stralloc_copyb(&to,line.s+x+1, y-1)) die_nomem();
        if (!stralloc_0(&to)) die_nomem();
      } else
        die_read();
      qmail_puts(&qq,"Mailing-List: ");
      qmail_put(&qq,mailinglist.s,mailinglist.len);
      qmail_puts(&qq,"\nList-ID: ");
      qmail_put(&qq,listid.s,listid.len);
      qmail_puts(&qq,"\nDate: ");
      datetime_tai(&dt,when);
      qmail_put(&qq,date,date822fmt(date,&dt));
      qmail_puts(&qq,"Message-ID: <");
      if (!stralloc_copyb(&line,strnum,fmt_ulong(strnum,
		(unsigned long) when + msgnum++))) die_nomem();
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
      qmail_puts(&qq,"-help@");
      qmail_put(&qq,outhost.s,outhost.len);
      qmail_puts(&qq,"\nSubject: ");
      qmail_puts(&qq,TXT_RETURNED_POST);
      qmail_put(&qq,quoted.s,quoted.len);
      qmail_puts(&qq,"@");
      qmail_put(&qq,outhost.s,outhost.len);
      qmail_puts(&qq, "\nTo: ");
      qmail_puts(&qq,to.s);
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
      } else
      qmail_puts(&qq,"\n\n");

      copy(&qq,"text/top",flagcd,FATAL);
      copy(&qq,"text/mod-timeout",flagcd,FATAL);
      if (flagcd == 'B') {
        encodeB("",0,&line,2,FATAL);
        qmail_put(&qq,line.s,line.len);
      }

      if (flagmime) {
        qmail_puts(&qq,"\n--");
        qmail_put(&qq,boundary,COOKIE);
        qmail_puts(&qq,"\nContent-Type: message/rfc822\n\n");
      }

      if (seek_begin(fd) == -1)
        strerr_die4sys(111,FATAL,ERR_SEEK,d,": ");

      substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));
      if (substdio_copy(&ssqq,&sstext) != 0) die_read();
      close (fd);

      if (flagmime) {
        qmail_puts(&qq,"\n--");
        qmail_put(&qq,boundary,COOKIE);
        qmail_puts(&qq,"--\n");
      }

      if (!stralloc_copy(&line,&outlocal)) die_nomem();
      if (!stralloc_cats(&line,"-return-@")) die_nomem();
      if (!stralloc_cat(&line,&outhost)) die_nomem();
      if (!stralloc_0(&line)) die_nomem();
      qmail_from(&qq,line.s);		/* sender */
        qmail_to(&qq,to.s);

     if (*(err = qmail_close(&qq)) != '\0')
       strerr_die3x(111,FATAL,ERR_TMP_QMAIL_QUEUE, err + 1);

     strnum[fmt_ulong(strnum,qmail_qp(&qq))] = 0;
     strerr_warn2("ezmlm-clean: info: qp ",strnum,0);
}

void dodir(dirname,reply)
char *dirname; int reply;
/* parses file names in directory 'dirname'. Files that are not owner */
/* writable (w) are ignored. If the files are older (by name!) than   */
/* now-delay, action is taken:                                        */
/* If the owner x bit is not set, the file is erased.                 */
/* If it is set and reply is not set, the file is erased. If both are */
/* set, a notice about the timeout is sent to the poster. If this     */
/* fails due to a message-related error (format, etc) the file is     */
/* erased even though no notice is sent. For temporary errors (like   */
/* out-of-memory) the message is left intact for the next run. If the */
/* notice is sent successfully, the file is erased. All this is to    */
/* do the best possible without risking a rerun of the .qmail file,   */
/* which could result in a redelivery of the action request and a     */
/* second (incorrect) reply to the moderator's request.               */

/* NOTE: ALL non-hidden files in this dir are processed and merci-    */
/* lessly deleted. No checks for proper file name. E.g. 'HELLO'       */
/* => time 0 => will be deleted on the next ezmlm-clean run.          */
{
  DIR *moddir;
  direntry *d;
  unsigned long modtime;
  struct stat st;

  moddir = opendir(dirname);
  if (!moddir)
    strerr_die6sys(0,FATAL,ERR_OPEN,dir,"/",dirname,": ");
  while ((d = readdir(moddir))) {
    if (d->d_name[0] == '.') continue;
    scan_ulong(d->d_name,&modtime);
    if (modtime < older) {
      if (!stralloc_copys(&fnmsg,dirname)) die_nomem();
      if (!stralloc_cats(&fnmsg,d->d_name)) die_nomem();
      if (!stralloc_0(&fnmsg)) die_nomem();
      if((stat(fnmsg.s,&st) != -1) && (st.st_mode & 0200)) {
        if(reply && (st.st_mode & 0100)) {
			/* unlink unless there was a TEMPORARY */
			/* not message-related error notifying */
			/* poster and msg x bit set.  Leave r/o*/
			/* messages alone. Non-x bit msg are   */
			/* trash. Just unlink, don't notify    */
          sendnotice(fnmsg.s);
          unlink(fnmsg.s);
        } else
          unlink(fnmsg.s);
      }
    }
  }
  closedir(moddir);
}


void main(argc,argv)
int argc;
char **argv;
{
  int fdlock;
  int delay;
  int opt;
  (void) umask(022);
  sig_pipeignore();
  when = now();

  while ((opt = getopt(argc,argv,"mMrRvV")) != opteof)
    switch(opt) {
      case 'm': flagmime = 1; break;
      case 'M': flagmime = 0; break;
      case 'r': flagreturn = 1; break;
      case 'R': flagreturn = 0; break;
      case 'v':
      case 'V': strerr_die2x(0,"ezmlm-clean version: ", auto_version);
                /* not reached */
      default:
	die_usage();
    }

  dir = argv[optind];
  if (!dir) die_usage();

  if (chdir(dir) == -1)
    strerr_die4sys(111,FATAL,ERR_SWITCH,dir,": ");

  getconf_line(&modtime,"modtime",0,FATAL,dir);
  if (!stralloc_0(&modtime)) die_nomem();
  scan_ulong(modtime.s,&delay);
  if (!delay) delay = DELAY_DEFAULT;
  else if (delay < DELAY_MIN) delay = DELAY_MIN;
  else if (delay > DELAY_MAX) delay = DELAY_MAX;
  older = (unsigned long) when - 3600L * delay;	/* delay is in hours */

  fdlock = open_append("mod/lock");
  if (fdlock == -1)
    strerr_die4sys(0,FATAL,ERR_OPEN,dir,"/mod/lock: ");
  if (lock_ex(fdlock) == -1)
    strerr_die4sys(0,FATAL,ERR_OBTAIN,dir,"/mod/lock: ");

  flagconf = 0;
  dodir("mod/pending/",flagreturn);
  dodir("mod/accepted/",0);
  dodir("mod/rejected/",0);
  dodir("mod/unconfirmed/",0);
  _exit(0);
}

