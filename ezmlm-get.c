#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include "alloc.h"
#include "error.h"
#include "stralloc.h"
#include "str.h"
#include "env.h"
#include "sender.h"
#include "sig.h"
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
#include "fmt.h"
#include "getconfopt.h"
#include "cookie.h"
#include "makehash.h"
#include "copy.h"
#include "constmap.h"
#include "subdb.h"
#include "hdr.h"
#include "open.h"
#include "lock.h"
#include "scan.h"
#include "idxthread.h"
#include "die.h"
#include "wrap.h"
#include "idx.h"
#include "mime.h"
#include "messages.h"
#include "config.h"
#include "auto_version.h"

int flagdo = 1;			/* React to commands (doesn't affect -dig)*/
int omitbottom = 0;		/* copy text/bottom + request */
int flagpublic = -1;		/* 0 = non-public, 1 = public, -1 = respect */
				/* dir/public. */
int flagsubonly = -1;		/* =1 subscribers only for get/index/thread */
unsigned long copylines = 0;	/* Number of lines from the message to copy */
const char *digsz =
		"from\\to\\subject\\reply-to\\date\\message-id\\cc\\"
		"mime-version\\content-type\\content-transfer-encoding";
int flagarchived;		/* if list is archived */
int flagindexed;		/* if list is indexed */
const char *flagformat = 0;

const char FATAL[] = "ezmlm-get: fatal: ";
const char USAGE[] =
"ezmlm-get: usage: ezmlm-get [-bBcClLpPsSvV] [-f fmt] [digestcode]";

static struct option options[] = {
  OPT_FLAG(omitbottom,'b',0,0),	/* add text/bottom (default) */
  OPT_FLAG(omitbottom,'B',1,0),	/* suppress text/bottom */
  OPT_FLAG(flagdo,'c',1,0),	/* do commands */
  OPT_FLAG(flagdo,'C',0,0),	/* ignore commands X dig */
  OPT_CSTR(flagformat,'f',"digformat"),
  OPT_FLAG(flagpublic,'p',1,0), /* always public */
  OPT_FLAG(flagpublic,'P',0,0),	/* never public = only mods do cmd */
  OPT_FLAG(flagsubonly,'s',1,"subgetonly"), /* only subs have archive access */
  OPT_FLAG(flagsubonly,'S',0,0), /* everyone has archive access */
  OPT_FLAG(flagindexed,0,1,"indexed"),
  OPT_FLAG(flagarchived,0,1,"archived"),
  OPT_ULONG(copylines,0,"copylines"),
  OPT_END
};

stralloc listname = {0};
stralloc fn = {0};
stralloc moddir = {0};
stralloc mydtline = {0};
stralloc digheaders = {0};
stralloc seed = {0};
stralloc digestcodefile = {0};
struct constmap digheadersmap;

char schar[] = "00_";
stralloc listno = {0};

datetime_sec when;
unsigned long cumsize = 0L;	/* cumulative msgs / 256 */
unsigned long cumsizen = 0L;	/* new cumulative msgs / 256 */
unsigned long max = 0L;		/* Last message in archive */
unsigned long msgsize = 0L;	/* for digest accounting */
datetime_sec digwhen;		/* last digest */

char strnum[FMT_ULONG];
char strnum2[FMT_ULONG];
char szmsgnum[FMT_ULONG];
char boundary[COOKIE];
char hashout[COOKIE];
stralloc line = {0};
stralloc line2 = {0};
stralloc qline = {0};
stralloc quoted = {0};
stralloc msgnum = {0};
stralloc num = {0};
stralloc subject = {0};
stralloc author = {0};

/* for copy archive */
stralloc archdate = {0};
stralloc archfrom = {0};
stralloc archto = {0};
stralloc archcc = {0};
stralloc archsubject = {0};
stralloc archmessageid = {0};
stralloc archkeywords = {0};
stralloc archblanklines = {0};
char archtype=' ';

/* for mods on non-public lists (needed for future fuzzy sub dbs) */
stralloc mod = {0};		/* moderator addr for non-public lists */
int ismod = 0;			/* true for moderator senders */

int act = AC_NONE;		/* Action we do */
int flageditor = 0;		/* if we're invoked for within dir/editor */
struct stat st;

int flaglocked = 0;		/* if directory is locked */
int flagq = 0;			/* don't use 'quoted-printable' */

const char *workdir;
const char *sender;
const char *digestcode;

struct qmail qq;

int subto(const char *s,unsigned int l)
{
  qmail_put(&qq,"T",1);
  qmail_put(&qq,s,l);
  qmail_put(&qq,"",1);
  return (int) l;
}

char inbuf[1024];
substdio ssin = SUBSTDIO_FDBUF(read,0,inbuf,sizeof(inbuf));
substdio ssin2 = SUBSTDIO_FDBUF(read,0,inbuf,sizeof(inbuf));

substdio ssnum;
char numbuf[16];

substdio sstext;
char textbuf[1024];

substdio ssindex;
char indexbuf[1024];

int fdlock;

void lockup(void)
/* lock unless locked */
{
  if(!flaglocked) {
    fdlock = lockfile("lock");
    flaglocked = 1;
  }
}

void unlock(void)
/* unlock if locked */
{
  if (flaglocked) {
    close(fdlock);
    flaglocked = 0;
  }
}

void code_qput(const char *s,unsigned int n)
{
    if (!flagcd)
      qmail_put(&qq,s,n);
    else {
      if (flagcd == 'B')
        encodeB(s,n,&qline,0);
      else
        encodeQ(s,n,&qline);
      qmail_put(&qq,qline.s,qline.len);
      msgsize += qline.len;
    }
}

void code_qputs(const char *s)
{
  code_qput(s,str_len(s));
  code_qput("\n",1);
}

void zapnonsub(const char *szerr)
/* fatal error if flagsubonly is set and sender is not a subscriber */
/* expects the current dir to be the list dir. Error is szerr */
/* added check for undefined sender as a precaution */
{
  if (sender && *sender) {	/* "no sender" is not a subscriber */
    if (!flagsubonly)
      return;
    if (issub(0,sender,0))
      return;		/* subscriber */
    if (issub("digest",sender,0))
      return;		/* digest subscriber */
    if (issub("allow",sender,0))
      return;		/* allow addresses */
  }
  strerr_die2x(100,FATAL,MSG1(ERR_SUBSCRIBER_CAN,szerr));
}

