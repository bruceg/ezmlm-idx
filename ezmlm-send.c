#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
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
#include "case.h"
#include "scan.h"
#include "str.h"
#include "fmt.h"
#include "readwrite.h"
#include "exit.h"
#include "substdio.h"
#include "getconf.h"
#include "constmap.h"
#include "byte.h"
#include "getconfopt.h"
#include "quote.h"
#include "subdb.h"
#include "mime.h"
#include "messages.h"
#include "makehash.h"
#include "cookie.h"
#include "hdr.h"
#include "die.h"
#include "wrap.h"
#include "idx.h"
#include "copy.h"
#include "config.h"
#include "auto_version.h"

static int flagnoreceived = 1;	/* suppress received headers by default. They*/
				/* are still archived. =0 => archived and */
				/* copied. */
static int flaglog = 1;		/* for lists with mysql support, use tags */
				/* and log traffic to the database */
const char FATAL[] = "ezmlm-send: fatal: ";
const char USAGE[] =
"ezmlm-send: usage: ezmlm-send [-cClLqQrR] [-h header] dir";

static int ignoredflag;
static const char *hash_str = 0;
static char *mlheader = 0;
static int flagrewritefrom = 0;

static struct option options[] = {
  OPT_FLAG(ignoredflag,'c',0,0), /* ignore for backwards compat */
  OPT_FLAG(ignoredflag,'C',0,0),
  OPT_CSTR(mlheader,'H',0),	/* Alternative sublist check header */
  OPT_FLAG(flaglog,'l',1,0),
  OPT_FLAG(flaglog,'L',0,0),
  OPT_FLAG(flagnoreceived,'r',0,0),
  OPT_FLAG(flagnoreceived,'R',1,0),
  OPT_CSTR(hash_str,'s',0),
  OPT_CSTR(hash_str,'S',0),
  OPT_FLAG(ignoredflag,'q',0,0),
  OPT_FLAG(ignoredflag,'Q',0,0),
  OPT_FLAG(flagrewritefrom,0,1,"rewritefrom"),
  OPT_END
};

static stralloc fnadir = {0};
static stralloc fnaf = {0};
static stralloc fnif = {0};
static stralloc fnifn = {0};
static stralloc line = {0};
static stralloc qline = {0};
static stralloc lines = {0};
static stralloc subject = {0};
static stralloc from = {0};
static stralloc received = {0};
static stralloc prefix = {0};
static stralloc content = {0};
stralloc boundary = {0};
static stralloc dcprefix = {0};
static stralloc dummy = {0};
static stralloc author = {0};

void die_indexn(void)
{
  strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnifn.s));
}

static unsigned long innum;
static unsigned long outnum;
static unsigned long msgnum = 0L;
static unsigned long hash_lo = 0L;
static unsigned long hash_hi = 52L;
static unsigned long msgsize = 0L;
static unsigned long cumsize = 0L;	/* cumulative archive size bytes / 256 */
static char encin = '\0';
static int flagindexed;
static int flagfoundokpart;		/* Found something to pass on. If multipart */
					/* we set to 0 and then set to 1 for any */
					/* acceptable mime part. If 0 -> reject */
static int flagsawreceived;
static int flagprefixed;
static unsigned int serial = 0;
static int flagarchived;
static int fdarchive;
static int fdindex;
static int fdindexn;

static substdio ssarchive;
static char archivebuf[1024];

static int flagsublist;
static stralloc sublist = {0};
static stralloc headerremove = {0};
static struct constmap headerremovemap;
static int headerremoveflag = 0;
static stralloc mimeremove = {0};
static struct constmap mimeremovemap;
static int mimeremoveflag = 0;

struct qmail qq;

int subto(const char *s,unsigned int l)
{
  qmail_put(&qq,"T",1);
  qmail_put(&qq,s,l);
  qmail_put(&qq,"",1);
  return (int) l;
}

void die_archive(void)
{
  strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnaf.s));
}
void die_numnew(void)
{
  strerr_die2sys(111,FATAL,MSG1(ERR_CREATE,"numnew"));
}

void qa_put(const char *buf,unsigned int len)
{
  qmail_put(&qq,buf,len);
  if (flagarchived)
    if (substdio_put(&ssarchive,buf,len) == -1) die_archive();
}

void qa_puts(const char *buf)
{
  qa_put(buf,str_len(buf));
}

