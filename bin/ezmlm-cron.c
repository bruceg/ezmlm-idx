#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>
#include "strerr.h"
#include "stralloc.h"
#include "getconfopt.h"
#include "substdio.h"
#include "error.h"
#include "str.h"
#include "fmt.h"
#include "wait.h"
#include "readwrite.h"
#include "sig.h"
#include "case.h"
#include "scan.h"
#include "open.h"
#include "lock.h"
#include "byte.h"
#include "getln.h"
#include "auto_qmail.h"
#include "auto_cron.h"
#include "auto_version.h"
#include "messages.h"
#include "die.h"
#include "idx.h"
#include "wrap.h"

const char FATAL[] = "ezmlm-cron: fatal: ";
const char USAGE[] =
"ezmlm-cron: usage: ezmlm-cron [-cCdDlLvV] [-w dow] [-t hh:mm] [-i hrs] listadr code";

static const char *flagt = 0;
static const char *flagw = 0;
static unsigned long deltah = 24L;	/* default interval 24h */
static int flagconfig = 0;
static int flagdelete = 0;
static int flaglist = 0;

static struct option options[] = {
  OPT_FLAG(flagconfig,'c',1,0),
  OPT_FLAG(flagconfig,'C',0,0),
  OPT_FLAG(flagdelete,'d',1,0),
  OPT_FLAG(flagdelete,'D',0,0),
  OPT_ULONG(deltah,'i',0),
  OPT_FLAG(flaglist,'l',1,0),
  OPT_FLAG(flaglist,'L',0,0),
  OPT_CSTR(flagt,'t',0),
  OPT_CSTR(flagw,'w',0),
  OPT_END
};

static void die_syntax(stralloc *line)
{
  if (!stralloc_0(line)) die_nomem();
  strerr_die4x(100,FATAL,MSG1(ERR_SYNTAX,TXT_EZCRONRC),": ",line->s);
}

static void die_argument(void)
{
  strerr_die2x(100,FATAL,MSG(ERR_NOT_CLEAN));
}

static int isclean(char *addr,
		   int flagaddr) /* 1 for addresses with '@', 0 for other args */
	/* assures that addr has only letters, digits, "-_" */
	/* also checks allows single '@' if flagaddr = 1 */
	/* returns 1 if clean, 0 otherwise */
{
  unsigned int pos;
  char ch;
  char *cp;
  if (flagaddr) {		/* shoud have one '@' */
    pos = str_chr(addr,'@');
    if (!pos || !addr[pos])
      return 0;			/* at least 1 char for local */
    if (!addr[pos+1])
      return 0;			/* host must be at least 1 char */
    pos++;
    case_lowerb(addr+pos,str_len(addr)-pos);
  } else
    pos = 0;
  pos +=  str_chr(addr + pos,'@');
  if (addr[pos])		/* but no more */
    return 0;
  cp = addr;
  while ((ch = *(cp++)))
    if (!(ch >= 'a' && ch <= 'z') &&
        !(ch >= 'A' && ch <= 'Z') &&
        !(ch >= '0' && ch <= '9') &&
        ch != '.' && ch != '-' && ch != '_' && ch != '@')
      return 0;
  return 1;
}

