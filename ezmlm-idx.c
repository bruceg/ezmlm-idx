#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include "stralloc.h"
#include "subfd.h"
#include "strerr.h"
#include "error.h"
#include "lock.h"
#include "slurp.h"
#include "open.h"
#include "getln.h"
#include "sgetopt.h"
#include "case.h"
#include "scan.h"
#include "str.h"
#include "fmt.h"
#include "readwrite.h"
#include "exit.h"
#include "substdio.h"
#include "sig.h"
#include "byte.h"
#include "die.h"
#include "idx.h"
#include "mime.h"
#include "msgtxt.h"
#include "getconf.h"
#include "makehash.h"
#include "config.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-idx: fatal: ";
const char USAGE[] =
"ezmlm-idx: usage: ezmlm-idx [-dDF] [-f msg] dir";

char strnum[FMT_ULONG];
char hash[HASHLEN];

stralloc fnadir = {0};
stralloc fnif = {0};
stralloc fnifn = {0};
stralloc fnaf = {0};

stralloc line = {0};
stralloc lines = {0};
stralloc dummy = {0};

int fdindexn;
int fdlock;
int fd;
int flagdate = 0;	/* use 'Received:' header by default, =1 -> 'Date:' */

	/* for reading index and in ezmlm-idx for reading message */
static substdio ssin;
static char inbuf[1024];

substdio ssindex;
char indexbuf[1024];

struct stat st;

stralloc subject = {0};
stralloc author = {0};
stralloc authmail = {0};
stralloc received = {0};
stralloc prefix = {0};

struct strerr index_err;

stralloc num = {0};

int idx_get_trimsubject(void)

/* reads an open message from 'fd', extracts the subject (if any), and    */
/* returns the subject in 'sub', the author in 'author', and the received */
/* rfc822 date to 'received'. 'fatal' is a program-specific error string. */
/* returns: 0 - no reply no prefix */
/*          1 - reply no prefix */
/*          2 - prefix no reply */
/*          3 - reply & prefix */
/* No terminal '\n' in any of the strallocs! */
{
int foundsubject = 0;
int issubject = 0;
int isfrom = 0;
int foundreceived = 0;
int foundfrom = 0;
int match;
int r;
unsigned int pos,pos1;

  substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));
  for (;;) {
    if (getln(&ssin,&line,&match,'\n') == -1)
      strerr_die2x(111,FATAL,MSG("ERR_READ_INPUT"));
    if (match) {
      if (line.len == 1)
        break;
      if (*line.s == ' ' || *line.s == '\t') {
				/* continuation */
        if (issubject) {
          if (!stralloc_cat(&subject,&line)) die_nomem();
        } else if (isfrom)
          if (!stralloc_cat(&author,&line)) die_nomem();
      } else {
        issubject = 0;
        isfrom = 0;
        if (!foundsubject && case_startb(line.s,line.len,"Subject:")) {
          if (!stralloc_copyb(&subject,line.s+8,line.len-8)) die_nomem();
          foundsubject = 1;
          issubject = 1;
        } else if (!foundfrom && case_startb(line.s,line.len,"From:")) {
          if (!stralloc_copyb(&author,line.s+5,line.len-5)) die_nomem();
          foundfrom = 1;
          isfrom = 1;
        } else if (!flagdate && !foundreceived &&
            case_startb(line.s,line.len,"Received:")) {
          pos = byte_chr(line.s,line.len,';');
          if (pos != line.len)
            if (!stralloc_copyb(&received,line.s+pos+2,line.len - pos - 3))
              die_nomem();
          foundreceived = 1;
        } else if (flagdate && !foundreceived &&
            case_startb(line.s,line.len,"Date:")) {
          if (line.len < 22) continue;				/* illegal */
          pos = 6 + byte_chr(line.s+6,line.len-6,',');
          if (pos == line.len)
            pos = 5;
          ++pos;
          while (line.s[pos] == ' ' || line.s[pos] == '\t') ++pos;	/* dd */
          pos1 = pos + 3;
          while (++pos1 < line.len && line.s[pos1] != ' ');		/* mo */
          ++pos1;
          if (!stralloc_copyb(&received,line.s+pos,pos1 - pos))
              die_nomem();					/* '01 Jun ' */
          if (pos1 + 2 < line.len) {
            if (line.s[pos1 + 2] == ' ') {			/* 2-digit */
              if (line.s[pos1] >= '7') {			/* >= 70 */
              if (!stralloc_cats(&received,"19")) die_nomem();
              } else if (!stralloc_cats(&received,"20")) die_nomem();
              pos = pos1 + 3;					/* 2 digit */
            } else
              pos = pos1 + 5;					/* 4 digit */
            if (pos < line.len) {
              pos += byte_chr(line.s+pos,line.len-pos,' ');	/* after time */
              if (pos < line.len) {
                ++pos;						/* zone */
                while (line.s[pos] != ' ' && line.s[pos] != '\n') ++pos;
              } else
                pos = line.len - 1;	/* no zone. Illegal; better than 0 */
              if (!stralloc_catb(&received,line.s+pos1,pos - pos1))
			die_nomem();
              foundreceived = 1;
              continue;
            }
          }
          received.len = 0;		/* bad format - scrap */
        }
      }
    } else
      break;
  }

  if (foundsubject) {
    concatHDR(subject.s,subject.len,&lines);	/* make 1 line */
    decodeHDR(lines.s,lines.len,&line);		/* decode mime */
    r= unfoldHDR(line.s,line.len,&subject,charset.s,&prefix,1);
						 /* trim mime */
  }
  else {
    r = 0;
    subject.len = 0;
  }
  return r;
}

