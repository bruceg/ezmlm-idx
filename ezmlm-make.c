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
#include "open.h"
#include "substdio.h"
#include "str.h"
#include "auto_bin.h"
#include "getln.h"
#include "error.h"
#include "lock.h"
#include "errtxt.h"
#include "idx.h"
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
int flags[NO_FLAGS];		/* holds flags */

const char *popt[10];
stralloc dotplus = {0};
stralloc dirplus = {0};
stralloc line = {0};

const char FATAL[] = "ezmlm-make: fatal: ";
#define WARNING "ezmlm-make: warning: "

void die_usage()
{
 strerr_die1x(100,
  "ezmlm-make: usage: ezmlm-make [-+] [ -a..zA..Z03..9 ] dir dot local host");
}
void die_relative()
{
  strerr_die2x(100,FATAL,ERR_SLASH);
}
void die_newline()
{
  strerr_die2x(100,FATAL,ERR_NEWLINE);
}
void die_quote()
{
  strerr_die2x(100,FATAL,ERR_QUOTE);
}
void die_nomem()
{
  strerr_die2x(111,FATAL,ERR_NOMEM);
}

void die_read()
{
  strerr_die4sys(111,FATAL,ERR_READ,dirplus.s,": ");
}

stralloc cmdline = {0};
stralloc outline = {0};
substdio sstext;
char textbuf[1024];

stralloc fname = {0};		/* file name */
stralloc oldfname = {0};	/* file name from prevoius tag */
stralloc dname = {0};		/* directory name */
stralloc lname = {0};		/* link name */
stralloc template = {0};	/* template file name */
stralloc ext1 = {0};		/* dot = dir/.qmail-ext1-ext2-list */
stralloc ext2 = {0};
stralloc f = {0};
stralloc key = {0};
struct timeval tv;
char sz[2] = "?";

void keyadd(u)
unsigned long u;
{
  char ch;
  ch = (char) u; if (!stralloc_append(&key,&ch)) die_nomem(); u >>= 8;
  ch = (char) u; if (!stralloc_append(&key,&ch)) die_nomem(); u >>= 8;
  ch = (char) u; if (!stralloc_append(&key,&ch)) die_nomem(); u >>= 8;
  ch = (char) u; if (!stralloc_append(&key,&ch)) die_nomem();
}

void keyaddtime()
{
  gettimeofday(&tv,(struct timezone *) 0);
  keyadd(tv.tv_usec);
}

char *dir;
char *dot;
char *local = (char *) 0;
char *host = (char *) 0;

void dirplusmake(slash)
char *slash;
{
  if (!stralloc_copys(&dirplus,dir)) die_nomem();
  if (!stralloc_cats(&dirplus,slash)) die_nomem();
  if (!stralloc_0(&dirplus)) die_nomem();
}