void tosender(void)
{
  qmail_puts(&qq,"To: ");
  if (!quote2(&quoted,sender)) die_nomem();
  qmail_put(&qq,quoted.s,quoted.len);
  qmail_puts(&qq,"\n");
}

void get_num(void)
{
  /* read dir/num -> max. */
  getconf_ulong2(&max,&cumsizen,"num",0);
}

unsigned long dignum(void)
{
/* return dignum if exists, 0 otherwise. */
  unsigned long u;
  getconf_ulong(&u,"dignum",0);
  return u;
}

void write_ulong(unsigned long num,unsigned long cum,unsigned long dat,
		 const char *fn,const char *fnn)
/* write num to "fnn" add ':' & cum if cum <>0, then move "fnn" to "fn" */
{
  int fd;

  fd = open_trunc(fnn);
  if (fd == -1)
     strerr_die2sys(111,FATAL,MSG1(ERR_CREATE,fnn));
  substdio_fdbuf(&ssnum,write,fd,numbuf,sizeof(numbuf));
  if (substdio_put(&ssnum,strnum,fmt_ulong(strnum,num)) == -1)
     strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn));
  if (substdio_puts(&ssnum,":") == -1)
     strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn));
  if (substdio_put(&ssnum,strnum,fmt_ulong(strnum,cum)) == -1)
     strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn));
  if (dat) {
    if (substdio_puts(&ssnum,":") == -1)
       strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn));
    if (substdio_put(&ssnum,strnum,fmt_ulong(strnum,dat)) == -1)
       strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn));
  }
  if (substdio_puts(&ssnum,"\n") == -1)
     strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn));
  if (substdio_flush(&ssnum) == -1)
     strerr_die2sys(111,FATAL,MSG1(ERR_FLUSH,fnn));
  if (fsync(fd) == -1)
     strerr_die2sys(111,FATAL,MSG1(ERR_SYNC,fnn));
  if (close(fd) == -1)
     strerr_die2sys(111,FATAL,MSG1(ERR_CLOSE,fnn));
  wrap_rename(fnn,fn);
}

void normal_bottom(char format)
/* Copies bottom text and the original message to the new message */
{
  if (!omitbottom) {
    copy(&qq,"text/bottom",flagcd);
    if (flagcd && format != RFC1153) {
      if (flagcd == 'B') {
	encodeB("",0,&line,2);	/* flush */
	qmail_put(&qq,line.s,line.len);
      }
      hdr_boundary(0);
      hdr_ctype(CTYPE_MESSAGE);
      hdr_adds("Content-Disposition: inline; filename=request.msg");
      qmail_puts(&qq,"\n");
    }
    qmail_puts(&qq,"Return-Path: <");
    if (!quote2(&quoted,sender)) die_nomem();
    qmail_put(&qq,quoted.s,quoted.len);
    qmail_puts(&qq,">\n");
    if (seek_begin(0) == -1)
      strerr_die2sys(111,FATAL,MSG(ERR_SEEK_INPUT));
    if (qmail_copy(&qq,&ssin2,copylines) != 0)
      strerr_die2sys(111,FATAL,MSG(ERR_READ_INPUT));
  } else {
    if (flagcd == 'B' && format != RFC1153) {
      encodeB("",0,&line,2);	/* flush */
      qmail_put(&qq,line.s,line.len);
    }
  }
}

void presub(unsigned long from,unsigned long to,stralloc *subject,
	    int factype,	/* action type (AC_THREAD,AC_GET,AC_DIGEST) */
	    char format)	/* output format type (see idx.h) */
/* Starts within header, outputs "subject" and optional headers, terminates*/
/* header and handles output before table-of-contents                      */
{
  switch(format) {
    case MIME:
    case VIRGIN:
    case NATIVE:
    case MIXED:
        hdr_mime((format == MIXED) ? CTYPE_MULTIPART : CTYPE_DIGEST);
	hdr_add2("Subject: ",subject->s,subject->len);
	hdr_boundary(0);
	hdr_ctype(CTYPE_TEXT);
	hdr_transferenc();	/* content-transfer-enc header if needed */
	break;
    case RFC1153:
        hdr_mime(CTYPE_TEXT);
	hdr_add2("Subject: ",subject->s,subject->len);
	qmail_puts(&qq,"\n");
	flagcd = '\0';	/* We make 8-bit messages, not QP/base64 for rfc1153 */
        break;		/* Since messages themselves aren't encoded */
    }
    if (!stralloc_cats(subject,"\n\n")) die_nomem();
    code_qput(subject->s,subject->len);
    if (format != NATIVE && factype != AC_THREAD && factype != AC_INDEX) {
      strnum[fmt_ulong(strnum,from)] = 0;
      strnum2[fmt_ulong(strnum2,to)] = 0;
      code_qputs(MSG2(TXT_TOP_TOPICS,strnum,strnum2));
    }
}

void postsub(int factype, /* action type (AC_THREAD, AC_GET, AC_DIGEST) */
	     char format)	/* output format type (see idx.h) */
/* output after TOC and before first message. */
{
    code_qputs("");
    code_qputs(MSG(TXT_ADMINISTRIVIA));
    code_qputs("");
    if(factype == AC_DIGEST) {
      copy(&qq,"text/digest",flagcd);
      if (flagcd == 'B') {
        encodeB("",0,&line,2);	/* flush */
        qmail_put(&qq,line.s,line.len);
      }
     } else
      normal_bottom(format);
    if (!flagcd || format == RFC1153)
      qmail_puts(&qq,"\n----------------------------------------------------------------------\n");
    else
      qmail_puts(&qq,"\n");
}

void postmsg(char format)
{
    switch(format) {
	case MIME:
	case VIRGIN:
	case NATIVE:
        case MIXED:
		hdr_boundary(1);
		break;
	case RFC1153:
		qmail_puts(&qq,"End of ");
		qmail_put(&qq,listname.s,listname.len);
		qmail_puts(&qq," Digest");
		qmail_puts(&qq,"\n***********************************\n");
		break;
    }
}

