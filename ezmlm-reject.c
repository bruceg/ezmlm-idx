#include <unistd.h>
#include "strerr.h"
#include "substdio.h"
#include "byte.h"
#include "case.h"
#include "str.h"
#include "readwrite.h"
#include "stralloc.h"
#include "getln.h"
#include "sgetopt.h"
#include "getconf.h"
#include "constmap.h"
#include "fmt.h"
#include "qmail.h"
#include "seek.h"
#include "scan.h"
#include "env.h"
#include "msgtxt.h"
#include "die.h"
#include "idx.h"
#include "config.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-reject: fatal: ";
const char USAGE[] =
"ezmlm-reject: usage: ezmlm-reject [-bBcCfFhHqQsStT] [dir]";

int flagrejectcommands = 1;	/* reject if subject is simple command */
int flagneedsubject = 1;	/* reject if subject is missing */
int flagtook = 0;		/* reject unless listaddress in To: or Cc: */
int exitquiet = 100;		/* reject with error (100) rather than exit */
				/* quietly (99) if listaddress missing */
int flagheaderreject = 0;	/* don't reject messages with headers from */
				/* other mailing lists. */
int flagbody = 0;		/* =1 => reject if subject or body starts with*/
				/* "subscribe" or "unsubscribe" */
int flagforward = 0;		/* =1 => forward commands to list-request */
int flagparsemime = 0;
int flaghavesubject = 0;
int flaghavecommand = 0;
int flagcheck = 0;		/* set after boundary is found in body, */
				/* until blank line */
unsigned long copylines = 0;	/* Number of lines from the message to copy */

stralloc mimeremove = {0};
stralloc mimereject = {0};
stralloc headerreject = {0};

struct constmap mimeremovemap;
struct constmap mimerejectmap;
struct constmap headerrejectmap;
int mimeremoveflag = 0;

char strnum[FMT_ULONG];
char buf0[256];
substdio ssin = SUBSTDIO_FDBUF(read,0,buf0,(int) sizeof(buf0));
substdio ssin2 = SUBSTDIO_FDBUF(read,0,buf0,(int) sizeof(buf0));

struct qmail qq;

stralloc line = {0};
stralloc to = {0};
stralloc content = {0};
stralloc subject = {0};
stralloc boundary = {0};
stralloc precd = {0};
stralloc mydtline = {0};

unsigned int findlocal(const stralloc *sa,unsigned int n)
	/* n is index of '@' within sa. Returns index to last postition */
	/* of local, n otherwise. */
{
  const char *first;
  const char *s;
  int level = 0;

  first = sa->s;
  s = sa->s + n;
  if (s <= first) return n;
  while (--s >= first) {
    switch (*s) {
      case ' ': case '\t': case '\n': break;
      case ')':
        if (--s <= first) return n;
        if (*s == '\\') break;
        ++level; ++s;
        while (level && --s > first) {
          if (*s == ')') if (*(s-1) != '\\') ++level;
          if (*s == '(') if (*(s-1) != '\\') --level;
        }
        break;
      case '"':
        --s;
        if (s < first) return n;
        return (unsigned int) (s - first);
      default:
        return (unsigned int) (s - first);
    }
  }
  return n;
}

unsigned int findhost(const stralloc *sa,unsigned int n)
	/* s in index to a '@' within sa. Returns index to first pos of */
	/* host part if there is one, n otherwise. */
{
  const char *last;
  const char *s;
  int level = 0;

  last = sa->s + sa->len - 1;
  s = sa->s + n;
  if (s >= last) return n;
  while (++s <= last) {
    switch (*s) {
      case ' ': case '\t': case '\n': break;
      case '(':
        ++level;
        while (level && (++s < last)) {
          if (*s == ')') --level; if (!level) break;
          if (*s == '(') ++level;
          if (*s == '\\') ++s;
        }
        break;
      case '"':
        while (++s < last) {
          if (*s == '"') break;
          if (*s == '\\') ++s;
        }
        break;
      default:
        return (unsigned int) (s - sa->s);
    }
  }
  return n;
}

