/*$Id$*/

/* Copies a file relative the current directory and substitutes    */
/* !A at the beginning of a line for the target,                   */
/* !R at the beginning of a line for the confirm reply address,    */
/* The following substitutions are also made. If not set, ?????    */
/* will be printed:  <#l#> outlocal                                */
/* will be printed:  <#h#> outhost                                 */
/* will be printed:  <#n#> outmsgnum                               */
/* Other tags are killed, e.g. removed. A missing file is a        */
/* permanent error so owner finds out ASAP. May not have access to */
/* maillog. Content transfer encoding is done for 'B' and 'Q'. For */
/* 'H' no content transfer encoding is done, but blank lines are   */
/* suppressed. Behavior for other codes is undefined. This includes*/
/* lower case 'q'/'b'! If code is 'H' substitution of target and   */
/* verptarget is prevented as it may create illegal headers.       */

#include <unistd.h>
#include "stralloc.h"
#include "substdio.h"
#include "strerr.h"
#include "str.h"
#include "getln.h"
#include "case.h"
#include "readwrite.h"
#include "qmail.h"
#include "errtxt.h"
#include "error.h"
#include "quote.h"
#include "copy.h"
#include "mime.h"
#include "open.h"
#include "byte.h"
#include "die.h"
#include "idx.h"
#include "config.h"

static stralloc line = {0};
static stralloc outline = {0};
static stralloc qline = {0};
static stralloc outlocal = {0};
static stralloc outhost = {0};
static substdio sstext;
static char textbuf[256];
static const char *target = "?????";
static const char *verptarget = "?????";
static const char *confirm = "?????";
static unsigned int confirmlocal;
static const char *szmsgnum = "?????";

void set_cpoutlocal(const stralloc *ln)
{	/* must be quoted for safety. Note that substitutions that use */
	/* outlocal within an atom may create illegal addresses */
  if (!quote(&outlocal,ln))
    die_nomem();
}

void set_cpouthost(const stralloc *ln)
{
  if (!stralloc_copy(&outhost,ln))
    die_nomem();
}

void set_cptarget(const char *tg)
{
  target = tg;
}

void set_cpverptarget(const char *tg)
{
  verptarget = tg;
}

void set_cpconfirm(const char *cf)
{
  confirm = cf;
  confirmlocal = str_chr(cf, '@');
}

void set_cpnum(const char *cf)
{
  szmsgnum = cf;
}

static struct qmail *qq;

static void codeput(const char *l,unsigned int n,char code)
{
  if (!code || code == 'H')
    qmail_put(qq,l,n);
  else {
    if (code == 'Q')
      encodeQ(l,n,&qline);
    else
      encodeB(l,n,&qline,0);
    qmail_put(qq,qline.s,qline.len);
  }
}

static void codeputs(const char *l,char code)
{
  codeput(l,str_len(l),code);
}

static int try_open(const char *fn)
{
  int fd;
  if ((fd = open_read(fn)) == -1) {
    if (errno != error_noent)
      strerr_die4sys(111,FATAL,ERR_OPEN,fn,": ");
    if (ezmlmrc.len != 0) {
      if (!stralloc_copy(&line,&ezmlmrc)) die_nomem();
      if (!stralloc_append(&line,"/")) die_nomem();
      if (!stralloc_cats(&line,fn)) die_nomem();
      if (!stralloc_0(&line)) die_nomem();
      if ((fd = open_read(line.s)) == -1)
	if (errno != error_noent)
	  strerr_die4sys(111,FATAL,ERR_OPEN,line.s,": ");
    }
  }
  if (fd == -1)
    strerr_die4sys(100,FATAL,ERR_OPEN,fn,": ");
  return fd;
}

