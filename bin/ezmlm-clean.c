#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "error.h"
#include "stralloc.h"
#include "str.h"
#include "env.h"
#include "sig.h"
#include "getconf.h"
#include "getconfopt.h"
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
#include "sys/direntry.h"
#include "cookie.h"
#include "fmt.h"
#include "messages.h"
#include "copy.h"
#include "open.h"
#include "scan.h"
#include "lock.h"
#include "hdr.h"
#include "die.h"
#include "idx.h"
#include "mime.h"
#include "config.h"
#include "auto_version.h"

static int flagmime = MOD_MIME;	/* default is message as attachment */
static int flagreturn = -1;
static stralloc fnmsg = {0};

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

const char FATAL[] = "ezmlm-clean: fatal: ";
const char USAGE[] =
"ezmlm-clean: usage: ezmlm-clean [-mMrRvV] dir";

static struct option options[] = {
  OPT_FLAG(flagmime,'m',1,0),
  OPT_FLAG(flagmime,'M',0,0),
  OPT_FLAG(flagreturn,'r',1,0),
  OPT_FLAG(flagreturn,'R',0,"noreturnposts"),
  OPT_END
};

static void die_read(void)
{
  strerr_die2x(111,FATAL,MSG1(ERR_READ,fnmsg.s));
}

static datetime_sec when;
static unsigned int older;

static char textbuf[1024];
static substdio sstext;

struct qmail qq;

static char strnum[FMT_ULONG];
char boundary[COOKIE];

stralloc quoted = {0};
stralloc line = {0};
static stralloc modtimestr = {0};
static stralloc to = {0};

static int fd;
static int match;
static unsigned long msgnum = 0;
			/* counter to make message-id unique, since we may */
			/* send out several msgs. This is not bullet-proof.*/
			/* Duplication occurs if we do x>1 msg && another  */
			/* ezmlm started within x seconds, and with the    */
			/* same pid. Very unlikely.                        */

static void sendnotice(const char *d)
/* sends file pointed to by d to the address in the return-path of the  */
/* message. */
{
  unsigned int x,y;
  const char *err;

      if (qmail_open(&qq) == -1)
        strerr_die2sys(111,FATAL,MSG(ERR_QMAIL_QUEUE));

      fd = open_read(d);
      if (fd == -1)
        strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,d));
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
      hdr_add2s("Mailing-List: ",MSG(TXT_MAILING_LIST));
      if (listid.len > 0)
	hdr_add2("List-ID: ",listid.s,listid.len);
      hdr_datemsgid(when+msgnum++);
      hdr_from("-help");
      hdr_subject(MSG(SUB_RETURNED_POST));
      hdr_add2s("To: ",to.s);
      if (flagmime) {
	hdr_mime(CTYPE_MULTIPART);
	hdr_boundary(0);
	hdr_ctype(CTYPE_TEXT);
        hdr_transferenc();
      } else
      qmail_puts(&qq,"\n\n");

      copy(&qq,"text/top",flagcd);
      copy(&qq,"text/mod-timeout",flagcd);
      if (flagcd == 'B') {
        encodeB("",0,&line,2);
        qmail_put(&qq,line.s,line.len);
      }

      if (flagmime) {
	hdr_boundary(0);
	hdr_ctype(CTYPE_MESSAGE);
        qmail_puts(&qq,"\n");
      }

      if (seek_begin(fd) == -1)
        strerr_die2sys(111,FATAL,MSG1(ERR_SEEK,d));

      substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));
      if (qmail_copy(&qq,&sstext,-1) != 0) die_read();
      close (fd);

      if (flagmime)
	hdr_boundary(1);

      if (!stralloc_copy(&line,&outlocal)) die_nomem();
      if (!stralloc_cats(&line,"-return-@")) die_nomem();
      if (!stralloc_cat(&line,&outhost)) die_nomem();
      if (!stralloc_0(&line)) die_nomem();
      qmail_from(&qq,line.s);		/* sender */
        qmail_to(&qq,to.s);

     if (*(err = qmail_close(&qq)) != '\0')
       strerr_die4x(111,FATAL,MSG(ERR_TMP_QMAIL_QUEUE),": ", err + 1);

     strnum[fmt_ulong(strnum,qmail_qp(&qq))] = 0;
     strerr_warn2("ezmlm-clean: info: qp ",strnum,0);
}

static void dodir(const char *dirname,int reply)
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
    strerr_die2sys(0,FATAL,MSG1(ERR_OPEN,dirname));
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

int main(int argc,char **argv)
{
  unsigned long delay;
  (void) umask(022);
  sig_pipeignore();
  when = now();

  getconfopt(argc,argv,options,1,0);
  if (flagreturn < 0)
    /* default to returning timed-out messages */
    flagreturn = !getconf_isset("noreturnposts");

  getconf_line(&modtimestr,"modtime",0);
  if (!stralloc_0(&modtimestr)) die_nomem();
  scan_ulong(modtimestr.s,&delay);
  if (!delay) delay = DELAY_DEFAULT;
  else if (delay < DELAY_MIN) delay = DELAY_MIN;
  else if (delay > DELAY_MAX) delay = DELAY_MAX;
  older = (unsigned long) when - 3600L * delay;	/* delay is in hours */

  lockfile("mod/lock");

  dodir("mod/pending/",flagreturn);
  dodir("mod/accepted/",0);
  dodir("mod/rejected/",0);
  dodir("mod/unconfirmed/",0);
  _exit(0);
}

