/*Id: ezmlm-make.c,v 1.31 1997/12/08 23:44:02 lindberg Exp lindberg $*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include "sgetopt.h"
#include "stralloc.h"
#include "strerr.h"
#include "exit.h"
#include "readwrite.h"
#include "byte.h"
#include "open.h"
#include "substdio.h"
#include "str.h"
#include "auto_bin.h"
#include "getln.h"
#include "error.h"
#include "lock.h"
#include "errtxt.h"
#include "die.h"
#include "idx.h"
#include "auto_etc.h"
#include "auto_version.h"

			/* defaults. All other flags are false = 0 */
const char *defflags="ap";	/* archived list -a */
				/* public list -p */
				/* no ezmlm-archive -I */
				/* no text edit for remote admin -D */
				/* not in edit mode -E */
				/* no subs list for remote admin -L */
				/* no remote admin -R */
				/* no message moderation -M */
				/* no subscription moderation -S */
				/* don't use .ezmlmrc from dot-file dir -C */
				/* no prefix -F */
				/* no trailer -T */

#define NO_FLAGS ('z' - 'a' + 1)
static int flags[NO_FLAGS];	/* holds flags */

static stralloc popt[10] = {{0}};
static stralloc dotplus = {0};
static stralloc dirplus = {0};
static stralloc line = {0};

const char FATAL[] = "ezmlm-make: fatal: ";
const char WARNING[] = "ezmlm-make: warning: ";
const char USAGE[] =
"ezmlm-make: usage: ezmlm-make [-+] [ -a..zA..Z03..9 ] dir dot local host";

void die_relative(void)
{
  strerr_die2x(100,FATAL,ERR_SLASH);
}
void die_newline(void)
{
  strerr_die2x(100,FATAL,ERR_NEWLINE);
}
void die_quote(void)
{
  strerr_die2x(100,FATAL,ERR_QUOTE);
}

void die_read(void)
{
  strerr_die4sys(111,FATAL,ERR_READ,dirplus.s,": ");
}

static stralloc outline = {0};
static substdio sstext;
static char textbuf[1024];

static stralloc fname = {0};	/* file name */
static stralloc oldfname = {0};	/* file name from prevoius tag */
static stralloc dname = {0};	/* directory name */
static stralloc lname = {0};	/* link name */
static stralloc template = {0};	/* template file name */
static stralloc f = {0};
static stralloc key = {0};
static struct timeval tv;
static char sz[2] = "?";

void keyadd(unsigned long u)
{
  char ch;
  ch = (char) u; if (!stralloc_append(&key,&ch)) die_nomem(); u >>= 8;
  ch = (char) u; if (!stralloc_append(&key,&ch)) die_nomem(); u >>= 8;
  ch = (char) u; if (!stralloc_append(&key,&ch)) die_nomem(); u >>= 8;
  ch = (char) u; if (!stralloc_append(&key,&ch)) die_nomem();
}

void keyaddtime(void)
{
  gettimeofday(&tv,(struct timezone *) 0);
  keyadd(tv.tv_usec);
}

static stralloc dir = {0};
static stralloc dot = {0};
static stralloc local = {0};
static stralloc host = {0};

static unsigned long euid;
static stralloc cfname = {0};	/* config file if spec as -C cf_file */
static stralloc code = {0};
static stralloc oldflags = {0};
static int usecfg = 0;

void dirplusmake(const char *slash)
{
  if (!stralloc_copy(&dirplus,&dir)) die_nomem();
  if (!stralloc_cats(&dirplus,slash)) die_nomem();
  if (!stralloc_0(&dirplus)) die_nomem();
}

void linkdotdir(const char *dash,const char *slash)
{
  if (!stralloc_copy(&dotplus,&dot)) die_nomem();
  if (!stralloc_cats(&dotplus,dash)) die_nomem();
  if (!stralloc_0(&dotplus)) die_nomem();
  dirplusmake(slash);
  if (flags['e' - 'a'])
    if (unlink(dotplus.s) == -1)
      if (errno != error_noent)
        strerr_die4x(111,FATAL,ERR_DELETE,dotplus.s,": ");
  if (symlink(dirplus.s,dotplus.s) == -1)
    strerr_die4sys(111,FATAL,ERR_CREATE,dotplus.s,": ");
  keyaddtime();
}

void dcreate(const char *slash)
{
  dirplusmake(slash);
  if (mkdir(dirplus.s,0755) == -1)
    if ((errno != error_exist) || !flags['e' - 'a'])
      strerr_die4sys(111,FATAL,ERR_CREATE,dirplus.s,": ");
  keyaddtime();
}

