/* Copies a file relative the current directory and substitutes    */
/* !A at the beginning of a line for the target,                   */
/* !R at the beginning of a line for the confirm reply address,    */
/* The following substitutions are also made. If not set, ?????    */
/* will be printed:                                                */
/*   <#A#> target email address (<#a#>@<#h#>)                      */
/*   <#C#> The confirm cookie hash code                            */
/*   <#H#> outhost                                                 */
/*   <#L#> outlocal (unchanged)                                    */
/*   <#R#> confirm address (<#r#>@<#h#>)                           */
/*   <#T#> confirm time stamp                                      */
/*   <#X#> confirm action code                                     */
/*   <#a#> local part of the target address                        */
/*   <#c#> the confirm cookie (<#X#>c.<#T#>.<#C#>-<#a#>=<#h#>)     */
/*   <#d#> base directory name                                     */
/*   <#h#> outhost                                                 */
/*   <#l#> outlocal (modified for digest requests)                 */
/*   <#n#> outmsgnum                                               */
/*   <#r#> local part of the confirm address (<#l#>-<#c#>)         */
/*   <#t#> target address, with '@' substituted by '='             */
/*   <#0#>-<#9#> temporary parameters, used by some messages       */
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
#include "messages.h"
#include "error.h"
#include "quote.h"
#include "copy.h"
#include "mime.h"
#include "altpath.h"
#include "byte.h"
#include "cookie.h"
#include "datetime.h"
#include "die.h"
#include "fmt.h"
#include "idx.h"
#include "config.h"

static stralloc srcline = {0};
static stralloc outline = {0};
static stralloc qline = {0};
static substdio sstext;
static char textbuf[256];
static const char *target = "?????";
static unsigned int targetlocal;
static const char *verptarget = "?????";
static const char *confirm = "?????";
static const char *action = "??.";
static char hash[COOKIE] = "????????????????????";
static datetime_sec when = (datetime_sec)-1;
static unsigned int confirmlocal;
static unsigned int confirmprefix;
static const char *szmsgnum = "?????";

void set_cptarget(const char *tg)
{
  target = tg;
  targetlocal = str_chr(tg, '@');
}

void set_cpverptarget(const char *tg)
{
  verptarget = tg;
}

void set_cpconfirm(const char *cf,unsigned int prefixlen)
{
  confirm = cf;
  confirmlocal = str_chr(cf, '@');
  confirmprefix = prefixlen + 1;
}

void set_cpnum(const char *cf)
{
  szmsgnum = cf;
}

void set_cpaction(const char *ac)
{
  action = ac;
}

void set_cpwhen(datetime_sec t)
{
  when = t;
}

void set_cphash(const char h[COOKIE])
{
  byte_copy(hash,COOKIE,h);
}

void codeput(struct qmail *qq,const char *l,unsigned int n,char code)
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

void codeputs(struct qmail *qq,const char *l,char code)
{
  codeput(qq,l,str_len(l),code);
}

/* Find tags <#x#>. Replace with for x=R confirm, for x=A */
/* target, x=l outlocal, x=h outhost. For others, just    */
/* skip tag. If outlocal/outhost are not set, the tags are*/
/* skipped. If confirm/taget are not set, the tags are    */
/* replaced by "???????" */
void copy_xlate(stralloc *out,
		const stralloc *line,
		const char *params[10],
		char q)
{
  unsigned int pos;
  unsigned int nextpos;
  char strnum[FMT_ULONG];

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
	if (q == 'H') strerr_die1x(111,MSG(ERR_SUBST_UNSAFE));
	if (!stralloc_cats(out,target)) die_nomem();
	break;
      case 'C':
	if (!stralloc_catb(out,hash,COOKIE)) die_nomem();
	break;
      case 'L':
	if (!quote(&qline,&mainlocal)) die_nomem();
	if (!stralloc_cat(out,&qline)) die_nomem();
	break;
      case 'R':
	if (!stralloc_cats(out,confirm)) die_nomem();
	break;
      case 'T':
	if (!stralloc_catb(out,strnum,fmt_ulong(strnum,(unsigned long )when))) die_nomem();
	break;
      case 'X':
	if (!stralloc_cats(out,action)) die_nomem();
	break;
      case 'a':
	if (!stralloc_catb(out,target,targetlocal)) die_nomem();
	break;
      case 'c':
	if (!stralloc_catb(out,confirm+confirmprefix,
			   confirmlocal-confirmprefix)) die_nomem();
	break;
      case 'd':
	if (listdir != 0)
	  if (!stralloc_cats(out,listdir)) die_nomem();
	break;
      case 'r':
	if (!stralloc_catb(out,confirm,confirmlocal)) die_nomem();
	break;
      case 'l':
	if (!quote(&qline,&outlocal)) die_nomem();
	if (!stralloc_cat(out,&qline)) die_nomem();
	break;
      case 'h':
      case 'H':
	if (!stralloc_cat(out,&outhost)) die_nomem();
	break;
      case 't':
	if (q == 'H') strerr_die1x(111,MSG(ERR_SUBST_UNSAFE));
	if (!stralloc_cats(out,verptarget)) die_nomem();
	break;
      case 'n':
	if (!stralloc_cats(out,szmsgnum)) die_nomem();
	break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
	if (params != 0)
	  if (params[line->s[pos+2]-'0'] != 0)
	    if (!stralloc_cats(out,params[line->s[pos+2]-'0']))
	      die_nomem();
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

void copy(struct qmail *qq,
	  const char *fn,	/* text file name */
	  char q)		/* '\0' for regular output, 'B' for base64, */
				/* 'Q' for quoted printable,'H' for header  */
{
  int fd;
  int flagsmatched = 1;
  int match;
  unsigned int pos;

  if ((fd = alt_open_read(fn)) == -1)
    strerr_die2sys((errno == error_noent) ? 100 : 111,
		   FATAL,MSG1(ERR_OPEN,fn));
  substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));
  for (;;) {
    if (getln(&sstext,&srcline,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,MSG1(ERR_READ,fn));
    if (srcline.len > 0) {		/* srcline.len is always > 0 if match is true */
      if (srcline.s[0] == '#') continue;
      /* suppress blank line for 'H'eader mode */
      if (srcline.len == 1 && q == 'H') continue;
      if (srcline.s[0] == '!') {
	if (srcline.s[1] == 'R') {
	  codeput(qq,"   ",3,q);
	  codeputs(qq,confirm,q);
	  codeput(qq,"\n",1,q);
	  continue;
	}
	if (srcline.s[1] == 'A') {
	  codeput(qq,"   ",3,q);
	  codeputs(qq,target,q);
	  codeput(qq,"\n",1,q);
	  continue;
	}
      }
      /* Find <=flags=> tags */
      if (q != 'H'
	  && srcline.len >= 5
	  && srcline.s[0] == '<'
	  && srcline.s[1] == '='
	  && srcline.s[srcline.len-3] == '='
	  && srcline.s[srcline.len-2] == '>') {
	for (flagsmatched = 1, pos = 2;
	     flagsmatched && pos < srcline.len - 3;
	     ++pos)
	  flagsmatched = flagsmatched && flag_isset(srcline.s[pos]);
	continue;
      }
      if (!flagsmatched) continue;

      copy_xlate(&outline,&srcline,0,q);
      codeput(qq,outline.s,outline.len,q);

      /* Last line is missing its trailing newline, add one on output. */
      if (!match)
	codeput(qq,"\n",1,q);
    } else
      break;
  }
  close(fd);
}