/* Find tags <#x#>. Replace with for x=R confirm, for x=A */
/* target, x=l outlocal, x=h outhost. For others, just    */
/* skip tag. If outlocal/outhost are not set, the tags are*/
/* skipped. If confirm/taget are not set, the tags are    */
/* replaced by "???????" */
void copy_xlate(stralloc *out,
		const stralloc *line,
		char q)
{
  unsigned int pos;
  unsigned int nextpos;

  pos = 0;
  nextpos = 0;
  out->len = 0;
  while ((pos += byte_chr(line->s+pos,line->len-pos,'<')) < line->len) {
    if (pos + 4 < line->len &&
	line->s[pos+1] == '#' &&
	line->s[pos+3] == '#' &&
	line->s[pos+4] == '>') {
      /* tag. Copy first part of line */
      if (!stralloc_catb(out,line->s+nextpos,pos-nextpos))
	die_nomem();
      switch(line->s[pos+2]) {
      case 'A':
	if (q == 'H') strerr_die1x(111,ERR_SUBST_UNSAFE);
	if (!stralloc_cats(out,target)) die_nomem();
	break;
      case 'L':
	if (!stralloc_cat(out,&local)) die_nomem();
	break;
      case 'R':
	if (!stralloc_cats(out,confirm)) die_nomem();
	break;
      case 'r':
	if (!stralloc_catb(out,confirm,confirmlocal)) die_nomem();
	break;
      case 'l':
	if (!stralloc_cat(out,&outlocal)) die_nomem();
	break;
      case 'h':
      case 'H':
	if (!stralloc_cat(out,&outhost)) die_nomem();
	break;
      case 't':
	if (q == 'H') strerr_die1x(111,ERR_SUBST_UNSAFE);
	if (!stralloc_cats(out,verptarget)) die_nomem();
	break;
      case 'n':
	if (!stralloc_cats(out,szmsgnum)) die_nomem();
	break;
      default:
	break;			/* unknown tags killed */
      }
      pos += 5;
      nextpos = pos;
    } else
      ++pos;				/* try next position */
  }
  if (!stralloc_catb(out,line->s+nextpos,line->len-nextpos))
    die_nomem();		/* remainder */
}

void copy(struct qmail *qqp,
	  const char *fn,	/* text file name */
	  char q)		/* '\0' for regular output, 'B' for base64, */
				/* 'Q' for quoted printable,'H' for header  */
{
  int fd;
  int flagsmatched = 1;
  int match;
  unsigned int pos;

  qq = qqp;
  fd = try_open(fn);
  substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));
  for (;;) {
    if (getln(&sstext,&line,&match,'\n') == -1)
      strerr_die4sys(111,FATAL,ERR_READ,fn,": ");
    if (line.len > 0) {		/* line.len is always > 0 if match is true */
      if (line.s[0] == '#') continue;
      /* suppress blank line for 'H'eader mode */
      if (line.len == 1 && q == 'H') continue;
      if (line.s[0] == '!') {
	if (line.s[1] == 'R') {
	  codeput("   ",3,q);
	  codeputs(confirm,q);
	  codeput("\n",1,q);
	  continue;
	}
	if (line.s[1] == 'A') {
	  codeput("   ",3,q);
	  codeputs(target,q);
	  codeput("\n",1,q);
	  continue;
	}
      }
      /* Find <=flags=> tags */
      if (q != 'H'
	  && line.len >= 5
	  && line.s[0] == '<'
	  && line.s[1] == '='
	  && line.s[line.len-3] == '='
	  && line.s[line.len-2] == '>') {
	for (flagsmatched = 1, pos = 2; pos < line.len - 3; ++pos) {
	  const char ch = line.s[pos];
	  if ((ch >= 'A' && ch <= 'Z' && flags[ch - 'A'])
	      || (ch >= 'a' && ch <= 'z' && !flags[ch - 'a'])) {
	    flagsmatched = 0;
	    break;
	  }
	}
	continue;
      }
      if (!flagsmatched) continue;

      copy_xlate(&outline,&line,q);
      codeput(outline.s,outline.len,q);

      /* Last line is missing its trailing newline, add one on output. */
      if (!match)
	codeput("\n",1,q);
    } else
      break;
  }
  close(fd);
}