int main(int argc,char **argv)
{
  int child;
  const char *sendargs[4];
  stralloc addr = {0};
  unsigned int pos = 0,pos2,poslocal,len;
  const char *cp;

  unsigned long hh = 4L;		/* default time 04:12 */
  unsigned long mm = 12L;
  const char *dow = "*";		/* day of week */
  const char *qmail_inject = "/bin/qmail-inject ";
  char strnum[FMT_ULONG];
  unsigned long uid,euid = 0;

  stralloc rp = {0};
  stralloc user = {0};
  stralloc euser = {0};
  stralloc dir = {0};
  stralloc listaddr = {0};
  stralloc line = {0};

  struct passwd *ppasswd;

  int match;
  int hostmatch;
  int localmatch;
  unsigned long dh,t;
  int founduser = 0;
  int listmatch = 0;
  int flagdigit = 0;
  int flagours;
  int foundlocal = 0;
  int foundmatch = 0;
  unsigned int nolists = 0;
  unsigned long maxlists;
  unsigned int lenhost,lenlocal;
  int fdin,fdout = -1;

  char *local = (char *) 0;	/* list = local@host */
  const char *host = (char *) 0;
  char *code = (char *) 0;	/* digest code */

  char inbuf[512];
  substdio ssin;

  char outbuf[512];
  substdio ssout;

  (void) umask(077);
  sig_pipeignore();

  optind = getconfopt(argc,argv,options,0,0);
  if (flagt != 0) {
    pos = scan_ulong(flagt,&hh);
    if (flagt[pos++] != ':') die_usage();
    (void) scan_ulong(flagt + pos,&mm);
  }
  if (flagw != 0) {
    dow = flagw;
    cp = flagw - 1;
    while (*(++cp)) {
      if (*cp >= '0' && *cp <= '7') {
	if (flagdigit) die_dow();
	flagdigit = 1;
      } else if (*cp == ',') {
	if (!flagdigit) die_dow();
	flagdigit = 0;
      } else
	die_dow();
    }
  }

  if (flaglist + flagdelete + flagconfig > 1)
    strerr_die2x(100,FATAL,MSG(ERR_EXCLUSIVE));
  uid = getuid();
  if (uid && !(euid = geteuid()))
    strerr_die2x(100,FATAL,MSG(ERR_SUID));
  if (!(ppasswd = getpwuid(uid)))
    strerr_die2x(100,FATAL,MSG(ERR_UID));
  if (!stralloc_copys(&user,ppasswd->pw_name)) die_nomem();
  if (!stralloc_0(&user)) die_nomem();
  if (!(ppasswd = getpwuid(euid)))
    strerr_die2x(100,FATAL,MSG(ERR_EUID));
  if (!stralloc_copys(&dir,ppasswd->pw_dir)) die_nomem();
  if (!stralloc_0(&dir)) die_nomem();
  if (!stralloc_copys(&euser,ppasswd->pw_name)) die_nomem();
  if (!stralloc_0(&euser)) die_nomem();

  wrap_chdir(dir.s);

  local = argv[optind++];	/* list address, optional for -c & -l */
  if (!local) {
    if (!flagconfig && !flaglist)
      die_usage();
    lenlocal = 0;
    lenhost = 0;
  } else {
    if (!stralloc_copys(&listaddr,local)) die_nomem();
    if (!isclean(local,1))
      die_argument();
    pos = str_chr(local,'@');
    lenlocal = pos;
    local[pos] = '\0';
    host = local + pos + 1;
    lenhost = str_len(host);
    code = argv[optind];
    if (!code) {		/* ignored for -l, -c, and -d */
      if (flagdelete || flaglist || flagconfig)
				/* get away with not putting code for delete */
        code = (char*)"a";	/* a hack - so what! */
      else
        die_usage();
    } else
      if (!isclean(code,0))
        die_argument();
  }
  if ((fdin = open_read(TXT_EZCRONRC)) == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,TXT_EZCRONRC));
	/* first line is special */
  substdio_fdbuf(&ssin,read,fdin,inbuf,sizeof(inbuf));
  if (getln(&ssin,&line,&match,'\n') == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_READ,TXT_EZCRONRC));

  if (!match)
    strerr_die2sys(111,FATAL,MSG1(ERR_READ,TXT_EZCRONRC));
	/* (since we have match line.len has to be >= 1) */
  line.s[line.len - 1] = '\0';
  if (!isclean(line.s,0))	 /* host for bounces */
    strerr_die2x(100,FATAL,MSG1(ERR_CFHOST,TXT_EZCRONRC));
  if (!stralloc_copys(&rp,line.s)) die_nomem();

  match = 1;
  for(;;) {
    if (!match) break;		/* to allow last line without '\n' */
    if (getln(&ssin,&line,&match,'\n') == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_READ,TXT_EZCRONRC));
    if (!line.len)
      break;
    line.s[line.len-1] = '\0';
    if (!case_startb(line.s,line.len,user.s))
      continue;
    pos = user.len - 1;
    if (pos >= line.len || line.s[pos] != ':')
      continue;
    founduser = 1;		 /* got user line */
    break;
  }
  close(fdin);
  if (!founduser)
    strerr_die2x(100,FATAL,MSG(ERR_BADUSER));
  
  if (flagconfig) {
    line.s[line.len-1] = '\n';	/* not very elegant ;-) */
    substdio_fdbuf(&ssout,write,1,outbuf,sizeof(outbuf));
    if (substdio_put(&ssout,line.s,line.len) == -1)
      strerr_die2sys(111,FATAL,MSG(ERR_WRITE_STDOUT));
    if (substdio_flush(&ssout) == -1)
      strerr_die2sys(111,FATAL,MSG(ERR_WRITE_STDOUT));
    _exit(0);
  }
  ++pos;				/* points to first ':' */
  len = str_chr(line.s+pos,':');	/* second ':' */
    if (!line.s[pos + len])
      die_syntax(&line);
  if (!local) {				/* only -d and std left */
    localmatch = 1;
    hostmatch = 1;
  } else {
    hostmatch = 0;
    if (len <= str_len(local))
      if (!str_diffn(line.s+pos,local,len))
        localmatch = 1;
  }
  pos += len + 1;
  len = str_chr(line.s + pos,':');	/* third */
  if (!line.s[pos + len])
    die_syntax(&line);
  if (local) {				/* check host */
    if (len == 0)			/* empty host => any host */
      hostmatch = 1;
    else
      if (len == str_len(host))
        if (!case_diffb(line.s+pos,len,host))
          hostmatch = 1;
  }
  pos += len + 1;
  pos += scan_ulong(line.s+pos,&maxlists);
  if (line.s[pos]) {			/* check additional lists */
    if (line.s[pos] != ':')
      die_syntax(&line);
    if (line.s[pos+1+str_chr(line.s+pos+1,':')])
      die_syntax(&line);	/* reminder lists are not separated by ':'  */
				/* otherwise a ':' or arg miscount will die */
				/* silently */
    if (local) {
      while (++pos < line.len) {
        len = str_chr(line.s + pos,'@');
        if (len == lenlocal && !str_diffn(line.s + pos,local,len)) {
          pos += len;
          if (!line.s[pos]) break;
          pos++;
          len = str_chr(line.s+pos,',');
            if (len == lenhost && !case_diffb(line.s+pos,len,host)) {
              listmatch = 1;
              break;
            }
        }
        pos += len;
      }
    }
  }
  if (!listmatch) {
    if (!hostmatch)
      strerr_die2x(100,FATAL,MSG(ERR_BADHOST));
    if (!localmatch)
      strerr_die2x(100,FATAL,MSG(ERR_BADLOCAL));
  }
	/* assemble correct line */
  if (!flaglist) {
    if (!stralloc_copyb(&addr,strnum,fmt_ulong(strnum,mm))) die_nomem();
    if (!stralloc_cats(&addr," ")) die_nomem();
    dh = 0L;
    if (deltah <= 3L) dh = deltah;
    else if (deltah <= 6L) dh = 6L;
    else if (deltah <= 12L) dh = 12L;
    else if (deltah <= 24L) dh = 24L;
    else if (deltah <= 48L) {
      if (dow[0] == '*') dow = "1,3,5";
    } else if (deltah <= 72L) {
      if (dow[0] == '*') dow = "1,4";
    } else
    if (dow[0] == '*') dow = "1";

    if (!dh) {
      if (!stralloc_cats(&addr,"*")) die_nomem();
    } else {
      if (!stralloc_catb(&addr,strnum,fmt_ulong(strnum,hh))) die_nomem();
      for (t = hh + dh; t < hh + 24L; t+=dh) {
        if (!stralloc_cats(&addr,",")) die_nomem();
        if (!stralloc_catb(&addr,strnum,fmt_ulong(strnum,t % 24L))) die_nomem();
      }
    }
    if (!stralloc_cats(&addr," * * ")) die_nomem();
    if (!stralloc_cats(&addr,dow)) die_nomem();
    if (!stralloc_cats(&addr," ")) die_nomem();
    if (!stralloc_cats(&addr,auto_qmail)) die_nomem();
    if (!stralloc_cats(&addr,qmail_inject)) die_nomem();
    if (!stralloc_cats(&addr,local)) die_nomem();
    if (!stralloc_cats(&addr,"-dig-")) die_nomem();
    if (!stralloc_cats(&addr,code)) die_nomem();
    if (!stralloc_cats(&addr,"@")) die_nomem();
    if (!stralloc_cats(&addr,host)) die_nomem();
		/* feed 'Return-Path: <user@host>' to qmail-inject */
    if (!stralloc_cats(&addr,"%Return-path: <")) die_nomem();
    if (!stralloc_cats(&addr,user.s)) die_nomem();
    if (!stralloc_cats(&addr,"@")) die_nomem();
    if (!stralloc_cat(&addr,&rp)) die_nomem();
    if (!stralloc_cats(&addr,">\n")) die_nomem();
  }
  if (!stralloc_0(&addr)) die_nomem();

  if (!flaglist) {
	/* now to rewrite crontab we need to lock */
    lockfile("crontabl");
  } /* if !flaglist */
  if ((fdin = open_read("crontab")) == -1) {
    if (errno != error_noent)
      strerr_die2sys(111,FATAL,MSG1(ERR_READ,"crontab"));
  } else
    substdio_fdbuf(&ssin,read,fdin,inbuf,sizeof(inbuf));
  if (flaglist)
    substdio_fdbuf(&ssout,write,1,outbuf,sizeof(outbuf));
  else {
    if ((fdout = open_trunc("crontabn")) == -1)
      strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,"crontabn"));
    substdio_fdbuf(&ssout,write,fdout,outbuf,sizeof(outbuf));
  }
  line.len = 0;

  if (fdin != -1) {
    for (;;) {
      if (!flaglist && line.len) {
        line.s[line.len-1] = '\n';
        if (substdio_put(&ssout,line.s,line.len) == -1)
          strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,"crontabn"));
      }
      if (getln(&ssin,&line,&match,'\n') == -1)
        strerr_die2sys(111,FATAL,MSG1(ERR_READ,"crontab"));
      if (!match)
        break;
      flagours = 0;			/* assume entry is not ours */
      foundlocal = 0;
      line.s[line.len - 1] = '\0';	/* match so at least 1 char */
      pos = 0;
      while (line.s[pos] == ' ' || line.s[pos] == '\t') ++pos;
      if (line.s[pos] == '#')
        continue;			/* cron comment */
      pos = str_chr(line.s,'/');
      if (!str_start(line.s+pos,auto_qmail)) continue;
      pos += str_len(auto_qmail);
      if (!str_start(line.s+pos,qmail_inject)) continue;
      pos += str_len(qmail_inject);
      poslocal = pos;
      pos = byte_rchr(line.s,line.len,'<');	/* should be Return-Path: < */
      if (pos == line.len)
        continue;			/* not ezmlm-cron line */
      pos++;
     len = str_chr(line.s+pos,'@');
      if (len == user.len - 1 && !str_diffn(line.s+pos,user.s,len)) {
        flagours = 1;
        ++nolists;		/* belongs to this user */
      }
      if (!local) {
        foundlocal = 1;
      } else {
        pos = poslocal + str_chr(line.s+poslocal,'@');
        if (pos + lenhost +1 >= line.len) continue;
        if (case_diffb(line.s+pos+1,lenhost,host)) continue;
        if (line.s[pos+lenhost+1] != '%') continue;
				/* check local */
        if (poslocal + lenlocal + 5 >= line.len) continue;
        if (!str_start(line.s+poslocal,local)) continue;
        pos2 = poslocal+lenlocal;
        if (!str_start(line.s+pos2,"-dig-")) continue;
        foundlocal = 1;
      }
      if (foundlocal) {
        foundmatch = 1;
        if (flaglist && (local || flagours)) {
          if (substdio_put(&ssout,line.s,line.len) == -1)
            strerr_die2sys(111,FATAL,MSG(ERR_WRITE_STDOUT));
          if (substdio_put(&ssout,"\n",1) == -1)
            strerr_die2sys(111,FATAL,MSG(ERR_WRITE_STDOUT));
        }
        line.len = 0;		/* same - kill line */
        if (flagours)
          --nolists;
      }
    }
    close(fdin);
  }
  if (flaglist) {
    if (substdio_flush(&ssout) == -1)
      strerr_die2sys(111,FATAL,MSG1(ERR_FLUSH,"stdout"));
    if (foundmatch)		/* means we had a match */
      _exit(0);
    else
      strerr_die2x(100,FATAL,MSG(ERR_NO_MATCH));
  }
	/* only -d and regular use left */

  if (nolists >= maxlists && !flagdelete)
    strerr_die2x(100,FATAL,MSG(ERR_LISTNO));
  if (!flagdelete)
    if (substdio_put(&ssout,addr.s,addr.len-1) == -1)
      strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,"crontabn"));
  if (flagdelete && !foundlocal)
    strerr_die2x(111,FATAL,MSG(ERR_NO_MATCH));
  if (substdio_flush(&ssout) == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_FLUSH,"crontabn"));
  if (fsync(fdout) == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_SYNC,"crontabn++"));
  if (close(fdout) == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_CLOSE,"crontabn"));
  wrap_rename("crontabn","crontab");
  sendargs[0] = "sh";
  sendargs[1] = "-c";

  if (!stralloc_copys(&line,auto_cron)) die_nomem();
  if (!stralloc_cats(&line,"/crontab '")) die_nomem();
  if (!stralloc_cats(&line,dir.s)) die_nomem();
  if (!stralloc_cats(&line,"/crontab'")) die_nomem();
  if (!stralloc_0(&line)) die_nomem();
  sendargs[2] = line.s;
  sendargs[3] = 0;
  if ((child = wrap_fork()) == 0) {
    if (setreuid(euid,euid) == -1)
      strerr_die2sys(100,FATAL,MSG(ERR_SETUID));
    wrap_execvp(sendargs);
  }
  /* parent */
  switch (wrap_waitpid(child)) {
      case 0:
        _exit(0);
      default:
        strerr_die2x(111,FATAL,MSG(ERR_CRONTAB));
  }
}