int sublistmatch(const char *sender)
{
  unsigned int i;
  unsigned int j;

  j = str_len(sender);
  if (j < sublist.len) return 0;

  i = byte_rchr(sublist.s,sublist.len,'@');
  if (i == sublist.len) return 1;

  if (byte_diff(sublist.s,i,sender)) return 0;
  if (case_diffb(sublist.s + i,sublist.len - i,sender + j - (sublist.len - i)))
    return 0;

  return 1;
}

void numwrite(void)
{		/* this one deals with msgnum, not outnum! */
  int fd;
  substdio ssnumnew;
  char numnewbuf[16];
  char strnum[FMT_ULONG];

  fd = open_trunc("numnew");
  if (fd == -1) die_numnew();
  substdio_fdbuf(&ssnumnew,write,fd,numnewbuf,sizeof(numnewbuf));
  if (substdio_put(&ssnumnew,strnum,fmt_ulong(strnum,msgnum)) == -1)
    die_numnew();
  if (substdio_puts(&ssnumnew,":") == -1) die_numnew();
  if (substdio_put(&ssnumnew,strnum,fmt_ulong(strnum,cumsize)) == -1)
    die_numnew();

  if (substdio_puts(&ssnumnew,"\n") == -1) die_numnew();
  if (substdio_flush(&ssnumnew) == -1) die_numnew();
  if (fsync(fd) == -1) die_numnew();
  if (close(fd) == -1) die_numnew(); /* NFS stupidity */
  wrap_rename("numnew","num");
}

int idx_copy_insertsubject(void)
/* copies old index file up to but not including msg, then adds a line with */
/* 'sub' trimmed of reply indicators, then closes the new index and moves it*/
/* to the name 'index'. Errors are dealt with directly, and if the routine  */
/* returns, it was successful. 'fatal' points to a program-specific error   */
/* string. Sub is not destroyed, but from is!!!                             */
/* returns 1 if reply-indicators were found, 0 otherwise.                   */
/* no terminal \n or \0 in any of the strallocs! */
{
  unsigned long idx;
  int match;
  int r;
  unsigned int pos;
  substdio ssin;
  char inbuf[1024];
  /* for writing new index file indexn later moved to index. */
  substdio ssindexn;
  char indexnbuf[1024];
  char strnum[FMT_ULONG];
  char hash[HASHLEN];

  if (!stralloc_copys(&fnadir,"archive/")) die_nomem();
  if (!stralloc_catb(&fnadir,strnum,fmt_ulong(strnum,outnum / 100)))
	die_nomem();
  if (!stralloc_copy(&fnif,&fnadir)) die_nomem();
  if (!stralloc_copy(&fnifn,&fnif)) die_nomem();
  if (!stralloc_cats(&fnif,"/index")) die_nomem();
  if (!stralloc_cats(&fnifn,"/indexn")) die_nomem();
  if (!stralloc_0(&fnif)) die_nomem();
  if (!stralloc_0(&fnifn)) die_nomem();
  if (!stralloc_0(&fnadir)) die_nomem();

			/* may not exists since we run before ezmlm-send */
  if (mkdir(fnadir.s,0755) == -1)
    if (errno != error_exist)
      strerr_die2sys(111,FATAL,MSG1(ERR_CREATE,fnadir.s));

			/* Open indexn */
  fdindexn = open_trunc(fnifn.s);
  if (fdindexn == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnifn.s));

			/* set up buffers for indexn */
  substdio_fdbuf(&ssindexn,write,fdindexn,indexnbuf,sizeof(indexnbuf));

  concatHDR(subject.s,subject.len,&lines);	/* make 1 line */
  decodeHDR(lines.s,lines.len,&qline);		/* decode mime */
  r = unfoldHDR(qline.s,qline.len,&lines,charset.s,&dcprefix,1);
						 /* trim mime */

  fdindex = open_read(fnif.s);
  if (fdindex == -1) {
    if (errno != error_noent)
      strerr_die2x(111,FATAL,MSG1(ERR_OPEN, fnif.s));
  } else {
    substdio_fdbuf(&ssin,read,fdindex,inbuf,sizeof(inbuf));
    for(;;) {
      if (getln(&ssin,&qline,&match,'\n') == -1)
        strerr_die2sys(111,FATAL,MSG1(ERR_READ,fnif.s));
      if (!match)
        break;
      pos = scan_ulong(qline.s,&idx);
      if (!idx)				/* "impossible!" */
        strerr_die2x(111,FATAL,MSG(ERR_BAD_INDEX));
      if (idx >= outnum)
        break;				/* messages always come in order */
      if (substdio_put(&ssindexn,qline.s,qline.len) == -1)
        die_indexn();
      if (qline.s[pos] == ':') {	/* has author line */
        if (getln(&ssin,&qline,&match,'\n') == -1)
          strerr_die2sys(111,FATAL,MSG1(ERR_READ,fnif.s));
        if (!match && qline.s[0] != '\t')	/* "impossible! */
          strerr_die2x(111,FATAL,MSG(ERR_BAD_INDEX));
        if (substdio_put(&ssindexn,qline.s,qline.len) == -1)
          die_indexn();
      }
    }
    close(fdindex);
  }
  if (!stralloc_copyb(&qline,strnum,fmt_ulong(strnum,outnum))) die_nomem();
  if (!stralloc_cats(&qline,": ")) die_nomem();	/* ':' for new ver */
  makehash(lines.s,lines.len,hash);
  if (!stralloc_catb(&qline,hash,HASHLEN)) die_nomem();
  if (!stralloc_cats(&qline," ")) die_nomem();
  if (r & 1)		/* reply */
    if (!stralloc_cats(&qline,"Re: ")) die_nomem();
  if (!stralloc_cat(&qline,&lines)) die_nomem();
  if (!stralloc_cats(&qline,"\n\t")) die_nomem();
  if (!stralloc_cat(&qline,&received)) die_nomem();
  if (!stralloc_cats(&qline,";")) die_nomem();

  concatHDR(from.s,from.len,&lines);
  mkauthhash(lines.s,lines.len,hash);

  if (!stralloc_catb(&qline,hash,HASHLEN)) die_nomem();
  if (!stralloc_cats(&qline," ")) die_nomem();

  author_name(&from,lines.s,lines.len);

  (void) unfoldHDR(from.s,from.len,&lines,charset.s,&dcprefix,0);
  if (!stralloc_cat(&qline,&lines)) die_nomem();

  if (!stralloc_cats(&qline,"\n")) die_nomem();
  if (substdio_put(&ssindexn,qline.s,qline.len) == -1) die_indexn();
  if (substdio_flush(&ssindexn) == -1) die_indexn();
  if (fsync(fdindexn) == -1) die_indexn();
  if (fchmod(fdindexn,MODE_ARCHIVE | 0700) == -1) die_indexn();
  if (close(fdindexn) == -1) die_indexn(); /* NFS stupidity */
  wrap_rename(fnifn.s,fnif.s);
  return r;
}