int getto(const stralloc *sa)
	/* find list address in line. If found, return 1, else return 0. */
{
  unsigned int pos = 0;
  unsigned int pos1;

  if (!sa->len) return 0;		/* no To: or Cc: line */
  while ((pos += 1 + byte_chr(sa->s+pos+1,sa->len-pos-1,'@')) != sa->len) {
    pos1 = findhost(sa,pos);
    if (pos1 == pos) break;
    if (pos1 + outhost.len <= sa->len)
      if (!case_diffb(sa->s+pos1,outhost.len,outhost.s)) { /* got host */
        pos1 = findlocal(sa,pos);
        if (pos1 == pos) break;
        ++pos1;				/* avoids 1 x 2 below */
        if (pos1 >= outlocal.len)
        if (!case_diffb(sa->s+pos1-outlocal.len,outlocal.len,outlocal.s))
          return 1;			/* got local as well */
     }
  }
  return 0;
}

void main(int argc,char **argv)
{
  unsigned long maxmsgsize = 0L;
  unsigned long minmsgsize = 0L;
  unsigned long msgsize = 0L;
  int opt;
  char linetype = ' ';
  char *cp, *cpstart, *cpafter;
  const char *dir;
  const char *err;
  const char *sender;
  unsigned int len;
  int match;

  while ((opt = getopt(argc,argv,"bBcCfFhHqQsStT")) != opteof)
    switch(opt) {
      case 'b': flagbody = 1; break;
      case 'B': flagbody = 0; break;
      case 'c': flagrejectcommands = 1; break;
      case 'C': flagrejectcommands = 0; break;
      case 'f': flagforward = 1; break;
      case 'F': flagforward = 0; break;
      case 'h': flagheaderreject = 1; break;
      case 'H': flagheaderreject = 0; break;
      case 'q': exitquiet = 99; break;
      case 'Q': exitquiet = 100; break;
      case 's': flagneedsubject = 1; break;
      case 'S': flagneedsubject = 0; break;
      case 't': flagtook = 0; break;
      case 'T': flagtook = 1; break;
      case 'v':
      case 'V': strerr_die2x(0, "ezmlm-reject version: ",auto_version);

      default: die_usage();
    }
  dir = argv[optind];
  if (dir) {
    startup(dir);
    flagparsemime = 1;		/* only if dir do we have mimeremove/reject */
    if (getconf_line(&line,"msgsize",0)) {
      if (!stralloc_0(&line)) die_nomem();
      len = scan_ulong(line.s,&maxmsgsize);
      if (line.s[len] == ':')
        scan_ulong(line.s+len+1,&minmsgsize);
    }
    if (flagforward) {
      if (!stralloc_copys(&mydtline,"Delivered-To: command forwarder for "))
	die_nomem();
      if (!stralloc_catb(&mydtline,outlocal.s,outlocal.len)) die_nomem();
      if (!stralloc_cats(&mydtline,"@")) die_nomem();
      if (!stralloc_catb(&mydtline,outhost.s,outhost.len)) die_nomem();
      if (!stralloc_cats(&mydtline,"\n")) die_nomem();
    }
  } else {
    msgtxt_init();
    flagtook = 1;		/* if no "dir" we can't get outlocal/outhost */
    flagforward = 0;		/* nor forward requests */
  }

  sender = env_get("SENDER");
  if (!sender)
    strerr_die2x(100,FATAL,MSG("ERR_NOSENDER"));
  if (!*sender)
    strerr_die2x(100,FATAL,MSG("ERR_BOUNCE"));

  if (flagparsemime) {		/* set up MIME parsing */
    if (getconf(&mimeremove,"mimekeep",0))
      mimeremoveflag = 1;
    else
      getconf(&mimeremove,"mimeremove",0);
    constmap_init(&mimeremovemap,mimeremove.s,mimeremove.len,0);
    getconf(&mimereject,"mimereject",0);
    constmap_init(&mimerejectmap,mimereject.s,mimereject.len,0);
  }
  if (flagheaderreject) {
    if (!dir) die_usage();
    getconf(&headerreject,"headerreject",1);
    constmap_init(&headerrejectmap,headerreject.s,headerreject.len,0);
  }
  for (;;) {
    if (getln(&ssin,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,MSG("ERR_READ_INPUT"));
    if (!match) break;
    if (flagheaderreject)
      if (constmap(&headerrejectmap,line.s,byte_chr(line.s,line.len,':')))
        strerr_die2x(100,FATAL,MSG("ERR_MAILING_LIST"));

    if (line.len == 1) break;
    cp = line.s; len = line.len;
    if ((*cp == ' ' || *cp == '\t')) {
      switch(linetype) {
	case 'T': if (!stralloc_catb(&to,cp,len-1)) die_nomem(); break;
	case 'S': if (!stralloc_catb(&subject,cp,len-1)) die_nomem(); break;
	case 'C': if (!stralloc_catb(&content,cp,len-1)) die_nomem(); break;
	case 'P': if (!stralloc_catb(&precd,cp,len-1)) die_nomem(); break;
	default: break;
      }
    } else {
      if (!flagtook &&
		(case_startb(cp,len,"to:") || case_startb(cp,len,"cc:"))) {
	linetype = 'T';		/* cat so that To/Cc don't overwrite */
        if (!stralloc_catb(&to,line.s + 3,line.len - 4)) die_nomem();
      } else if ((flagneedsubject || flagrejectcommands) &&
			 case_startb(cp,len,"subject:")) {
	if (!stralloc_copyb(&subject,cp+8,len-9)) die_nomem();
	linetype = 'S';
      } else if (case_startb(cp,len,"content-type:")) {
	if (!stralloc_copyb(&content,cp+13,len-14)) die_nomem();
	linetype = 'C';
      } else if (case_startb(cp,len,"precedence:")) {
	if (!stralloc_copyb(&precd,cp+11,len-12)) die_nomem();
	linetype = 'P';
      } else {
	if (flagforward && line.len == mydtline.len) {
	  if (!byte_diff(line.s,line.len,mydtline.s))
            strerr_die2x(100,FATAL,MSG("ERR_LOOPING"));
        }
        linetype = ' ';
      }
    }
  }
  if (precd.len >= 4 &&
		(!case_diffb(precd.s + precd.len - 4,4,"junk") ||
		!case_diffb(precd.s + precd.len - 4,4,"bulk")))
	  strerr_die1x(99,MSG("ERR_JUNK"));	/* ignore precedence junk/bulk */
  cp = subject.s;
  len = subject.len;
  while (len && (cp[len-1] == ' ' || cp[len-1] == '\t')) --len;
  while (len && ((*cp == ' ') || (*cp == '\t'))) { ++cp; --len; }
  flaghavesubject = 1;

  if (flagbody)
    if ((len > 9 && case_starts(cp,"subscribe"))
	|| (len > 11 && case_starts(cp,"unsubscribe")))
      flaghavecommand = 1;

  switch(len) {
    case 0: flaghavesubject = 0; break;
    case 4: if (!case_diffb("help",4,cp)) flaghavecommand = 1; break;
    case 6:	/* Why can't they just leave an empty subject empty? */
	    if (!case_diffb("(null)",6,cp))
              flaghavesubject = 0;
            else
	    if (!case_diffb("(none)",6,cp))
              flaghavesubject = 0;
            else
              if (!case_diffb("remove",6,cp))
	        flaghavecommand = 1;
            break;
    case 9: if (!case_diffb("subscribe",9,cp)) flaghavecommand = 1; break;
    case 11: if (!case_diffb("unsubscribe",11,cp)) flaghavecommand = 1; break;
    case 12: if (!case_diffb("(no subject)",12,cp)) flaghavesubject = 0; break;
    default: break;
  }

  if (!flagtook && !getto(&to))
    strerr_die2x(exitquiet,FATAL,MSG("ERR_NO_ADDRESS"));

  if (flagneedsubject && !flaghavesubject)
    strerr_die2x(100,FATAL,MSG("ERR_NO_SUBJECT"));

  if (flagrejectcommands && flaghavecommand) {
    if (flagforward) {			/* flagforward => forward */
      if (qmail_open(&qq,(stralloc *) 0) == -1)	/* open queue */
	strerr_die2sys(111,FATAL,MSG("ERR_QMAIL_QUEUE"));
      qmail_put(&qq,mydtline.s,mydtline.len);
      if (seek_begin(0) == -1)
	strerr_die2sys(111,FATAL,MSG("ERR_SEEK_INPUT"));
      if (qmail_copy(&qq,&ssin2,copylines) != 0)
	strerr_die2sys(111,FATAL,MSG("ERR_READ_INPUT"));
      if (!stralloc_copy(&to,&outlocal)) die_nomem();
      if (!stralloc_cats(&to,"-request@")) die_nomem();
      if (!stralloc_cat(&to,&outhost)) die_nomem();
      if (!stralloc_0(&to)) die_nomem();
      qmail_from(&qq,sender);
      qmail_to(&qq,to.s);
      if (*(err = qmail_close(&qq)) == '\0') {
        strnum[fmt_ulong(strnum,qmail_qp(&qq))] = 0;
        strerr_die2x(99,"ezmlm-request: info: forward qp ",strnum);
      } else
        strerr_die3x(111,FATAL,MSG("ERR_TMP_QMAIL_QUEUE"),err + 1);
    } else
      strerr_die2x(100,FATAL,MSG("ERR_SUBCOMMAND"));
  }

  if (content.len) {			/* MIME header */
    cp = content.s;
    len = content.len;
    while (len && (*cp == ' ' || *cp == '\t')) { ++cp; --len; }
    cpstart = cp;
    if (*cp == '"') {			/* might be commented */
      ++cp; cpstart = cp;
      while (len && *cp != '"') { ++cp; --len; }
    } else {
      while (len && *cp != ' ' && *cp != '\t' && *cp != ';') {
        ++cp; --len;
      }
    }

    if (flagparsemime)
      if ((!!constmap(&mimeremovemap,cpstart,cp-cpstart) ^ mimeremoveflag) ||
	  constmap(&mimerejectmap,cpstart,cp-cpstart)) {
	*(cp) = (char) 0;
	strerr_die5x(100,FATAL,MSG("ERR_BAD_TYPE"),cpstart,"'",MSG("ERR_SIZE_CODE"));
      }

    cpafter = content.s+content.len;
    while((cp += byte_chr(cp,cpafter-cp,';')) != cpafter) {
      ++cp;
      while (cp < cpafter && (*cp == ' ' || *cp == '\t')) ++cp;
      if (case_startb(cp,cpafter - cp,"boundary=")) {
        cp += 9;			/* after boundary= */
        if (cp < cpafter && *cp == '"') {
          ++cp;
          cpstart = cp;
          while (cp < cpafter && *cp != '"') ++cp;
	  if (cp == cpafter)
		strerr_die1x(100,MSG("ERR_MIME_QUOTE"));
        } else {
          cpstart = cp;
          while (cp < cpafter &&
             *cp != ';' && *cp != ' ' && *cp != '\t') ++cp;
        }
        if (!stralloc_copys(&boundary,"--")) die_nomem();
        if (!stralloc_catb(&boundary,cpstart,cp-cpstart))
		die_nomem();
	break;
      }
    }		/* got boundary, now parse for parts */
  }

  for (;;) {
    if (getln(&ssin,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,MSG("ERR_READ_INPUT"));
    if (!match) break;
    if (line.len == 1) {
      flagcheck = 0;
      continue;
		/* Doesn't do continuation lines. _very_ unusual, and worst */
		/* case one slips through that shouldn't have */
    } else if (flagcheck && case_startb(line.s,line.len,"content-type:")) {
        cp = line.s + 13;
	len = line.len - 14;			/* zap '\n' */
        while (*cp == ' ' || *cp == '\t') { ++cp; --len; }
        cpstart = cp;
	if (*cp == '"') {			/* quoted */
	  ++cp; cpstart = cp;
	  while (len && *cp != '"') { ++cp; --len; }
        } else {				/* not quoted */
          while (len && *cp != ' ' && *cp != '\t' && *cp != ';') {
	    ++cp; --len;
	  }
        }
	if (flagparsemime && constmap(&mimerejectmap,cpstart,cp-cpstart)) {
          *cp = '\0';
          strerr_die4x(100,FATAL,MSG("ERR_BAD_PART"),cpstart,MSG("ERR_SIZE_CODE"));
        }
    } else if (boundary.len && *line.s == '-' && line.len > boundary.len &&
	!str_diffn(line.s,boundary.s,boundary.len)) {
        flagcheck = 1;
    } else {
      if (!msgsize && flagbody)
	if (case_startb(line.s,line.len,"subscribe") ||
		case_startb(line.s,line.len,"unsubscribe"))
	  strerr_die2x(100,FATAL,MSG("ERR_BODYCOMMAND"));
      if (!flagcheck) {
	  msgsize += line.len;
	  if (maxmsgsize && msgsize > maxmsgsize) {
	    strnum[fmt_ulong(strnum,maxmsgsize)] = 0;
	    strerr_die5x(100,FATAL,MSG("ERR_MAX_SIZE"),strnum," bytes",MSG("ERR_SIZE_CODE"));
	  }
      }
    }
  }
  if (msgsize < minmsgsize) {
    strnum[fmt_ulong(strnum,minmsgsize)] = 0;
        strerr_die5x(100,FATAL,MSG("ERR_MIN_SIZE"),strnum," bytes",MSG("ERR_SIZE_CODE"));
  }
  _exit(0);
}