substdio ss;
char ssbuf[SUBSTDIO_OUTSIZE];

void f_open(const char *slash)
{
  int fd;

  dirplusmake(slash);
  fd = open_trunc(dirplus.s);
  if (fd == -1)
    strerr_die4sys(111,FATAL,ERR_CREATE,dirplus.s,": ");

  substdio_fdbuf(&ss,write,fd,ssbuf,sizeof(ssbuf));
}

void f_put(const char *buf,unsigned int len)
{
  if (substdio_bput(&ss,buf,len) == -1)
    strerr_die4sys(111,FATAL,ERR_WRITE,dirplus.s,": ");
}
void f_puts(const char *buf)
{
  if (substdio_bputs(&ss,buf) == -1)
    strerr_die4sys(111,FATAL,ERR_WRITE,dirplus.s,": ");
}

void f_close(void)
{
  if (substdio_flush(&ss) == -1)
    strerr_die4sys(111,FATAL,ERR_FLUSH,dirplus.s,": ");
  if (fsync(ss.fd) == -1)
    strerr_die4sys(111,FATAL,ERR_SYNC,dirplus.s,": ");
  if (close(ss.fd) == -1) /* NFS stupidity */
    strerr_die4sys(111,FATAL,ERR_CLOSE,dirplus.s,": ");
  keyaddtime();
}

void frm(const char *slash)
{
  dirplusmake(slash);
  if (unlink(dirplus.s) == -1)
    if (errno != error_noent)
    strerr_die4sys(111,FATAL,ERR_DELETE,dirplus.s,": ");
}

int read_line(const char *dpm,stralloc *sa)
{
  int fdin;
  int match;
  dirplusmake(dpm);
  if ((fdin = open_read(dirplus.s)) == -1) {
    if (errno != error_noent) die_read();
    return -1;
  } else {
    substdio_fdbuf(&sstext,read,fdin,textbuf,sizeof(textbuf));
    if (getln(&sstext,sa,&match,'\n') == -1) die_read();
    sa->len -= match;
    close(fdin);
    return 0;
  }
}
  
int read_new_config(void)
{
  if (read_line("/flags",&oldflags) != 0) return 0;
  if (euid > 0 && !flags['c' - 'a'] && (cfname.len != 0))
    read_line("/ezmlmrc",&cfname);
  read_line("/dot",&dot);
  read_line("/outlocal",&local);
  read_line("/outhost",&host);
  read_line("/digestcode",&code);
  read_line("/sublist",&popt[0]);
  read_line("/fromheader",&popt[3]);
  read_line("/tstdigopts",&popt[4]);
  read_line("/owner",&popt[5]);
  read_line("/sql",&popt[6]);
  read_line("/modpost",&popt[7]);
  read_line("/modsub",&popt[8]);
  read_line("/remote",&popt[9]);
  return 1;
}

int read_old_config(void)
{
  unsigned char ch;
  int fdin;
  int match;

  /* for edit, try to get args from dir/config */
  dirplusmake("/config");
  if ((fdin = open_read(dirplus.s)) == -1) {
    if (errno != error_noent) die_read();
    return 0;
  } else {
    substdio_fdbuf(&sstext,read,fdin,textbuf,sizeof(textbuf));
    for (;;) {
      if (getln(&sstext,&line,&match,'\n') == -1) die_read();
      if (!match) break;
      if (line.s[0] == '#') continue;
      if (line.len == 1) break;
      if (line.s[1] != ':') break;
      line.s[line.len - 1] = '\0';
      switch (ch = line.s[0]) {
      case 'X':
	if (euid > 0 && !flags['c' - 'a'] && (cfname.len > 0))
	  if (!stralloc_copys(&cfname,line.s+2)) die_nomem();
	break;	/* for safety: ignore if root */
      case 'T': if (!stralloc_copys(&dot,line.s+2)) die_nomem(); break;
      case 'L': if (!stralloc_copys(&local,line.s+2)) die_nomem(); break;
      case 'H': if (!stralloc_copys(&host,line.s+2)) die_nomem(); break;
      case 'C': if (!stralloc_copys(&code,line.s+2)) die_nomem(); break;
      case 'F': if (!stralloc_copys(&oldflags,line.s+2)) die_nomem(); break;
      case 'D': break;	/* no reason to check */
      default:
	if (ch == '0' || (ch >= '3' && ch <= '9')) {
	  if (usecfg && popt[ch - '0'].len == 0)
	    if (!stralloc_copys(&popt[ch-'0'],line.s+2)) die_nomem();
	} else
	  strerr_die4x(111,FATAL,dirplus.s,ERR_SYNTAX,line.s+2);
	break;
      }
    }
    close(fdin);
  }
  return 1;
}