void copymsg(int fd,char format)
/* Copy archive message "msg" itself from open file handle fd, in "format" */
{
  int match;
  int flaginheader;
  int flagskipblanks;
  int flaggoodfield;

  switch(format) {
    case VIRGIN:
    case NATIVE:
      substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));
      for (;;) {
        if (getln(&sstext,&line,&match,'\n') == -1)
           strerr_die2sys(111,FATAL,MSG1(ERR_READ,line.s));
        if (match) {
           qmail_put(&qq,line.s,line.len);
	   msgsize += line.len;
        } else
           break;
      }
      break;
    case MIME:
    case MIXED:
      flaginheader = 1;
      flaggoodfield = 0;
      substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));
      for (;;) {
        if (getln(&sstext,&line,&match,'\n') == -1)
           strerr_die2sys(111,FATAL,MSG1(ERR_READ,line.s));
        if (match) {
          if (flaginheader) {
            if (line.len == 1) {
              flaginheader = 0;
              flaggoodfield = 1;
            } else if (line.s[0] != ' ' && line.s[0] != '\t') {
              flaggoodfield = 0;
              if (constmap(&digheadersmap,line.s,
			byte_chr(line.s,line.len,':')))
                flaggoodfield = 1;
            }
            if (flaggoodfield) {
              qmail_put(&qq,line.s,line.len);		/* header */
	      msgsize += line.len;
	    }
          } else {
            qmail_put(&qq,line.s,line.len);		/* body */
	    msgsize += line.len;
	  }
        } else
          break;
      }
      break;
    case RFC1153:		/* Not worth optimizing. Rarely used */
      flaginheader = 1;
      flagskipblanks = 1;	/* must skip terminal blanks acc to rfc1153 */
      archtype = ' ';		/* rfc1153 requires ordered headers */
      if (!stralloc_copys(&archblanklines,"")) die_nomem();
      substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));
      for (;;) {
        if (getln(&sstext,&line,&match,'\n') == -1)
           strerr_die2sys(111,FATAL,MSG1(ERR_READ,line.s));
        if (match) {
          if (flaginheader) {
            if (line.len == 1) {
              flaginheader = 0;
              if (archdate.len) {
                qmail_put(&qq,archdate.s,archdate.len);
                archdate.len = 0;
		msgsize += archdate.len;
              }
              if (archto.len) {
                qmail_put(&qq,archto.s,archto.len);
		msgsize += archto.len;
                archto.len = 0;
              }
              if (archfrom.len) {
                qmail_put(&qq,archfrom.s,archfrom.len);
		msgsize += archfrom.len;
                archfrom.len = 0;
              }
              if (archcc.len) {
                qmail_put(&qq,archcc.s,archcc.len);
		msgsize += archcc.len;
                archcc.len = 0;
              }
              if (archsubject.len) {
                qmail_put(&qq,archsubject.s,archsubject.len);
		msgsize += archsubject.len;
                archsubject.len = 0;
              }
              if (archmessageid.len) {
                qmail_put(&qq,archmessageid.s,archmessageid.len);
		msgsize += archmessageid.len;
                archmessageid.len = 0;
              }
              if (archkeywords.len) {
                qmail_put(&qq,archkeywords.s,archkeywords.len);
		msgsize += archkeywords.len;
                archkeywords.len = 0;
              }
              qmail_puts(&qq,"\n");
            } else if (line.s[0] == ' ' || line.s[0] == '\t') {
              switch (archtype) {	/* continuation lines */
                case ' ':
                  break;
                case 'D':
                  if (!stralloc_cat(&archdate,&line)) die_nomem(); break;
                case 'F':
                  if (!stralloc_cat(&archfrom,&line)) die_nomem(); break;
                case 'T':
                  if (!stralloc_cat(&archto,&line)) die_nomem(); break;
                case 'C':
                  if (!stralloc_cat(&archcc,&line)) die_nomem(); break;
                case 'S':
                  if (!stralloc_cat(&archsubject,&line)) die_nomem(); break;
                case 'M':
                  if (!stralloc_cat(&archmessageid,&line)) die_nomem(); break;
                case 'K':
                  if (!stralloc_cat(&archkeywords,&line)) die_nomem(); break;
                default:
                  strerr_die2x(111,FATAL,
                      "Program error: Bad archive header type");
              }
            } else {
              archtype = ' ';
              if (case_startb(line.s,line.len,"cc:")) {
                archtype='C';
                if (!stralloc_copy(&archcc,&line)) die_nomem();
              }
              else if (case_startb(line.s,line.len,"date:")) {
                archtype='D';
                if (!stralloc_copy(&archdate,&line)) die_nomem();
              }
              else if (case_startb(line.s,line.len,"from:")) {
                archtype='F';
                if (!stralloc_copy(&archfrom,&line)) die_nomem();
              }
              else if (case_startb(line.s,line.len,"keywords:")) {
                archtype='K';
                if (!stralloc_copy(&archkeywords,&line)) die_nomem();
              }
              else if (case_startb(line.s,line.len,"message-id:")) {
                archtype='M';
                if (!stralloc_copy(&archmessageid,&line)) die_nomem();
              }
              else if (case_startb(line.s,line.len,"subject:")) {
                archtype='S';
                if (!stralloc_copy(&archsubject,&line)) die_nomem();
              }
              else if (case_startb(line.s,line.len,"to:")) {
                archtype='T';
                if (!stralloc_copy(&archto,&line)) die_nomem();
              }
            }
          } else if (line.len == 1) {
            if (!flagskipblanks)
              if (!stralloc_copys(&archblanklines,"\n")) die_nomem();
          } else {
            if (archblanklines.len) {
              qmail_put(&qq,archblanklines.s,archblanklines.len);
              archblanklines.len = 0;
            }
            flagskipblanks = 0;
            qmail_put(&qq,line.s,line.len);
	    msgsize += line.len;
          }
        } else
          break;
      }
      break;
    default:
      strerr_die2x(100,FATAL,"Program error: bad format in copymsg()");
  }
}