void linkdotdir(dash,slash)
char *dash;
char *slash;
{
  if (!stralloc_copys(&dotplus,dot)) die_nomem();
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

void dcreate(slash)
char *slash;
{
  dirplusmake(slash);
  if (mkdir(dirplus.s,0755) == -1)
    if ((errno != error_exist) || !flags['e' - 'a'])
      strerr_die4sys(111,FATAL,ERR_CREATE,dirplus.s,": ");
  keyaddtime();
}

substdio ss;
char ssbuf[SUBSTDIO_OUTSIZE];

void f_open(slash)
char *slash;
{
  int fd;

  dirplusmake(slash);
  fd = open_trunc(dirplus.s);
  if (fd == -1)
    strerr_die4sys(111,FATAL,ERR_CREATE,dirplus.s,": ");

  substdio_fdbuf(&ss,write,fd,ssbuf,sizeof(ssbuf));
}

void f_put(buf,len)
char *buf;
unsigned int len;
{
  if (substdio_bput(&ss,buf,len) == -1)
    strerr_die4sys(111,FATAL,ERR_WRITE,dirplus.s,": ");
}
void f_puts(buf)
char *buf;
{
  if (substdio_bputs(&ss,buf) == -1)
    strerr_die4sys(111,FATAL,ERR_WRITE,dirplus.s,": ");
}

void f_close()
{
  if (substdio_flush(&ss) == -1)
    strerr_die4sys(111,FATAL,ERR_FLUSH,dirplus.s,": ");
  if (fsync(ss.fd) == -1)
    strerr_die4sys(111,FATAL,ERR_SYNC,dirplus.s,": ");
  if (close(ss.fd) == -1) /* NFS stupidity */
    strerr_die4sys(111,FATAL,ERR_CLOSE,dirplus.s,": ");
  keyaddtime();
}

void frm(slash)
char *slash;
{
  dirplusmake(slash);
  if (unlink(dirplus.s) == -1)
    if (errno != error_noent)
    strerr_die4sys(111,FATAL,ERR_DELETE,dirplus.s,": ");
}


void main(argc,argv)
int argc;
char **argv;
{
  unsigned long euid;
  int opt;
  int flagdo;
  int flagnot;
  int flagover;
  int flagnotexist;
  int flagforce = 0;
  int flagforce_p = 0;
  int usecfg = 0;
  int match;
  unsigned int next,i,j;
  int last;
  unsigned int slpos,hashpos,pos;
  int fdin,fdlock,fdtmp;
  char *p;
  char *oldflags = (char *) 0;
  char *code = (char *) 0;
  const char *cfname = (char *) 0;	/* config file if spec as -C cf_file */
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
  for (pos = 0; pos < 10; popt[pos++] = (char *) 0);

  while ((opt = getopt(argc,argv,
   "+aAbBcC:dDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ0:3:4:5:6:7:8:9:"))
           != opteof) {
    if (opt == 'v' || opt == 'V')
      strerr_die2x(0,"ezmlm-make version: ",auto_version);
    if (opt =='C')	/* treat this like nl switch to allow override of -c*/
      cfname = optarg;
    if (opt >= 'a' && opt <= 'z') {
      flags[opt - 'a'] = 3;		/* Dominant "set" */
      if (opt == 'e') flagforce++;	/* two 'e' => ignore 'E' */
    } else if (opt >= 'A' && opt <= 'Z')
      flags[opt - 'A'] = 2;		/* Dominant "unset" */
    else if (opt >= '0' && opt <= '9')
      popt[opt-'0'] = optarg;
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

  if (!(dir = *argv++)) die_usage();
  if (dir[0] != '/') die_relative();
  if (dir[str_chr(dir,'\'')]) die_quote();
  if (dir[str_chr(dir,'\n')]) die_newline();

  if (flags['e' - 'a'] & 1) {	/* lock for edit */
    dirplusmake("/lock");
    fdlock = lockfile(dirplus.s);

				/* for edit, try to get args from dir/config */
    dirplusmake("/config");
    if ((fdin = open_read(dirplus.s)) == -1) {
      if (errno != error_noent) die_read();
    } else {
      substdio_fdbuf(&sstext,read,fdin,textbuf,sizeof(textbuf));
      for (;;) {
	if (getln(&sstext,&line,&match,'\n') == -1) die_read();
	if (!match) break;
	if (line.s[0] == '#') continue;
	if (line.len == 1) break;
	if (line.s[1] != ':') break;
	line.s[line.len - 1] = '\0';
	      if (!stralloc_cat(&cmdline,&line)) die_nomem();
      }
      close(fdin);
      pos = 0;
      while (pos < cmdline.len) {
	ch = cmdline.s[pos];
	pos += 2;
	switch (ch) {
	  case 'X': if (euid && !flags['c' - 'a'] && (!cfname))
			cfname = cmdline.s + pos;	/* cmdline overrides */
		    break;	/* for safety: ignore if root */
          case 'T': dot = cmdline.s + pos; break;
          case 'L': local = cmdline.s + pos; break;
          case 'H': host = cmdline.s + pos; break;
          case 'C': code = cmdline.s + pos; break;
          case 'D': break;	/* no reason to check */
          case 'F': oldflags = cmdline.s + pos; break;
          default:
                 if (ch == '0' || (ch >= '3' && ch <= '9')) {
                   if (usecfg && !popt[ch - '0'])
                     popt[ch - '0'] = cmdline.s + pos;
                 } else
                   strerr_die4x(111,FATAL,dirplus.s,ERR_SYNTAX,
                        cmdline.s+pos);
                 break;
        }
        pos += str_len(cmdline.s + pos) + 1;
      }
    }
  }

  if (p = *argv++) {
    dot = p;
    if (p = *argv++) {
      if (!local || str_diff(local,p))
	flagforce = 1;		/* must rewrite if list name changed */
      local = p;
      if (p = *argv++) {
	if (!host || str_diff(host,p))
	  flagforce = 1;	/* must rewrite if list name changed */
        host = p;
        if (p = *argv++) {
          code = p;
        }
      }
    }
  }
  if (!dot || !local || !host) die_usage();
  if (dot[0] != '/') die_relative();		/* force absolute dot */

			/* use flags from config, overridden with new values */
			/* if there are old flags, we're in "edit" and "-+" */
			/* Previous versions only wrote _set_ flags to */
			/* to DIR/confiag. We need to make sure that we */
			/* don't apply the defaults for non-specified ones! */
  if (usecfg && oldflags && flags['e' - 'a']) {
    while ((ch = *(oldflags++))) {
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

  if (local[str_chr(local,'\n')]) die_newline();
  if (host[str_chr(host,'\n')]) die_newline();

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
  slpos = str_len(dot);
  while ((--slpos > 0) && dot[slpos] != '/');
  if (dot[slpos] == '/') {
    if (!stralloc_copyb(&template,dot,slpos+1)) die_nomem();	/* dot dir */
    slpos += str_chr(dot+slpos,'-');
    if (dot[slpos]) {
      slpos++;
      pos = slpos + str_chr(dot+slpos,'-');
      if (dot[pos]) {
        if (!stralloc_copyb(&ext1,dot+slpos,pos-slpos)) die_nomem();
        pos++;
        slpos = pos + str_chr(dot+pos,'-');
        if (dot[slpos])
          if (!stralloc_copyb(&ext2,dot+pos,slpos-pos)) die_nomem();
      }
    }
  }
  if (!stralloc_0(&ext1)) die_nomem();
  if (!stralloc_0(&ext2)) die_nomem();
  popt[1] = ext1.s;
  popt[2] = ext2.s;
	/* if 'c', template already has the dot directory. If 'C', cfname */
	/* (if exists and != '') points to the file name to use instead. */
  if (flags['c'-'a'] || (cfname && *cfname)) {
    if (!flags['c'-'a']) {	/* i.e. there is a cfname specified */
      if (!stralloc_copys(&template,cfname)) die_nomem();
    } else
      if (!stralloc_cats(&template,TXT_DOTEZMLMRC)) die_nomem();
  if (!stralloc_0(&template)) die_nomem();
  if ((fdin = open_read(template.s)) == -1)
    if (errno != error_noent)
      strerr_die4sys(111,FATAL,ERR_OPEN,template.s,": ");
    else
      strerr_die3x(100,FATAL,template.s,ERR_NOEXIST);
  } else {			/* /etc/ezmlmrc */
    if (!stralloc_copys(&template,TXT_ETC_EZMLMRC)) die_nomem();
    if (!stralloc_0(&template)) die_nomem();
    if ((fdin = open_read(template.s)) == -1)
      if (errno != error_noent)
        strerr_die4sys(111,FATAL,ERR_OPEN,template.s,": ");
      else {			/* ezbin/ezmlmrc */
	if (!stralloc_copys(&template,auto_bin)) die_nomem();
	if (!stralloc_cats(&template,TXT_EZMLMRC)) die_nomem();
	if (!stralloc_0(&template)) die_nomem();
	if ((fdin = open_read(template.s)) == -1)
	  if (errno != error_noent)
	    strerr_die4sys(111,FATAL,ERR_OPEN,template.s,": ");
	  else
	    strerr_die3x(100,FATAL,template.s,ERR_NOEXIST);
      }
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
    if (line.s[str_chr(line.s,'.')])
      strerr_die3x(100,FATAL,ERR_PERIOD,line.s);
      flagdo = 1;
      flagover = 0;
      hashpos = 0;
      pos = str_chr(line.s+2,'#')+2;
      if (line.s[pos]) {
        hashpos = pos;
        pos++;
        flagnot = 0;
        while ((ch = line.s[pos]) &&
              (line.s[pos] != '/' && line.s[pos+1] != '>')) {
          if (ch == '^') {
            flagnot = 1;
            pos++;
            continue;
          }
			/* E is ignored. For files => create unless exists */
	  if (ch == 'E' && !flagnot ||  ch == 'e' && flagnot) {
		if (flags['e' - 'a'] && !flagforce)
	    flagover = 1;		/* ignore #E & #^e, but set flagover */
          } else if (ch >= 'a' && ch <= 'z')
            flagdo &= (flags[ch - 'a'] ^ flagnot);
          else if (ch >= 'A' && ch <= 'Z')
            flagdo &= !(flags[ch - 'A'] ^ flagnot);
          else if (ch >= '0' && ch <= '9')
            flagdo &= (popt[ch - '0'] && *popt[ch - '0']) ^flagnot;
          flagnot = 0;
          pos++;
        }
        if (line.s[pos] != '/' || line.s[pos+1] != '>')
          strerr_die3x(100,FATAL,ERR_ENDTAG,line.s);
      } else {
        flagdo = 1;
        pos = 2;	/* name needs to be >= 1 char */
        while (line.s[pos = str_chr(line.s+pos,'/')+pos]) {
          if (line.s[pos+1] == '>')
            break;
          pos++;
        }
        if (!line.s[pos])
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
        slpos = str_chr(line.s + 3,'/') + 3;
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
      pos = next + str_chr(line.s+next,'<');
      if (line.s[pos] &&
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
            if (code && *code)
              if (!stralloc_cats(&outline,code)) die_nomem();
            last = pos + 4; next = pos + 5; break;
          case 'D':		/* listdir */
            if (!stralloc_cats(&outline,dir)) die_nomem();
            last = pos + 4; next = pos + 5; break;
          case 'F':		/* flags */
            if (!stralloc_cat(&outline,&f)) die_nomem();
            last = pos + 4; next = pos + 5; break;
          case 'H':		/* hostname */
            if (!stralloc_cats(&outline,host)) die_nomem();
            last = pos + 4; next = pos + 5; break;
          case 'L':		/* local */
            if (!stralloc_cats(&outline,local)) die_nomem();
            last = pos + 4; next = pos + 5; break;
          case 'T':		/* dot */
            if (!stralloc_cats(&outline,dot)) die_nomem();
            last = pos + 4; next = pos + 5; break;
          case 'X':		/* config file name */
            if (cfname)
	      if (!stralloc_cats(&outline,cfname)) die_nomem();
            last = pos + 4; next = pos + 5; break;
          default:		/* copy unknown tag as is for e.g. <#A#> and*/
				/* <#R#> to be processed by -manage/store   */
                                /* stuff in args for <#0#> .. <#9#> */
            if ((line.s[pos+2] >= '0') && (line.s[pos+2] <= '9')) {
              if (popt[line.s[pos+2] - '0'])
                if (!stralloc_cats(&outline,popt[line.s[pos+2]-'0']))
                  die_nomem();
            } else
              if (!stralloc_catb(&outline,line.s+pos,5)) die_nomem();
            last = pos + 4; next = pos + 5; break;
        }
      } else {			/* not tag */
        if (line.s[pos]) {
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