int main(int argc,char **argv)
{
  char *dir,*cp;
  unsigned long msgnum = 0L;
  unsigned long msgmax;
  int opt,r;

  while ((opt = getopt(argc,argv,"dDf:FvV")) != opteof)
    switch (opt) {
      case 'd': flagdate = 1; break;
      case 'D': flagdate = 0; break;
      case 'f': if (optarg) (void) scan_ulong(optarg,&msgnum); break;
      case 'F': msgnum = 0L;
      case 'v':
      case 'V': strerr_die2x(0,"ezmlm-archive version: ",auto_version);
      default: die_usage();
  }
  startup(dir = argv[optind]);

  (void) umask(022);
  sig_pipeignore();
			/* obtain lock to write index files */
  fdlock = lockfile("lock");

  getconf_line(&prefix,"prefix",0);
					/* support rfc2047-encoded prefix */
  decodeHDR(prefix.s,prefix.len,&line);
  unfoldHDR(line.s,line.len,&prefix,charset.s,&dummy,0);
					/* need only decoded one */

			/* Get message number */
  getconf_ulong(&msgmax,"num",1);
  if (msgnum > msgmax) _exit(0);
  if (msgnum) {
    msgnum = (msgnum / 100) * 100 - 1;
  }
  while (++msgnum <= msgmax) {
    if (msgnum == 1 || !(msgnum % 100)) {
      if (!stralloc_copys(&fnadir,"archive/")) die_nomem();
      if (!stralloc_catb(&fnadir,strnum,fmt_ulong(strnum,msgnum / 100)))
	die_nomem();
      if (!stralloc_copy(&fnifn,&fnadir)) die_nomem();
      if (!stralloc_copy(&fnif,&fnadir)) die_nomem();
      if (!stralloc_cats(&fnif,"/index")) die_nomem();
      if (!stralloc_cats(&fnifn,"/indexn")) die_nomem();
      if (!stralloc_0(&fnadir)) die_nomem();
      if (!stralloc_0(&fnifn)) die_nomem();
      if (!stralloc_0(&fnif)) die_nomem();

			/* May not exist, so be nice and make it */
      if (mkdir(fnadir.s,0755) == -1)
	if (errno != error_exist)
	  strerr_die2sys(100,FATAL,MSG1("ERR_CREATE",fnadir.s));

			/* Open index */
      fdindexn = open_trunc(fnifn.s);
      if (fdindexn == -1)
        strerr_die2sys(100,FATAL,MSG1("ERR_WRITE",fnifn.s));

			/* set up buffers for index */
      substdio_fdbuf(&ssindex,write,fdindexn,indexbuf,sizeof(indexbuf));

			/* Get subject without the 'Subject: ' */
			/* make sure there is one */
    }

    if (!stralloc_copys(&fnaf,fnadir.s)) die_nomem();
    if (!stralloc_cats(&fnaf,"/")) die_nomem();
    if (!stralloc_catb(&fnaf,strnum,
       fmt_uint0(strnum,(unsigned int) (msgnum % 100),2))) die_nomem();
    if (!stralloc_0(&fnaf)) die_nomem();
    fd = open_read(fnaf.s);
    if (fd == -1) {
      if (errno != error_noent)
        strerr_die2sys(100,FATAL,MSG1("ERR_READ",fnaf.s));
    } else if (fstat(fd,&st) == -1 || (!(st.st_mode & 0100)))
        close(fd);
    else {
      int k;

      subject.len = 0;		/* clear in case they're missing in msg */
      author.len = 0;
      received.len = 0;
      r = idx_get_trimsubject();
      close(fd);
      if (!stralloc_copyb(&line,strnum,fmt_ulong(strnum,msgnum))) die_nomem();
      if (!stralloc_cats(&line,": ")) die_nomem();
      makehash(subject.s,subject.len,hash);
      if (!stralloc_catb(&line,hash,HASHLEN)) die_nomem();
      if (!stralloc_cats(&line," ")) die_nomem();
      if (r & 1)	/* reply */
	if (!stralloc_cats(&line,"Re: ")) die_nomem();
      if (!stralloc_cat(&line,&subject)) die_nomem();
      if (!stralloc_cats(&line,"\n\t")) die_nomem();
      if (!stralloc_cat(&line,&received)) die_nomem();
      if (!stralloc_cats(&line,";")) die_nomem();

      concatHDR(author.s,author.len,&lines);
      mkauthhash(lines.s,lines.len,hash);
      if (!stralloc_catb(&line,hash,HASHLEN)) die_nomem();

      k = author_name(&cp,lines.s,lines.len);
      decodeHDR(cp,k,&author);

      (void) unfoldHDR(author.s,author.len,&lines,charset.s,&prefix,0);

      if (!stralloc_cats(&line," ")) die_nomem();
      if (!stralloc_cat(&line,&lines)) die_nomem();
      if (!stralloc_cats(&line,"\n")) die_nomem();
      if (substdio_put(&ssindex,line.s,line.len) == -1)
          strerr_die2sys(100,FATAL,MSG1("ERR_WRITE",fnifn.s));
    }

    if (!((msgnum + 1) % 100) ||
		(msgnum == msgmax)) {	/* last in this set */
      if (substdio_flush(&ssindex) == -1)
        strerr_die4sys(100,FATAL,MSG("ERR_FLUSH"),fnifn.s, ": ");
      if (fsync(fdindexn) == -1)
        strerr_die4sys(100,FATAL,MSG("ERR_SYNC"),fnifn.s, ": ");
      if (fchmod(fdindexn,MODE_ARCHIVE | 0700) == -1)
        strerr_die2sys(100,FATAL,MSG1("ERR_WRITE",fnifn.s));
      if (close(fdindexn) == -1)
        strerr_die4sys(100,FATAL,MSG("ERR_CLOSE"),fnifn.s,": ");
      if (rename(fnifn.s,fnif.s) == -1)
        strerr_die4x(111,FATAL,MSG("ERR_MOVE"),fnifn.s,": ");
    }
  }
  fd = open_append("indexed");
  if (fd == -1)
    strerr_die2sys(100,FATAL,MSG1("ERR_CREATE","indexed"));
  close(fd);
  close(fdlock);
  _exit(0);
}