void mime_getbad(unsigned long msg)
/* Message not found as a MIME multipart */
{
   hdr_boundary(0);
   hdr_ctype(CTYPE_TEXT);
   qmail_puts(&qq,"Content-Disposition: inline; filename=\"");
   qmail_put(&qq,listname.s,listname.len);
   qmail_puts(&qq,"_");
   qmail_put(&qq,strnum,fmt_ulong(strnum,msg));
   qmail_puts(&qq,".ezm\"\n");
   hdr_transferenc();
   copy(&qq,"text/get-bad",flagcd);
}

void msgout(unsigned long msg,char format)
/* Outputs message (everything that's needed per message) */
{
  int fd;
  unsigned int len;

    if (!stralloc_copys(&fn,"archive/")) die_nomem();

    len = fmt_ulong(strnum, msg / 100);
    if (!stralloc_catb(&fn,strnum,len)) die_nomem();
    if (!stralloc_cats(&fn,"/")) die_nomem();
    len = fmt_uint0(strnum, (unsigned int) (msg % 100),2);
    if (!stralloc_catb(&fn,strnum,len)) die_nomem();
    if (!stralloc_0(&fn)) die_nomem();

    switch(format) {
      case MIME:
      case VIRGIN:
      case NATIVE:
      case MIXED:
	fd = open_read(fn.s);
	if (fd == -1) {
	  if (errno != error_noent)
	    strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,fn.s));
          else
            mime_getbad(msg);
        } else if (fstat(fd,&st) == -1 || (!(st.st_mode & 0100))) {
	  close(fd);
          mime_getbad(msg);
        } else {
	  hdr_boundary(0);
	  hdr_ctype(CTYPE_MESSAGE);
          qmail_puts(&qq,"Content-Disposition: inline; filename=\"");
	  qmail_put(&qq,listname.s,listname.len);
	  qmail_puts(&qq,"_");
	  qmail_put(&qq,strnum,fmt_ulong(strnum,msg));
	  qmail_puts(&qq,".ezm\"\n\n");
          copymsg(fd,format);
	  close(fd);
        }
	break;
      case RFC1153:
	fd = open_read(fn.s);
	if (fd == -1) {
	  if (errno != error_noent)
	    strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,fn.s));
	  else {
	    qmail_puts(&qq,"\n== ");
	    qmail_put(&qq,strnum,fmt_ulong(strnum,msg));
	    qmail_puts(&qq," ==\n\n");
	    copy(&qq,"text/get-bad",flagcd);
          }
        } else {
          if (fstat(fd,&st) == -1 || (!(st.st_mode & 0100))) {
	    close(fd);
	    qmail_puts(&qq,"\n== ");
	    qmail_put(&qq,strnum,fmt_ulong(strnum,msg));
	    qmail_puts(&qq," ==\n\n");
	    copy(&qq,"text/get-bad",flagcd);
	  } else {
	    copymsg(fd,format);
	    close(fd);
          }
	}
	qmail_puts(&qq,"\n------------------------------\n\n");
	break;
      default:
        strerr_die2x(100,FATAL,"Program error: Unrecognized format in msgout");
        break;
    }
}

void digest(msgentry *msgtable,
	    subentry *subtable,
	    authentry *authtable,
	    unsigned long from,unsigned long to,
	    stralloc *subj,int factype,char format)
/* Output digest range from-to as per msgtable/subtable (from mkthread(s)). */
/* "Subject is the subject of the _entire_ digest/set. */
{
  const msgentry *pmsgt;
  subentry *psubt;
  const char *cp;
  int ffirstmsg;
  unsigned int len;
  unsigned long msg;
  unsigned long subnum;

  psubt = subtable;
  presub(from,to,subj,factype,format);

  if (format != NATIVE) {
    while (psubt->sub) {
      ffirstmsg = 1;
		/* ptr to first message with this subject */
      pmsgt = msgtable + psubt->firstmsg - from;
      subnum = (unsigned long) (psubt - subtable +1);
      for (msg=psubt->firstmsg; msg<=to; msg++) {
        if (pmsgt->subnum == subnum) {
          if(ffirstmsg) {
            ffirstmsg = 0;
            if (!stralloc_copys(&line,"\n")) die_nomem();
	    if (psubt->sublen <= HASHLEN + 2) {
              if (!stralloc_cats(&line,"(null)\n")) die_nomem();
	    } else
              if (!stralloc_catb(&line,psubt->sub + HASHLEN + 1,
		psubt->sublen - HASHLEN - 1)) die_nomem();
          } else
            if (!stralloc_copys(&line,"")) die_nomem();
          if (!stralloc_cats(&line,"\t")) die_nomem();
          if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,msg))) die_nomem();
          if (!stralloc_append(&line," ")) die_nomem();
          if (pmsgt->authnum) {
	    author.len = 0;
	    cp = authtable[pmsgt->authnum - 1].auth;
	    len = authtable[pmsgt->authnum - 1].authlen - 1;
	    if (len > HASHLEN) {
              if (!stralloc_catb(&author,cp + HASHLEN + 1,
		len - HASHLEN - 1)) die_nomem();
	    } else
	      if (!stralloc_catb(&author,cp,len)) die_nomem();
	    if (!stralloc_0(&author)) die_nomem();
	  }
          if (!stralloc_cats(&line,MSG1(TXT_BY,author.s))) die_nomem();
	  if (!stralloc_append(&line,"\n")) die_nomem();
          code_qput(line.s,line.len);
        }
        pmsgt++;
      }
      psubt++;
    }
  }
  postsub(factype,format);

  psubt = subtable;
  while (psubt->sub) {
    pmsgt = msgtable + psubt->firstmsg - from;
    subnum = (unsigned long) (psubt - subtable +1);
    for (msg=psubt->firstmsg; msg<=to; msg++) {
      if (pmsgt->subnum == subnum)
        msgout(msg,format);
      pmsgt++;
    }
    psubt++;
  }
  postmsg(format);
  idx_destroythread(msgtable,subtable,authtable);
}