static void rewrite_from()
{
  concatHDR(from.s,from.len,&dummy);
  author_name(&author,dummy.s,dummy.len);
  stralloc_0(&author);
  stralloc_copy(&dummy,&outlocal);
  stralloc_0(&dummy);
  if (!stralloc_copyb(&line,"From: \"",7)) die_nomem();
  if (!stralloc_cats(&line,MSG2(AUTHOR_VIA_LIST,author.s,dummy.s))) die_nomem();
  if (!stralloc_catb(&line,"\" <",3)) die_nomem();
  if (!stralloc_catb(&line,outlocal.s,outlocal.len)) die_nomem();
  if (!stralloc_catb(&line,"@",1)) die_nomem();
  if (!stralloc_catb(&line,outhost.s,outhost.len)) die_nomem();
  if (!stralloc_catb(&line,">\n",2)) die_nomem();
  if (!stralloc_catb(&line,"Cc:",3)) die_nomem();
  if (!stralloc_catb(&line,from.s,from.len)) die_nomem();
}

int main(int argc,char **argv)
{
  unsigned long subs;
  const char *sender;
  const char *ret;
  const char *err;
  int flagmlwasthere;
  int flaglistid = 0;	/* no listid header added */
  int match;
  unsigned int i;
  int r = 0;
  int flaginheader;
  int flagbadfield;
  int flagbadpart;
  int flagseenext;
  int flagarchiveonly;
  int flagtrailer;
  unsigned int pos;
  char *cp, *cpstart, *cpafter;
  stralloc mydtline = {0};
  char strnum[FMT_ULONG];
  char szmsgnum[FMT_ULONG];
  char hashout[COOKIE+1];

  umask(022);
  sig_pipeignore();

  getconfopt(argc,argv,options,1,0);
  /* Fix up some command-line arguments. */
  if (mlheader != 0)
    mlheader[str_chr(mlheader,':')] = '\0';
  if (hash_str != 0) {
    pos = scan_ulong(hash_str,&hash_lo);
    if (hash_str[pos++]) {
      (void) scan_ulong(hash_str+pos,&hash_hi);
      if (hash_hi > 52L)
	hash_hi = 52L;
      if (hash_lo > hash_hi)
	hash_lo = hash_hi;
    }
  }
  initsub(0);

  sender = get_sender();

  lockfile("lock");

  flagarchived = getconf_isset("archived");
  flagindexed = getconf_isset("indexed");
  flagprefixed = getconf_line(&prefix,"prefix",0);
  if (prefix.len) {		/* encoding and serial # support */
				/* no sanity checks - you put '\n' or '\0' */
				/* into the coded string, you pay */

    decodeHDR(prefix.s,prefix.len,&line);
    (void) unfoldHDR(line.s,line.len,&dcprefix,charset.s,&dummy,0);
    if (!stralloc_copy(&dcprefix,&line)) die_nomem();
    serial = byte_rchr(prefix.s,prefix.len,'#');
  }
  flagtrailer = getconf_isset("addtrailer");

  if (getconf(&mimeremove,"mimekeep",0))
    mimeremoveflag = 1;
  else
    getconf(&mimeremove,"mimeremove",0);

  getconf_ulong2(&msgnum,&cumsize,"num",0);
  ++msgnum;

  flagsublist = getconf_line(&sublist,"sublist",0);

  if (getconf(&headerremove,"headerkeep",0))
    headerremoveflag = 1;
  else
    getconf(&headerremove,"headerremove",1);
  if (!constmap_init(&headerremovemap,headerremove.s,headerremove.len,0))
	die_nomem();

  if (!stralloc_copys(&mydtline,"Delivered-To: mailing list ")) die_nomem();
  if (!stralloc_catb(&mydtline,outlocal.s,outlocal.len)) die_nomem();
  if (!stralloc_cats(&mydtline,"@")) die_nomem();
  if (!stralloc_catb(&mydtline,outhost.s,outhost.len)) die_nomem();
  if (!stralloc_cats(&mydtline,"\n")) die_nomem();

  if (sender) {
    if (!*sender)
      strerr_die2x(100,FATAL,MSG(ERR_BOUNCE));
    if (str_equal(sender,"#@[]"))
      strerr_die2x(100,FATAL,MSG(ERR_BOUNCE));
    if (flagsublist)
      if (!sublistmatch(sender))
        strerr_die2x(100,FATAL,MSG(ERR_NOT_PARENT));
  }
  innum = msgnum;				/* innum = incoming */
  outnum = msgnum;				/* outnum = outgoing */
  if (flagsublist && !flagarchived) {		/* msgnum = archive */
    pos = byte_rchr(sublist.s,sublist.len,'@');	/* checked in sublistmatch */
    if (str_start(sender+pos,"-return-"))
      pos += 8;
      pos += scan_ulong(sender+pos,&innum);
      if (!flagarchived && innum && sender[pos] == '-')
        outnum = innum;
  }
  szmsgnum[fmt_ulong(szmsgnum,outnum)] = '\0';
  set_cpnum(szmsgnum);				/* for copy */

  if (flagarchived) {
    if (!stralloc_copys(&fnadir,"archive/")) die_nomem();
    if (!stralloc_catb(&fnadir,strnum,
		fmt_ulong(strnum,outnum / 100))) die_nomem();
    if (!stralloc_copy(&fnaf,&fnadir)) die_nomem();
    if (!stralloc_cats(&fnaf,"/")) die_nomem();
    if (!stralloc_catb(&fnaf,strnum,fmt_uint0(strnum,
		(unsigned int) (outnum % 100),2))) die_nomem();
    if (!stralloc_0(&fnadir)) die_nomem();
    if (!stralloc_0(&fnaf)) die_nomem();

    if (mkdir(fnadir.s,0755) == -1)
      if (errno != error_exist)
	strerr_die2sys(111,FATAL,MSG1(ERR_CREATE,fnadir.s));
    fdarchive = open_trunc(fnaf.s);
    if (fdarchive == -1)
      strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnaf.s));

    substdio_fdbuf(&ssarchive,write,fdarchive,archivebuf,sizeof(archivebuf));
						/* return-path to archive */
    if (!stralloc_copys(&line,"Return-Path: <")) die_nomem();
    if (sender) {				/* same as qmail-local */
      if (!quote2(&qline,sender)) die_nomem();
      for (i = 0;i < qline.len;++i) if (qline.s[i] == '\n') qline.s[i] = '_';
      if (!stralloc_cat(&line,&qline)) die_nomem();
    }
    if (!stralloc_cats(&line,">\n")) die_nomem();
    if (substdio_put(&ssarchive,line.s,line.len) == -1) die_archive();
  }

    if (qmail_open(&qq) == -1)
      strerr_die2sys(111,FATAL,MSG(ERR_QMAIL_QUEUE));

  if (!flagsublist) {
    hdr_add2s("Mailing-List: ",MSG(TXT_MAILING_LIST));
    if (listid.len > 0) {
      flaglistid = 1;
      hdr_add2("List-ID: ",listid.s,listid.len);
    }
  }
  copy(&qq,"headeradd",'H');
  qa_put(mydtline.s,mydtline.len);

  flagmlwasthere = 0;
  flaginheader = 1;
  flagfoundokpart = 1;
  flagbadfield = headerremoveflag;
  flagbadpart = 0;
  flagseenext = 0;
  flagsawreceived = 0;
  flagarchiveonly = 0;
  for (;;) {
    if (gethdrln(subfdin,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,MSG(ERR_READ_INPUT));
    if (flaginheader && match) {
      if (line.len == 1) {		/* end of header */
	flaginheader = 0;
        if (flagindexed)		/* std entry */
          r = idx_copy_insertsubject();	/* all indexed lists */
        if (flagprefixed && !flagsublist) {
          qa_puts("Subject:");
          if (!flagindexed) {		/* non-indexed prefixed lists */
            concatHDR(subject.s,subject.len,&lines);
            decodeHDR(lines.s,lines.len,&qline);
            r = unfoldHDR(qline.s,qline.len,&lines,
			charset.s,&dcprefix,1);
          }
          if (!(r & 2)) {
            qmail_puts(&qq," ");
            if (serial == prefix.len)
              qmail_put(&qq,prefix.s,prefix.len);
            else {
              qmail_put(&qq,prefix.s,serial);
              qmail_puts(&qq,szmsgnum);
              qmail_put(&qq,prefix.s+serial+1,prefix.len-serial-1);
            }
          }
          qa_put(subject.s,subject.len);
        }
		/* do other stuff to do with post header processing here */
	if (content.len) {		/* get MIME boundary, if exists */
          concatHDR(content.s,content.len,&qline);
          if (!stralloc_copy(&content,&qline)) die_nomem();

	  if (flagtrailer &&		/* trailer only for some multipart */
		case_startb(content.s,content.len,"multipart/"))
	    if (!case_startb(content.s+10,content.len-10,"mixed") &&
		!case_startb(content.s+10,content.len-10,"digest") &&
		!case_startb(content.s+10,content.len-10,"parallel"))
	      flagtrailer = 0;

           cp = content.s;
           cpafter = cp + content.len;	/* check after each ';' */
           while ((cp += byte_chr(cp,cpafter-cp,';')) != cpafter) {
             ++cp;
             while (cp < cpafter &&
			(*cp == ' ' || *cp == '\t' || *cp == '\n')) ++cp;
             if (case_startb(cp,cpafter-cp,"boundary=")) {
               cp += 9;			/* after boundary= */
               if (*cp == '"') {	/* quoted boundary */
                 ++cp;
                 cpstart = cp;
                 while (cp < cpafter && *cp != '"') ++cp;
		 if (cp == cpafter)
			strerr_die1x(100,MSG(ERR_MIME_QUOTE));
               } else {			/* non-quoted boundary */
                 cpstart = cp;		/* find terminator of boundary */
                 while (cp < cpafter && *cp != ';' &&
			*cp != ' ' && *cp != '\t' && *cp != '\n') ++cp;
               }
               if (!stralloc_copys(&boundary,"--")) die_nomem();
               if (!stralloc_catb(&boundary,cpstart,cp-cpstart))
			die_nomem();
	       flagfoundokpart = 0;
               if (!constmap_init(&mimeremovemap,mimeremove.s,mimeremove.len,0))
			die_nomem();
               flagbadpart = 1;		/* skip before first boundary */
               qa_puts("\n");		/* to make up for the lost '\n' */
            }
          }
        }
      } else {
        flagbadfield = headerremoveflag;
        flagarchiveonly = 0;
	if (constmap(&headerremovemap,line.s,byte_chr(line.s,line.len,':')))
	  flagbadfield = !headerremoveflag;
        if ((flagnoreceived || !flagsawreceived) &&
		case_startb(line.s,line.len,"Received:")) {
            if (!flagsawreceived) {		/* get date from first rec'd */
              flagsawreceived = 1;		/* line (done by qmail) */
              pos = byte_chr(line.s,line.len,';');
              if (pos != line.len) {		/* has '\n' */
		for (++pos; pos < line.len; ++pos)
		  if (line.s[pos] != ' ' && line.s[pos] != '\t')
		    break;
                if (!stralloc_copyb(&received,line.s+pos,line.len-pos-1))
                  die_nomem();
	      }
            } else {				/* suppress, but archive */
              flagarchiveonly = 1;		/* but do not suppress the */
              flagbadfield = 1;			/* top one added by qmail */
            }
	} else if (case_startb(line.s,line.len,"Mailing-List:"))
	  flagmlwasthere = 1;		/* sublists always ok ezmlm masters */
	else if (mlheader && case_startb(line.s,line.len,mlheader))
	  flagmlwasthere = 1;		/* mlheader treated as ML */
        else if ((mimeremove.len || flagtrailer) &&	/* else no MIME need*/
		case_startb(line.s,line.len,"Content-Type:")) {
          if (!stralloc_copyb(&content,line.s+13,line.len-13)) die_nomem();
	} else if (case_startb(line.s,line.len,"Subject:")) {
          if (!stralloc_copyb(&subject,line.s+8,line.len-8)) die_nomem();
          if (flagprefixed && !flagsublist)	/* don't prefix for sublists */
	    flagbadfield = 1;			/* we'll print our own */
        } else if (flagtrailer &&
		 case_startb(line.s,line.len,"Content-Transfer-Encoding:")) {
          cp = line.s + 26;
          cpafter = cp + line.len;
          while (cp < cpafter && (*cp == ' ' || *cp == '\t')) ++cp;
          if (case_startb(cp,cpafter-cp,"base64")) encin = 'B';
          else if (case_startb(cp,cpafter-cp,"Quoted-Printable")) encin = 'Q';
        } else if (flaglistid && case_startb(line.s,line.len,"list-id:"))
	  flagbadfield = 1;		/* suppress if we added our own */
	else if (flagindexed) {

          if (case_startb(line.s,line.len,"From:")) {
            if (!stralloc_copyb(&from,line.s+5,line.len-5)) die_nomem();
	    if (flagrewritefrom)
	      rewrite_from();
          }
        } else if (line.len == mydtline.len)
	  if (!byte_diff(line.s,line.len,mydtline.s))
            strerr_die2x(100,FATAL,MSG(ERR_LOOPING));
      }
    } else				/* body */
      msgsize += line.len;		/* always for tstdig support */

    if (!(flaginheader && flagbadfield)) {
      if (boundary.len && line.len > boundary.len &&
		!str_diffn(line.s,boundary.s,boundary.len)) {
        if (line.s[boundary.len] == '-' && line.s[boundary.len+1] == '-') {
          flagbadpart = 0;		/* end boundary should be output */
          if (flagtrailer) {
            qmail_puts(&qq,"\n");
	    hdr_add(boundary.s,boundary.len);
	    hdr_ctype(CTYPE_TEXT);
            hdr_transferenc();		/* trailer for multipart message */
	    copy(&qq,"text/trailer",flagcd);
            if (flagcd == 'B')	{	/* need to do our own flushing */
              encodeB("",0,&qline,2);
              qmail_put(&qq,qline.s,qline.len);
            }
	  }
        } else {			/* new part */
            flagbadpart = 1;		/* skip lines */
            if (!stralloc_copy(&lines,&line)) die_nomem();	/* but save */
            flagseenext = 1;		/* need to check Cont-type */
        }
      } else if (flagseenext) {		/* last was boundary, now stored */
        if (case_startb(line.s,line.len,"content-type:")) {
          flagseenext = 0;		/* done thinking about it */
          cp = line.s + 13;			/* start of type */
          while (*cp == ' ' || *cp == '\t') ++cp;
          cpstart = cp;			/* end of type */
          while (*cp != '\n' && *cp != '\t' && *cp != ' ' && *cp != ';') ++cp;
	  flagbadpart = constmap(&mimeremovemap,cpstart,cp-cpstart)
	    ? !mimeremoveflag
	    : mimeremoveflag;
	  if (!flagbadpart) {
	    /* do this part */
	    flagfoundokpart = 1;
	    qa_put(lines.s,lines.len); /* saved lines */
	  }
        } else if (line.len == 1) {	/* end of content desc */
          flagbadpart = 0;		/* default type, so ok */
          flagfoundokpart = 1;		/* this is part of a multipart msg */
          flagseenext = 0;		/* done thinking about it */
          qa_put(lines.s,lines.len);	/* saved lines */
        } else				/* save line in cont desc */
          if (!stralloc_cat(&lines,&line)) die_nomem();
      }
      if (!flagbadpart)
        qa_put(line.s,line.len);

    } else if (flagarchiveonly && flagarchived)	/* received headers */
      if (substdio_put(&ssarchive,line.s,line.len) == -1) die_archive();
    if (!match)
      break;
  }
  if (!boundary.len && flagtrailer) {
    codeput(&qq,"\n",1,encin);		/* trailer for non-multipart message */
    copy(&qq,"text/trailer",encin);
    codeput(&qq,"\n",1,encin);		/* no need to flush for plain/QP */
    if (encin == 'B')
      qmail_put(&qq,"\n",1); /* Need an actual newline after the base64 data */
  }

  cumsize += (msgsize + 128L) >> 8;	/* round to 256 byte 'records' */
					/* check message tag */
  if (flagsublist) {			/* sublists need tag if selected/suppt*/
    if (flaglog)
      if ((ret = checktag(innum,hash_lo+1L,"m",(char *) 0,hashout))) {
        if (*ret) strerr_die2x(111,FATAL,ret);
        else strerr_die2x(100,FATAL,MSG(ERR_NOT_PARENT));
      }
    if (!flagmlwasthere)		/* sublists need ML header */
      strerr_die2x(100,FATAL,MSG(ERR_SUBLIST));
  } else				/* others are not allowed to have one */
    if (flagmlwasthere)
      strerr_die2x(100,FATAL,MSG(ERR_MAILING_LIST));
  if (!flagfoundokpart)			/* all parts were on the strip list */
      strerr_die2x(100,FATAL,MSG(ERR_BAD_ALL));

  if (flagarchived) {
    if (substdio_flush(&ssarchive) == -1) die_archive();
    if (fsync(fdarchive) == -1) die_archive();
    if (fchmod(fdarchive,MODE_ARCHIVE | 0700) == -1) die_archive();
    if (close(fdarchive) == -1) die_archive(); /* NFS stupidity */
  }

  if (flaglog) {
    tagmsg(innum,sender,"m",hashout,qq.msgbytes,53L);
    hashout[COOKIE] = '\0';
  }

  numwrite();
  if (!stralloc_copy(&line,&outlocal)) die_nomem();
  if (!stralloc_cats(&line,"-return-")) die_nomem();
  if (!stralloc_cats(&line,szmsgnum)) die_nomem();
  if (!stralloc_cats(&line,"-@")) die_nomem();
  if (!stralloc_cat(&line,&outhost)) die_nomem();
  if (!stralloc_cats(&line,"-@[]")) die_nomem();
  if (!stralloc_0(&line)) die_nomem();
  qmail_from(&qq,line.s);			/* envelope sender */
  subs = putsubs(0,hash_lo,hash_hi,subto);	/* subscribers */
  if (flagsublist) hash_lo++;

  if (*(err = qmail_close(&qq)) == '\0') {
      if (flaglog)				/* mysql logging */
	(void) logmsg(outnum,hash_lo,subs,flagsublist ? 3 : 4);
      closesub();
      strnum[fmt_ulong(strnum,qmail_qp(&qq))] = 0;
      strerr_die2x(0,"ezmlm-send: info: qp ",strnum);
  } else {
      --msgnum;
      cumsize -= (msgsize + 128L) >> 8;
      numwrite();
      strerr_die4x(111,FATAL,MSG(ERR_TMP_QMAIL_QUEUE),": ",err + 1);
  }
}