static int open_template(stralloc *fn)
{
  int fd;
  struct stat st;
  if (!stralloc_0(fn)) die_nomem();
  if (stat(fn->s,&st) == -1)
    strerr_die4sys(111,FATAL,ERR_STAT,fn->s,": ");
  if (S_ISDIR(st.st_mode)) {
    --fn->len;
    if (!stralloc_cats(fn,TXT_EZMLMRC)) die_nomem();
    if (!stralloc_0(fn)) die_nomem();
  }
  if ((fd = open_read(fn->s)) == -1) {
    if (errno != error_noent)
      strerr_die4sys(111,FATAL,ERR_OPEN,fn->s,": ");
    else
      strerr_die3x(100,FATAL,template.s,ERR_NOEXIST);
  }
  return fd;
}

void main(int argc,char **argv)
{
  int opt;
  int flagdo;
  int flagnot;
  int flagover;
  int flagnotexist = 0;
  int flagforce = 0;
  int flagforce_p = 0;
  int match;
  unsigned int next,i,j;
  int last;
  unsigned int slpos,hashpos,pos;
  int fdin,fdlock,fdtmp;
  char *p;
  unsigned char ch;

  keyadd((unsigned long) getpid());
  keyadd((unsigned long) getppid());
  euid = (unsigned long) geteuid();
  keyadd(euid);
  keyadd((unsigned long) getgid());
  gettimeofday(&tv,(struct timezone *) 0);
  keyadd(tv.tv_sec);

  (void) umask(077);
	/* flags with defined use. vV for version. Others free */

  for (pos = 0; pos < (unsigned int) NO_FLAGS; pos++) {
    flags[pos] = 0;
  }
  for (pos = 0; pos < 10; popt[pos++].len = 0);

  while ((opt = getopt(argc,argv,
   "+aAbBcC:dDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ0:3:4:5:6:7:8:9:"))
           != opteof) {
    if (opt == 'v' || opt == 'V')
      strerr_die2x(0,"ezmlm-make version: ",auto_version);
    if (opt =='C')	/* treat this like nl switch to allow override of -c*/
      if (!stralloc_copys(&cfname,optarg)) die_nomem();
    if (opt >= 'a' && opt <= 'z') {
      flags[opt - 'a'] = 3;		/* Dominant "set" */
      if (opt == 'e') flagforce++;	/* two 'e' => ignore 'E' */
    } else if (opt >= 'A' && opt <= 'Z')
      flags[opt - 'A'] = 2;		/* Dominant "unset" */
    else if (opt >= '0' && opt <= '9') {
      if (!stralloc_copys(&popt[opt-'0'],optarg)) die_nomem();
    }
    else if (opt == '+') {
      flagforce_p++;		/* two '+' => ignore 'E' */
      flags['e' - 'a'] = 3;	/* -+ implies -e */
      usecfg = 1;
    } else
      die_usage();
  }
  argv += optind;

  if (flagforce_p > 1 || flagforce > 1)
    flagforce = 1;
  else
    flagforce = 0;

  if (*argv == 0) die_usage();
  if (!stralloc_copys(&dir,*argv++)) die_nomem();
  if (dir.s[0] != '/') die_relative();
  if (byte_chr(dir.s,dir.len,'\'') < dir.len) die_quote();
  if (byte_chr(dir.s,dir.len,'\n') < dir.len) die_newline();

  if (flags['e' - 'a'] & 1) {
    /* lock for edit */
    dirplusmake("/lock");
    fdlock = lockfile(dirplus.s);
    read_new_config() || read_old_config();
  }

  if ((p = *argv++) != 0) {
    if (!stralloc_copys(&dot,p)) die_nomem();
    if ((p = *argv++) != 0) {
      if (local.len == 0 || str_diff(local.s,p))
	flagforce = 1;		/* must rewrite if list name changed */
      if (!stralloc_copys(&local,p)) die_nomem();
      if ((p = *argv++) != 0) {
	if (host.len == 0 || str_diff(host.s,p))
	  flagforce = 1;	/* must rewrite if list name changed */
	if (!stralloc_copys(&host,p)) die_nomem();
	if ((p = *argv++) != 0) {
	  if (!stralloc_copys(&code,p)) die_nomem();
        }
      }
    }
  }
  if (dot.len == 0 || local.len == 0 || host.len == 0) die_usage();
  if (dot.s[0] != '/') die_relative();		/* force absolute dot */

			/* use flags from config, overridden with new values */
			/* if there are old flags, we're in "edit" and "-+" */
			/* Previous versions only wrote _set_ flags to */
			/* to DIR/confiag. We need to make sure that we */
			/* don't apply the defaults for non-specified ones! */
  if (usecfg && oldflags.len > 0 && flags['e' - 'a']) {
    for (p = oldflags.s, i = oldflags.len; ch = *p, i > 0; ++p, --i) {
      if (ch >= 'a' && ch <= 'z') {		/* unset flags ignored */
        if (ch != 'e')
          if (!flags[ch - 'a'])			/* cmd line overrides */
	    flags[ch - 'a'] = 1;
      }
    }
  }

  if (!usecfg) {				/* apply defaults */
    while (( ch = *(defflags++))) {		/* gets used up! */
      if (ch >= 'a' && ch <= 'z') {		/* defensive! */
	if (!flags[ch - 'a'])			/* cmdline still overrides */
	  flags[ch - 'a'] = 1;
      }
    }
  }

  for (pos = 0; pos < (unsigned int) NO_FLAGS; pos++) {	/* set real flags */
    if (flags[pos] & 2)				/* 2 = "dominant" 0 */
      flags[pos] = flags[pos] & 1;		/* 3 = "dominant" 1 */
  }

  if (byte_chr(local.s,local.len,'\n') < local.len) die_newline();
  if (byte_chr(host.s,host.len,'\n') < host.len) die_newline();

	/* build 'f' for <#F#> */
  if (!stralloc_ready(&f,28)) die_nomem();
  if (!stralloc_copys(&f,"-")) die_nomem();
  for (ch = 0; ch <= 'z' - 'a'; ch++) {		/* build string with flags */
    if (flags[ch])
      sz[0] = 'a' + ch;
    else
      sz[0] = 'A' + ch;
    if (!stralloc_append(&f,sz)) die_nomem();
  }

  fdin = -1;	/* assure failure for .ezmlmrc in case flags['c'-'a'] = 0 */
  slpos = dot.len;
  while ((--slpos > 0) && dot.s[slpos] != '/');
  if (dot.s[slpos] == '/') {
    if (!stralloc_copyb(&template,dot.s,slpos+1)) die_nomem();	/* dot dir */
    slpos += byte_chr(dot.s+slpos,dot.len-slpos,'-');
    if (dot.s[slpos]) {
      slpos++;
      if (slpos < dot.len
	  && (pos = slpos+byte_chr(dot.s+slpos,dot.len-slpos,'-')) < dot.len) {
        if (!stralloc_copyb(&popt[1],dot.s+slpos,pos-slpos)) die_nomem();
        pos++;
	if (pos < dot.len
	    && (slpos = pos+byte_chr(dot.s+pos,dot.len-pos,'-')) < dot.len)
          if (!stralloc_copyb(&popt[2],dot.s+pos,slpos-pos)) die_nomem();
      }
    }
  }
	/* if 'c', template already has the dot directory. If 'C', cfname */
	/* (if exists and != '') points to the file name to use instead. */
  if (flags['c'-'a'] || (cfname.len > 0)) {
    if (!flags['c'-'a']) {	/* i.e. there is a cfname specified */
      if (!stralloc_copy(&template,&cfname)) die_nomem();
    } else
      if (!stralloc_cats(&template,TXT_DOTEZMLMRC)) die_nomem();
    fdin = open_template(&template);
  } else {
    /* /etc/ezmlm/default/ezmlmrc */
    if (!stralloc_copys(&template,auto_etc)) die_nomem();
    if (!stralloc_cats(&template,TXT_DEFAULT)) die_nomem();
    fdin = open_template(&template);
  }

  dcreate("");		/* This is all we do, the rest is up to ezmlmrc */
			/* do it after opening template to avoid aborts */
			/* with created DIR. Well we also write DIR/key */
			/* at the end except in -e[dit] mode.           */

  substdio_fdbuf(&sstext,read,fdin,textbuf,sizeof(textbuf));
  if (!stralloc_0(&oldfname)) die_nomem();		/* init oldfname */
  flagdo = 0;

  if (getln(&sstext,&line,&match,'\n') == -1)
    strerr_die4sys(111,FATAL,ERR_READ,template.s,": ");
  if (!match)
    strerr_die4sys(111,FATAL,ERR_READ,template.s,": ");
  i = str_rchr(auto_version,'-');			/* check version */
  if (auto_version[i]) i++;
  j = 0;
  while (line.s[j] == auto_version[i] && j < line.len &&
		auto_version[i] != '.' && auto_version[i]) {
    i++; j++;						/* major */
  }							/* first minor */
  if (auto_version[i] != '.' || j + 1 >= line.len ||
		auto_version[i+1] != line.s[j+1])
    strerr_warn2(WARNING,ERR_VERSION, (struct strerr *) 0);

  for (;;) {
    if (getln(&sstext,&line,&match,'\n') == -1)
      strerr_die4sys(111,FATAL,ERR_READ,template.s,": ");
    if (!match)
      break;
    if (line.s[0] == '#')				/* comment */
      continue;
    if (!stralloc_0(&line)) die_nomem();
    if (line.s[0] == '<' && line.s[1] == '/') {		/* tag */
      if (byte_chr(line.s,line.len,'.') < line.len)
	strerr_die3x(100,FATAL,ERR_PERIOD,line.s);
      flagdo = 1;
      flagover = 0;
      hashpos = 0;
      pos = byte_chr(line.s+2,line.len-2,'#')+2;
      if (pos < line.len-2) {
        hashpos = pos;
        pos++;
        flagnot = 0;
        while (pos < line.len
	       && ((ch = line.s[pos]) != '/' && line.s[pos+1] != '>')) {
          if (ch == '^') {
            flagnot = 1;
            pos++;
            continue;
          }
			/* E is ignored. For files => create unless exists */
	  if ((ch == 'E' && !flagnot) || (ch == 'e' && flagnot)) {
	    if (flags['e' - 'a'] && !flagforce)
	      flagover = 1;		/* ignore #E & #^e, but set flagover */
          } else if (ch >= 'a' && ch <= 'z')
            flagdo &= (flags[ch - 'a'] ^ flagnot);
          else if (ch >= 'A' && ch <= 'Z')
            flagdo &= !(flags[ch - 'A'] ^ flagnot);
          else if (ch >= '0' && ch <= '9')
            flagdo &= (popt[ch - '0'].len > 0) ^flagnot;
          flagnot = 0;
          pos++;
        }
        if (pos < line.len
	    && (line.s[pos] != '/' || line.s[pos+1] != '>'))
          strerr_die3x(100,FATAL,ERR_ENDTAG,line.s);
      } else {
        flagdo = 1;
        pos = 2;	/* name needs to be >= 1 char */
        while ((pos = byte_chr(line.s+pos,line.len-pos,'/')+pos) < line.len) {
          if (line.s[pos+1] == '>')
            break;
          pos++;
        }
        if (pos >= line.len)
          strerr_die3x(100,FATAL,ERR_ENDTAG,line.s);
      }
      if (hashpos)
        pos = hashpos;	/* points to after file name */

      if (line.s[2] == '+') {			/* mkdir */
        if (!flagdo)
          continue;
        if (!stralloc_copys(&dname,"/")) die_nomem();
        if (!stralloc_catb(&dname,line.s+3,pos-3)) die_nomem();
        if (!stralloc_0(&dname)) die_nomem();
        dcreate(dname.s);
        flagdo = 0;
        continue;
      } else if (line.s[2] == ':') {		/* ln -s */
        if (!flagdo)
          continue;
        slpos = byte_chr(line.s+3,line.len-3,'/') + 3;
        if (slpos >= pos)
          strerr_die3x(100,FATAL,ERR_LINKDIR,line.s);
        if (!stralloc_copyb(&dname,line.s+slpos,pos-slpos)) die_nomem();
        if (!stralloc_copyb(&lname,line.s+3,slpos-3)) die_nomem();
        if (!stralloc_0(&dname)) die_nomem();
        if (!stralloc_0(&lname)) die_nomem();
        linkdotdir(lname.s,dname.s);
        flagdo = 0;
        continue;
      } else if (line.s[2] == '-') {		/* rm */
        if (!flagdo)
          continue;
        if (!stralloc_copys(&dname,"/")) die_nomem();
        if (!stralloc_catb(&dname,line.s+3,pos-3)) die_nomem();
        if (!stralloc_0(&dname)) die_nomem();
        frm(dname.s);
        flagdo = 0;
        continue;
      }
						/* only plain files left */
						/* first get file name */
      if (pos > 2) {			/* </#ai/> => add to open file */
        if (!stralloc_copyb(&fname,line.s+1,pos-1)) die_nomem();
        if (!stralloc_0(&fname)) die_nomem();
      }

      if (str_diff(fname.s, oldfname.s)) {
	flagnotexist = 1;
			/* Treat special case of #E when editing which _should*/
			/* write only if the file does not exist. flagover */
			/* is set if we need to check */
        if (flagover) {	/* skip if exists */
	  dirplusmake(fname.s);		/* decided by FIRST tag for file */
	  fdtmp = open_read(dirplus.s);
	  if (fdtmp == -1) {
	    if (errno != error_noent)
	      strerr_die3sys(111,ERR_OPEN,dirplus.s,": ");
          } else {
	    flagnotexist = 0;		/* already there - don't do it */
	    close(fdtmp);
	  }
        }
        if (oldfname.len > 1) {
          f_close();
          if (!stralloc_copys(&oldfname,"")) die_nomem();
          if (!stralloc_0(&oldfname)) die_nomem();
          }
          if (flagdo && flagnotexist) {
            if (!fname.len)
              strerr_die3x(100,FATAL,ERR_FILENAME,line.s);
            f_open(fname.s);
           if (!stralloc_copy(&oldfname,&fname)) die_nomem();
          }
        }
	if (flagdo) flagdo = flagnotexist;
        continue;
    } else if (!flagdo)
      continue;			/* part not to go out */
    last = -1;
    next = 0;
    outline.len = 0;
    for (;;) {
      if (next < line.len &&
	  (pos = next + byte_chr(line.s+next,line.len-next,'<')) < line.len &&
          line.s[pos+1] == '#' &&
          line.s[pos+2] &&
          line.s[pos+3] == '#' &&
          line.s[pos+4] == '>') {	/* host/local */
        if (!stralloc_catb(&outline,line.s+last+1,pos-last-1))
                die_nomem();
        switch (line.s[pos+2]) {
          case 'B':		/* path to ezmlm binaries (no trailing /) */
            if (!stralloc_cats(&outline,auto_bin)) die_nomem();
            last = pos + 4; next = pos + 5; break;
          case 'C':		/* digestcode */
	    if (!stralloc_cat(&outline,&code)) die_nomem();
            last = pos + 4; next = pos + 5; break;
          case 'D':		/* listdir */
            if (!stralloc_cat(&outline,&dir)) die_nomem();
            last = pos + 4; next = pos + 5; break;
          case 'F':		/* flags */
            if (!stralloc_cat(&outline,&f)) die_nomem();
            last = pos + 4; next = pos + 5; break;
          case 'H':		/* hostname */
            if (!stralloc_cat(&outline,&host)) die_nomem();
            last = pos + 4; next = pos + 5; break;
          case 'L':		/* local */
            if (!stralloc_cat(&outline,&local)) die_nomem();
            last = pos + 4; next = pos + 5; break;
          case 'T':		/* dot */
            if (!stralloc_cat(&outline,&dot)) die_nomem();
            last = pos + 4; next = pos + 5; break;
          case 'X':		/* config file name */
	    if (!stralloc_cat(&outline,&cfname)) die_nomem();
            last = pos + 4; next = pos + 5; break;
          default:		/* copy unknown tag as is for e.g. <#A#> and*/
				/* <#R#> to be processed by -manage/store   */
                                /* stuff in args for <#0#> .. <#9#> */
            if ((line.s[pos+2] >= '0') && (line.s[pos+2] <= '9')) {
	      if (!stralloc_cat(&outline,&popt[line.s[pos+2]-'0']))
		die_nomem();
            } else
              if (!stralloc_catb(&outline,line.s+pos,5)) die_nomem();
            last = pos + 4; next = pos + 5; break;
        }
      } else {			/* not tag */
	if (pos < line.len) {
          next++;
        } else {
          if (!stralloc_catb(&outline,line.s+last+1,line.len-last-1))
            die_nomem();
          f_puts(outline.s);
          break;
        }
      }
    }
  }

  close(fdin);
  if (oldfname.len > 1)
    f_close();

  if (!flags['e' - 'a']) {	/* don't redo key when editing a list */
    f_open("/key");
    f_put(key.s,key.len);
    f_close();
  }
  _exit(0);
}