void doheaders(void)
{
  int flaggoodfield,match;

  if (act == AC_DIGEST)
    copy(&qq,"headeradd",'H');

  hdr_add2s("Mailing-List: ",MSG(TXT_MAILING_LIST));
  if (listid.len > 0)
    hdr_add2("List-ID: ",listid.s,listid.len);
  hdr_datemsgid(when);
  hdr_from("-help");
  if (!stralloc_copys(&mydtline,"Delivered-To: responder for ")) die_nomem();
  if (!stralloc_catb(&mydtline,outlocal.s,outlocal.len)) die_nomem();
  if (!stralloc_cats(&mydtline,"@")) die_nomem();
  if (!stralloc_catb(&mydtline,outhost.s,outhost.len)) die_nomem();
  if (!stralloc_cats(&mydtline,"\n")) die_nomem();

  qmail_put(&qq,mydtline.s,mydtline.len);

  flaggoodfield = 0;
  if (act != AC_DIGEST)
    for (;;) {
    if (getln(&ssin,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,MSG(ERR_READ_INPUT));
    if (!match) break;
    if (line.len == 1) break;
    if ((line.s[0] != ' ') && (line.s[0] != '\t')) {
      flaggoodfield = 0;
      if (case_startb(line.s,line.len,"mailing-list:")) {
        if (flageditor)			/* we may be running from a sublist */
          flaggoodfield = 0;
        else
          strerr_die2x(100,FATAL,MSG(ERR_MAILING_LIST));
      }
      if (line.len == mydtline.len)
	if (byte_equal(line.s,line.len,mydtline.s))
          strerr_die2x(100,FATAL,MSG(ERR_LOOPING));
      if (case_startb(line.s,line.len,"delivered-to:"))
        flaggoodfield = 1;
      if (case_startb(line.s,line.len,"received:"))
        flaggoodfield = 1;
    }
    if (flaggoodfield)
      qmail_put(&qq,line.s,line.len);
  }
}


void main(int argc,char **argv)
{
  char *def;
  char *local;
  const char *action = "";
  char *psz;
  const char *err;
  int fd;
  unsigned int i,j;
  int flagremote;
  int match;
  int goodexit = 0;			/* exit code for normal exit */
					/* in manager this will be set to 0 */
  unsigned long from,u,to,issue,prevmax;
  unsigned long mno = 0;
  unsigned long chunk;
  unsigned long subs = 0;
  unsigned int pos,pos1;
  unsigned int len;
  int opt;
  char outformat = 0;
  msgentry *msgtable;
  subentry *subtable;
  authentry *authtable;
  dateentry *datetable;
  struct datetime dt;
  char date[DATE822FMT];

  (void) umask(022);
  sig_pipeignore();
  when = now();
  datetime_tai(&dt,when);

  opt = getconfopt(argc,argv,options,1,0);

  initsub(0);
  if (flagformat != 0)
    if (FORMATS[str_chr(FORMATS,flagformat[0])])
      outformat = flagformat[0];
  if (outformat == 0) {
    outformat =
      (getconf_line(&line,"digformat",0)
       && FORMATS[str_chr(FORMATS,line.s[0])])
      ? line.s[0]
      : DEFAULT_FORMAT;
  }

  /* code to activate digest (-digest-code)*/
  if ((digestcode = argv[opt]) == 0) {
    if (getconf_line(&digestcodefile,"digestcode",0)
	&& digestcodefile.len > 0) {
      if (!stralloc_0(&digestcodefile)) die_nomem();
      digestcode = digestcodefile.s;
    }
  }
  /* ignore any extra args */

  if (!stralloc_copy(&listname,&outlocal)) die_nomem();	/* for content disp */

  local = env_get("LOCAL");
  def = env_get("DEFAULT");
  sender = get_sender();
  if (local && *local) {	/* in editor local = outlocal */
    if (!sender) strerr_die2x(100,FATAL,MSG(ERR_NOSENDER));
    if (!*sender)
      strerr_die2x(100,FATAL,MSG(ERR_BOUNCE));
    if (str_equal(sender,"#@[]"))
      strerr_die2x(100,FATAL,MSG(ERR_BOUNCE));
    if (!sender[str_chr(sender,'@')])
      strerr_die2x(100,FATAL,MSG(ERR_ANONYMOUS));
    if (def) {
      if (*def) {
	action = def;
	goodexit = 99;
      } else
	_exit(0);		/* list-@host should do -help from manager */
    } else {			/* editor */
      act = AC_DIGEST;		/* on list-@host ! */
      flageditor = 1;		/* to avoid Mailing-list error on sublists */
				/* when running out of dir/editor. */
    }
    if (case_starts(action,"dig")) {
      action += 3;
      if (action[0] == '-' || action [0] == '.') {
        action++;
	if (!digestcode)
            strerr_die2x(100,FATAL,MSG(ERR_BAD_DIGCODE));
        len = str_len(digestcode);
        if (len <= str_len(action) && case_startb(action,len,digestcode)) {
          if (FORMATS[str_chr(FORMATS,*(action+len))])
            outformat = *(action+len);
          act = AC_DIGEST;
        } else
          strerr_die2x(100,FATAL,MSG(ERR_BAD_DIGCODE));
      }
    }
  } else			/* Command line operation */
    act = AC_DIGEST;

	/* Things we deal with. If anything else just die with success!   */
	/* At the moment this is -index, -thread, and -get.               */
	/* If flagdo = 0 we only service -dig commands. This is to support*/
	/* "secret" lists that are still archived and digested. -c on     */
	/* cmd line. */

  if (act == AC_NONE) {
    if (case_equals(action,ACTION_DIGEST)) {
      act = AC_GET;		/* list-digest@ => msg since last digest */
      action = ACTION_GET;
    } else if (case_starts(action,ACTION_GET) || case_starts(action,ALT_GET))
      act = AC_GET;
    else if (case_starts(action,ACTION_INDEX) || case_starts(action,ALT_INDEX))
      act = AC_INDEX;
    else if (case_starts(action,ACTION_THREAD) ||
	 case_starts(action,ALT_THREAD))
      act = AC_THREAD;
  }
  if (act == AC_NONE)			/* not for us. Pass the buck. */
    _exit(0);
  if (act != AC_INDEX) {		/* need to do header processing */
    if(!getconf(&digheaders,"digheaders",0)) {
      if(!stralloc_copys(&digheaders,digsz)) die_nomem();
      if (!stralloc_0(&digheaders)) die_nomem();
      psz = digheaders.s;
      while (*psz) {
        if (*psz == '\\') *psz = '\0';
        ++psz;
      }
    }
    if (!constmap_init(&digheadersmap,digheaders.s,digheaders.len,0))
	die_nomem();
  }
  if (act != AC_DIGEST) {
    if (!flagdo)			/* only do digests */
      strerr_die2x(100,FATAL,MSG(ERR_NOCMD));
    if (flagpublic < 0)
      flagpublic = !getconf_isset("modgetonly") && getconf_isset("public");
    if (!flagpublic) {
		/* This all to take care of non-public lists. They should*/
		/* still do digests, but do other things only for        */
		/* moderators that have remote access. Since this is rare*/
		/* efforts have been made to keep everything that's not  */
		/* needed elsewhere in here.                   */
      getconf_line(&moddir,"modsub",0);
      flagremote = getconf_line(&line,"remote",0);
      if (!flagremote)
        strerr_die2x(100,FATAL,MSG(ERR_NOT_PUBLIC));
      if (!moddir.len) {
        if (line.len) {
          if (!stralloc_copy(&moddir,&line)) die_nomem();
        } else {
          if (!stralloc_copys(&moddir,"mod")) die_nomem();
        }
      }
      if (!stralloc_0(&moddir)) die_nomem();
      ismod = issub(moddir.s,sender,&mod);
      if (!ismod)			/* sender = moderator? */
        strerr_die2x(100,FATAL,MSG(ERR_NOT_PUBLIC));
    }
  }

  if (act == AC_DIGEST) {
    workdir = "digest";
    if (!stralloc_cats(&outlocal,"-digest")) die_nomem();
    if (getconf_line(&line,"chunk",0)) {
      if (!stralloc_0(&line)) die_nomem();
      (void) scan_ulong(line.s,&chunk);		/* same chunk as main list */
      if (chunk == 0)				/* limit range to 1-53 */
	chunk = 1L;
      else if (chunk > 52)
	chunk = 52L;
    } else {
      chunk = 0L;
    }
  } else
    workdir = ".";


  if (!flagarchived)
    strerr_die2x(100,FATAL,MSG(ERR_NOT_ARCHIVED));

  if (qmail_open(&qq) == -1)
    strerr_die2sys(111,FATAL,MSG(ERR_QMAIL_QUEUE));

  set_cpnum("");	/* default for <#n#> replacement */

  switch (act) {

  case AC_DIGEST:

/* -dig{.|-}'digestcode'[f] returns an rfc1153 digest                        */
/* of messages from the archive. Messages                                    */
/* dignum+1 through the last message received by the list are processed and  */
/* dignum is updated to the last message processed. digissue is advanced.    */

    get_num();				/* max = last successful message */
    to = max;
    lockup();			/* another digest could corrupt dignum */
				/* but will be saved only if flagdigrange==0 */
    if(getconf_line(&num,"dignum",0)) {
      if(!stralloc_0(&num)) die_nomem();
      pos = scan_ulong(num.s,&prevmax);
      if (num.s[pos] == ':') pos++;
      pos += 1 + scan_ulong(num.s+pos,&cumsize);	/* last cumsize */
      if (num.s[pos] == ':') pos++;
      scan_ulong(num.s+pos,&digwhen);			/* last reg dig */
    } else {
      prevmax = 0L;
      cumsize = 0L;
      digwhen = 0L;
    }
    mno = prevmax + 1L;
    if(!max || mno > max)	/* if a digest-list is "sending" the request, */
				/* don't make noise: errors go to postmaster!*/
      strerr_die2x(goodexit,FATAL,MSG(ERR_EMPTY_DIGEST));
    szmsgnum[fmt_ulong(szmsgnum,mno)] = '\0';
    set_cpnum(szmsgnum);	/* for copy */
				/* prepare subject to get entropy for tagmsg*/
    date[date822fmt(date,&dt)-1] = 0; /* skip trailing '\n' in date */
    if (getconf_line(&num,"digissue",0)) {
      if(!stralloc_0(&num)) die_nomem();
      scan_ulong(num.s,&issue);
      issue++;
    } else {
      issue = 1;
    }
    strnum[fmt_ulong(strnum,issue)] = 0;
    if (!stralloc_copys(&subject,MSG2(SUB_DIGEST_ISSUE,date,strnum)))
      die_nomem();
					/* use the subject as entropy */
    if (!stralloc_copy(&line,&subject)) die_nomem();
    if (!stralloc_0(&line)) die_nomem();

    if (!stralloc_ready(&seed,HASHLEN+1)) die_nomem();
    seed.len = HASHLEN + 1;
    seed.s[HASHLEN] = '\0';
    makehash(line.s,line.len,seed.s);
    if (chunk) {			/* only if slaves are used */
      qmail_puts(&qq,"Ezauth: ");
      qmail_put(&qq,seed.s,HASHLEN);
      qmail_puts(&qq,"\n");
    }

    doheaders();
    qmail_puts(&qq,"To: ");
    if (!quote(&quoted,&listname)) die_nomem();
    qmail_put(&qq,quoted.s,quoted.len);
    qmail_puts(&qq,"@");
    qmail_put(&qq,outhost.s,outhost.len);
    qmail_puts(&qq,"\n");
    if (flagindexed && (outformat != NATIVE))
      idx_mkthreads(&msgtable,&subtable,&authtable,&datetable,
	mno,to,max,flaglocked);
    else
      idx_mklist(&msgtable,&subtable,&authtable,mno,to);
    digest(msgtable,subtable,authtable,mno,to,&subject,AC_DIGEST,outformat);

    write_ulong(issue,0L,0L,"digissue","digissuen");
    write_ulong(max,cumsizen, (unsigned long) when,"dignum","dignumn");
    break;

  case AC_GET:

/* -get[-|\.][[num].num2] copies archive num-num2. num & num2 are adjusted   */
/* to be > 0 and <= last message, to num2 >= num and to num2-num <= MAXGET.  */

    zapnonsub(ACTION_GET);		/* restrict to subs if requested */
    tosender();
				/* for rfc1153 */
    if (!stralloc_copys(&subject,MSG1(SUB_DIGEST_OF,action))) die_nomem();

    to = 0;
    pos = str_len(ACTION_GET);
    if (!case_starts(action,ACTION_GET))
      pos = str_len(ALT_GET);
    if (FORMATS[str_chr(FORMATS,action[pos])]) {
       outformat = action[pos];
       ++pos;
    }
					/* optional - or . after '-get' */
    if (action[pos] == '-' || action[pos] == '.') pos++;
    get_num();				/* max = last successful message */
					/* accept any separator. It may be  */
					/* the terminal '\n', but then      */
					/* scan will = 0 on the \0 so should*/
					/* be safe                          */
    if (!max)
      strerr_die2x(100,FATAL,MSG(ERR_EMPTY_LIST));
    szmsgnum[fmt_ulong(szmsgnum,max)] = '\0';
    set_cpnum(szmsgnum);	/* for copy this is the latest message arch'd*/
    doheaders();
    if(action[pos += scan_ulong(action + pos,&u)])
      scan_ulong(action + pos + 1, &to);
    if (u == 0 && to == 0) {		/* default: messages since last */
					/* digest, or last MAXGET if too many */
      to= max;
      u = dignum();
      if (u == 0) {		/* no digest => last up to HISTGET msgs */
	to = max;
	if (max > HISTGET) u = max - HISTGET; else u = 1;
      }
      if (to - u >= MAXGET) u = to - MAXGET + 1;	/* max MAXGET */
    } else if (u > max) {
      if (to) {			/* -get.999999_x returns 30 and msg since last*/
	to = max;		/* digest 30*/
        u = dignum();
	if (u > HISTGET) u -= HISTGET; else u = 1;
        if (to - u >= MAXGET) u = to - MAXGET + 1;
      } else
	u = max;
    }
    if (u == 0) u = 1;			/* -get.5 => 1-5 */
    if (to < u) to = u;			/* -get23_2 => 23 */
    if (to >= u + MAXGET) to = u + MAXGET - 1;
					/* no more than MAXGET at a time */
    if (to > max) to = max;
    if (flagindexed && (outformat != NATIVE))	/* fake out threading */
      idx_mkthreads(&msgtable,&subtable,&authtable,&datetable,
	u,to,max,0);
    else
      idx_mklist(&msgtable,&subtable,&authtable,u,to);
    digest(msgtable,subtable,authtable,u,to,&subject,AC_GET,outformat);
    break;

  case AC_INDEX:

/* -index[f][#|-|\.][[num][.num2] Show subject index for messages num-num2 in*/
/* sets of 100.                                                              */
/* Default last 2 sets. num and num2 are made reasonable as for get. num2 is */
/* limited to num+MAXINDEX to limit the amount of data sent.                 */

    if (!flagindexed)
      strerr_die2x(100,FATAL,MSG(ERR_NOT_INDEXED));
    zapnonsub(ACTION_INDEX);	/* restrict to subs if requested */
    to = 0;
    pos = str_len(ACTION_INDEX);
    if (!case_starts(action,ACTION_INDEX))
      pos = str_len(ALT_INDEX);
    if (FORMATS[str_chr(FORMATS,action[pos])]) {
       outformat = action[pos];		/* ignored, but be nice ... */
       ++pos;
    }
    get_num();				/* max = last successful message */
    if (!max)
      strerr_die2x(100,FATAL,MSG(ERR_EMPTY_LIST));
    szmsgnum[fmt_ulong(szmsgnum,max)] = '\0';
    set_cpnum(szmsgnum);	/* for copy this is the latest message arch'd*/

    doheaders();
    tosender();
    if (!stralloc_copys(&subject,MSG1(SUB_RESULT_OF,action))) die_nomem();
    presub(1,1,&subject,AC_INDEX,outformat);

    if (action[pos] == '-' || action[pos] == '.') pos++;
    if(action[pos += scan_ulong(action + pos,&u)])
      scan_ulong(action + pos + 1, &to);

    if (u == 0 && to == 0) { to = max; u = max - 100; }
    if (u <= 0) u = 1;
    if (u > max) u = max;
    if (to < u) to = u;
    if (to > u + MAXINDEX) to = u+MAXINDEX;	/* max MAXINDEX index files */
    if (to > max) to = max;
    u /= 100;
    to /= 100;
    while (u <= to) {
      if (!stralloc_copys(&fn,"archive/")) die_nomem();
      if (!stralloc_catb(&fn,strnum,fmt_ulong(strnum,u))) die_nomem();
      if (!stralloc_cats(&fn,"/index")) die_nomem();
      if (!stralloc_0(&fn)) die_nomem();

      if (u == max/100)	/* lock if last index file in archive */
        lockup();

      fd = open_read(fn.s);
      if (fd == -1)
        if (errno != error_noent)
          strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,fn.s));
        else
          code_qputs(MSG(TXT_NOINDEX));
      else {
        substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));
        for (;;) {
          if (getln(&sstext,&line,&match,'\n') == -1)
            strerr_die2sys(111,FATAL,MSG1(ERR_READ,fn.s));
          if (match) {
            if (line.s[0] != '\t') {	/* subject line */
              pos = byte_chr(line.s,line.len,' ');
	      pos1 = 0;
	      if (pos && pos != line.len && line.s[pos - 1] == ':')
                pos1 = pos + HASHLEN + 1;	/* after hash */
              if (pos1 >= line.len) {	/* bad! */
                pos = 0;
                pos1 = 0;		/* output as is */
              }
              if (!stralloc_copyb(&line2,line.s,pos)) die_nomem();
              if (!stralloc_catb(&line2,line.s+pos1,line.len-pos1)) die_nomem();
            } else {
	      pos = byte_chr(line.s,line.len,';');
	      if (pos + HASHLEN + 1 < line.len && pos > 15 &&
				line.s[pos + 1] != ' ') {
		  if (!stralloc_copyb(&line2,line.s,pos - 15)) die_nomem();
		  pos++;
		  if (!stralloc_catb(&line2,line.s + pos + HASHLEN,
			line.len - pos - HASHLEN)) die_nomem();
	      } else			/* old format - no author hash */
                if (!stralloc_copyb(&line2,line.s,line.len)) die_nomem();
	    }
            code_qput(line2.s,line2.len);
          } else
            break;
        }
        close(fd);
      }

      if (u == max/100)	/* unlock if last index in archive file */
        unlock();

      u++;
    }
    normal_bottom(outformat);
    postmsg(outformat);
    break;

  case AC_THREAD:

/* -thread[f][-|.]num returns messages with subject matching message        */
/* 'num' in the subject index. If 'num' is not in[1..last_message] an error */
/* message is returned.                                                     */

    if (!flagindexed)
      strerr_die2x(100,FATAL,MSG(ERR_NOT_INDEXED));

    zapnonsub(ACTION_THREAD);		/* restrict to subs if requested*/

    get_num();				/* max = last successful message */
    if (!max)
      strerr_die2x(100,FATAL,MSG(ERR_EMPTY_LIST));
    szmsgnum[fmt_ulong(szmsgnum,max)] = '\0';
    set_cpnum(szmsgnum);	/* for copy this is the latest message arch'd*/

    doheaders();
    tosender();
				/* for rfc1153 */
    if (!stralloc_copys(&subject,MSG1(SUB_DIGEST_OF,action))) die_nomem();

    to = 0;
    pos = str_len(ACTION_THREAD);
    if (!case_starts(action,ACTION_THREAD))
      pos = str_len(ALT_THREAD);
    if (FORMATS[str_chr(FORMATS,action[pos])]) {
       outformat = action[pos];
       ++pos;
    }
    if (action[pos] == '-' || action[pos] == '.') pos++;
    if(action[pos += scan_ulong(action + pos,&u)])
      scan_ulong(action + pos + 1, &to);

    if(u == 0 || u > max) {
      hdr_add2("Subject: ",subject.s,subject.len);
      qmail_puts(&qq,"\n");
      copy(&qq,"text/get-bad",flagcd);
    } else {	/* limit range to at most u-THREAD_BEFORE to u+THREAD_AFTER */
      if (u > THREAD_BEFORE)
        from = u-THREAD_BEFORE;
      else
        from = 1L;
      if (u + THREAD_AFTER > max) {
        idx_mkthread(&msgtable,&subtable,&authtable,from,max,u,max,0);
        digest(msgtable,subtable,authtable,from,max,&subject,
		AC_THREAD,outformat);
      } else {
        idx_mkthread(&msgtable,&subtable,&authtable,
		from,u+THREAD_AFTER,u,max,0);
        digest(msgtable,subtable,authtable,from,u+THREAD_AFTER,
			&subject,AC_THREAD,outformat);
      }
    }
    break;

  default:
	/* This happens if the initial check at the beginning of 'main'    */
	/* matches something that isn't matched here. Conversely, just     */
	/* adding an action here is not enough - it has to be added to the */
	/* initial check as well.                                          */

    strerr_die2x(100,FATAL,
		 "Program error: I'm supposed to deal with this but I didn't");
  }

  if (!stralloc_copy(&line,&outlocal)) die_nomem();
  if (act == AC_DIGEST) {
    if (chunk) {
      if (!stralloc_cats(&line,"-return-g-")) die_nomem();
    } else
      if (!stralloc_cats(&line,"-return-")) die_nomem();
    strnum[fmt_ulong(strnum,mno)] = '\0';
    if (!stralloc_cats(&line,strnum)) die_nomem();
    if (!stralloc_cats(&line,"-@")) die_nomem();

    if (!stralloc_cat(&line,&outhost)) die_nomem();
    if (!stralloc_cats(&line,"-@[]")) die_nomem();
  } else {
    if (!stralloc_cats(&line,"-return-@")) die_nomem();
    if (!stralloc_cat(&line,&outhost)) die_nomem();
  }
  if (!stralloc_0(&line)) die_nomem();

  qmail_from(&qq,line.s);
  if (act == AC_DIGEST) {	 /* Do recipients */
    tagmsg(mno,seed.s,"d",hashout,qq.msgbytes,chunk);
    if (chunk) {
      if (!stralloc_copys(&line,"T")) die_nomem();
      if (!stralloc_cat(&line,&outlocal)) die_nomem();
      if (!stralloc_cats(&line,"-s-d-")) die_nomem();
      if (!stralloc_catb(&line,hashout,COOKIE)) die_nomem();
      if (!stralloc_cats(&line,"-")) die_nomem();
      if (!stralloc_cats(&line,strnum)) die_nomem();
      if (!stralloc_cats(&line,"-")) die_nomem();
      if (!stralloc_copys(&line2,"@")) die_nomem();
      if (!stralloc_cat(&line2,&outhost)) die_nomem();
      if (!stralloc_0(&line2)) die_nomem();
      j = 0;
      for (i = 0; i <= 52; i += chunk) {		/* To slaves */
        qmail_put(&qq,line.s,line.len);
        schar[0] = '0' + i / 10;
        schar[1] = '0' + (i % 10);
        qmail_put(&qq,schar,3);
        j += (chunk - 1);
        if (j > 52) j = 52;
        schar[0] = '0' + j / 10;
        schar[1] = '0' + (j % 10);
        qmail_put(&qq,schar,2);
        qmail_put(&qq,line2.s,line2.len);
      }
    } else
      subs = putsubs(workdir,0L,52L,&subto);
  } else {			/* if local is set, sender is checked */
    if (ismod)
      qmail_to(&qq,mod.s);
    else
      qmail_to(&qq,sender);
  }

  if (*(err = qmail_close(&qq)) == '\0') {	/* Done. Skip rest. */
    if (act == AC_DIGEST) {
      if (chunk)
	(void) logmsg(mno,0L,0L,2);
      else
        (void) logmsg(mno,0L,subs,4);
    }
    closesub();			/* close db connection */
    unlock();			/* NOP if nothing locked */
    strnum[fmt_ulong(strnum,qmail_qp(&qq))] = 0;
    strerr_die2x(goodexit,"ezmlm-get: info: qp ",strnum);
  } else {			/* failed. Reset last msg & issue for digest */
    if(act == AC_DIGEST) {
      issue--;
      write_ulong(issue,0L,0L,"digissue","digissuen");
      write_ulong(prevmax,cumsize,(unsigned long) digwhen,"dignum","dignumn");
    }
    unlock();			/* NOP if nothing locked */
    strerr_die4x(111,FATAL,MSG(ERR_TMP_QMAIL_QUEUE),": ",err + 1);
  }
}
