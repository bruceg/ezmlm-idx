#include <sys/types.h>
#include <unistd.h>
#include "alloc.h"
#include "direntry.h"
#include "datetime.h"
#include "now.h"
#include "stralloc.h"
#include "strerr.h"
#include "error.h"
#include "env.h"
#include "sig.h"
#include "open.h"
#include "getln.h"
#include "case.h"
#include "scan.h"
#include "str.h"
#include "fmt.h"
#include "readwrite.h"
#include "fork.h"
#include "wait.h"
#include "exit.h"
#include "substdio.h"
#include "getconf.h"
#include "gen_alloc.h"
#include "gen_allocdefs.h"
#include "constmap.h"
#include "byte.h"
#include "subdb.h"
#include "msgtxt.h"
#include "makehash.h"
#include "mime.h"
#include "wrap.h"
#include "die.h"
#include "idx.h"
#include "yyyymm.h"
#include "cgi.h"
#include "auto_etc.h"

const char FATAL[] = "ezmlm-cgi: fatal: ";
const char USAGE[] =
"ezmlm-cgi: usage: ezmlm-cgi";
#define GET "-getv"
#define THREAD "-threadv"
#define SUBSCRIBE "-subscribe"
#define FAQ "-faq"
#define TXT_CGI_SUBSCRIBE "\">[Subscribe To List]</a>\n"
#define TXT_CGI_FAQ "\">[List FAQ]</a>\n"

int flagshowhtml = 1;	/* show text/html parts. This leads to duplication */
			/* when both text/plain and text/html are in a     */
			/* multipart/alternative message, but it is assumed*/
			/* that text/html is not frivolous, but only used  */
			/* when the formatting is important. */
int flagobscure = 0;	/* Don't remove Sender's E-mail address in message */
			/* view. Overridden by config file (- before list */
			/* name). */

/**************** Header processing ***********************/
static char headers_used[] = "Subject\\From\\Date\\content-type\\"
		"content-transfer-encoding\\mime-version";
/* index of headers displayed (shown in order listed above) */
static int headers_shown[] = {1,1,1,0,0,0};
/* index of specific headers */
#define NO_HDRS 6
#define HDR_SUBJECT 1
#define HDR_FROM 2
#define HDR_CT 4
#define HDR_CTENC 5
#define HDR_VERSION 6

/* Need to add inits if you increase NO_HDRS */
static stralloc hdr[NO_HDRS] = { {0},{0},{0},{0},{0},{0} };
/**************** Header processing ***********************/


/* index of subject in above, first = 1 */

/* TODO: Sort headers before display. Find a way to display the body with the*/
/* correct charset, ideally letting the browser do the work (should really */
/* be able to specify charset for DIV ! */

/* ulong at least 32 bits. (Creating a Year 0xffffff problem ;-) */
#define MAXULONG 0xffffffff

static char cmdstr[5] = "xxx:";
#define ITEM "-msadiz"
#define ITEM_MESSAGE 1
#define ITEM_SUBJECT 2
#define ITEM_AUTHOR 3
#define ITEM_DATE 4
#define ITEM_INDEX 5

#define DIRECT "psnpnz"
#define DIRECT_SAME 0
#define DIRECT_NEXT 1
#define DIRECT_PREV -1
/* use only as the argument for some functions. Terrible hack for date links */
#define DIRECT_FIRST 3
#define DIRECT_LAST 2

static char *dir = 0;
static char *local = 0;
static char *host = 0;
static char *home = 0;
static char *banner = 0;
static const char *charset = 0;
static char *stylesheet = 0;
static char *cmd;
static char strnum[FMT_ULONG];
/* these are the only headers we really care about for message display */
/* one can always retrieve the complete message by E-mail */
static stralloc charg = {0};
static stralloc url = {0};
static stralloc author = {0};
static stralloc subject = {0};
static stralloc base = {0};
static stralloc line = {0};
static stralloc decline = {0};	/* for rfc2047-decoded headers and QP/base64 */
static stralloc cfline = {0};	/* from config file */
static stralloc fn = {0};
static stralloc dtline = {0};
static stralloc headers = {0};
static stralloc encoding = {0};
static stralloc content = {0};
static stralloc curcharset = {0};
static stralloc sainit = {0};
static struct constmap headermap;
static unsigned long uid,euid;
static int recursion_level;
static int so = 0;
static int ss23 = 0;
static int state = 0;
static int match;	/* used everywhere and no overlap */
static int fd;		/* same; never >1 open */
static int cache;	/* 0 = don't; 1 = don't know; 2 = do */
static int flagtoplevel;
static unsigned int flagmime;
static unsigned int cs,csbase,pos;
static int flagrobot;
static int flagpre;
static int precharcount;
static char cn1 = 0;
static char cn2 = 0;
static char lastjp[] = "B";/* to get back to the correct JP after line break */


static struct msginfo {	/* clean info on the target message */
  int item;		/* What we want */
  int direction;	/* Relation to current msg */
  int axis;		/* Axis of desired movement [may be calculated] */
  unsigned long source;	/* reference message number */
  unsigned long target;
  unsigned long date;
  unsigned long *authnav;	/* msgnav structure */
  unsigned long *subjnav;	/* msgnav structure */
  char *author;
  char *subject;
  char *cgiarg;			/* sub/auth as expected from axis */
} msginfo;

static mime_info *mime_current = 0;
static mime_info *mime_tmp = 0;

static struct datetime dt;

static char inbuf[4096];
static substdio ssin;

void die_syntax(const char *s)
{
  strerr_die4x(100,FATAL,MSG("ERR_SYNTAX"),"config file: ",s);
}

static char outbuf[4096];
static substdio ssout = SUBSTDIO_FDBUF(write,1,outbuf,sizeof(outbuf));

void oput(const char *s, unsigned int l)
/* unbuffered. Avoid extra copy as httpd buffers */
{
  if (substdio_put(&ssout,s,l) == -1)
    strerr_die2sys(111,FATAL,MSG("ERR_WRITE_STDOUT"));
}

void oputs(const char *s)
{
  oput(s,str_len(s));
}

/* this error is for things that happen only if program logic is screwed up */
void die_prog(const char *s) { strerr_die5x(100,FATAL,"program error (please send bug report to bugs@ezmlm.org): ",s," Command: ",cmd); }

/* If we already issued a header than this will look ugly */
void cgierr(const char *s,const char *s1,const char *s2)
{
  strerr_warn4(FATAL,s,s1,s2,(struct strerr *)0);
  oputs("Content-type: text/plain\n");
  oputs("Status: 500 Couldn't do it\n\n");
  oputs("I tried my best, but:\n\n");
  if (s) oputs(s);
  if (s1) oputs(s1);
  if (s2) oputs(s2);
  oputs("\n");
  substdio_flush(&ssout);
  _exit(0);
}

static unsigned long msgnav[5]; /* 0 prev prev 1 prev 2 this 3 next 4 next-next */

void toggle_flagpre(int flag)
{
  flagpre = flag;
  precharcount = 0;
  cn1 = 0; cn2 = 0;		/* just in case */
}

unsigned int decode_charset(const char *s,unsigned int l)
/* return charset code. CS_BAD means that base charset should be used, i.e. */
/* that charset is empty or likely invalid. CS_NONE are charsets for which  */
/* we don't need to do anything special. */
{
  if (case_startb(s,l,"iso-8859") || case_startb(s,l,"us-ascii") ||
	case_startb(s,l,"utf"))	/* at the moment, we can do utf-8 right */
    return CS_NONE;		/* what is utf-7 (used by OE)? */
  if (case_startb(s,l,"x-cp") ||
	case_startb(s,l,"cp") ||
	case_startb(s,l,"x-mac") ||
	case_startb(s,l,"koi8")) return CS_NONE;
  if (!l || *s == 'x' || *s == 'X') return CS_BAD;
  if (case_startb(s,l,"iso-2022")) {
    if (case_startb(s+8,l-8,"-cn"))
      return CS_2022_CN;
    if (case_startb(s+8,l-8,"-jp"))
      return CS_2022_JP;
    return CS_2022_KR;
  }
  if (case_startb(s,l,"cn-") ||
		case_startb(s,l,"hz-gb") ||
		case_startb(s,l,"gb") ||
		case_startb(s,l,"big5"))
    return CS_CN;		/* Only consideration for linebreak */
  if (case_startb(s,l,"iso_8859") ||
	case_startb(s,l,"latin") ||
	case_startb(s,l,"windows")) return CS_NONE;
/* Add other charsets here. Later we will add code to replace a detected */
/* charset name with another, and to connect conversion routines, such as */
/* between windows-1251/koi-8r/iso-8859-5 */
  return CS_BAD;
}

void html_put (const char *s,unsigned int l)
/* At this time, us-ascii, iso-8859-? create no problems. We just encode  */
/* some html chars. iso-2022 may have these chars as character components.*/
/* cs is set for these, 3 for CN, 2 for others. Bit 0 set means 2 byte    */
/* chars for SS2/SS3 shiftouts (JP doesn't use them, KR has single byte.  */
/* If cs is set and we're shifted out (so set) we don't substitute. We    */
/* also look for SI/SO to adjust so, and ESC to detect SS2/SS3. Need to   */
/* ignore other ESC seqs correctly. JP doesn't use SI/SO, but uses        */
/* ESC ( B/J and ESC $ B/@ analogously, so we use these to toggle so.     */
/* "Roman", i.e. ESC ( J is treated as ascii - no differences in html-    */
/* relevant chars. Together, this allows us to deal with all iso-2022-*   */
/* as a package. see rfc1468, 1554, 1557, 1922 for more info.             */
/* line break at 84 to avoid splits with lines just a little too long. */
{
  if (!cs) {		/* us-ascii & iso-8859- & unrecognized */
    for (;l--;s++) {
      precharcount++;
      switch (*s) {
        case '>': oputs("&gt;"); break;
        case '<': oputs("&lt;"); break;
        case '"': oputs("&quot;"); break;
        case '&': oputs("&amp;"); break;
	case '\n': precharcount = 0; oput(s,1); break;
	case ' ':
	  if (precharcount >= 84 && flagpre) {
	    oput("\n",1);			/* in place of ' ' */
	    precharcount = 0;
	  } else
	    oput(s,1);				/* otherwise out with it. */
	  break;
        default: oput(s,1); break;
      }
    }
  } else if (cs == CS_CN) {			/* cn-, gb*, big5 */
    for (;l--;s++) {
      precharcount++;
      if (cn1) { cn2 = cn1; cn1 = 0; }		/* this is byte 2 */
      else { cn2 = 0; cn1 = *s & 0x80; }	/* this is byte 1/2 or ascii */
      if (!cn1 && !cn2) {			/* ascii */
	switch (*s) {
          case '>': oputs("&gt;"); break;
          case '<': oputs("&lt;"); break;
          case '"': oputs("&quot;"); break;
          case '&': oputs("&amp;"); break;
	  case '\n': precharcount = 0; oput(s,1); break;
          case ' ':
		if (precharcount >= 84 && flagpre) {
		  oput("\n",1);		/* break in ascii sequence */
		  precharcount = 0;
		} else
		  oput(s,1);
		break;
	  default: oput(s,1); break;
	}
      } else if (precharcount >= 84 && flagpre && cn2) {
	  oput("\n",1);			/* break after 2-byte code */
	  precharcount = 0;
      }
    }
  } else {					/* iso-2022 => PAIN! */
    for (;l--;s++) {
      precharcount++;
      if (ss23) {				/* ss2/ss3 character */
	ss23--;
	oput(s,1);
        continue;
      }
      if (so) {					/* = 0 ascii, = 1 SO charset */
        if (!(*s & 0xe0)) {			/* ctrl-char */
	  switch (*s) {
	    case ESC: state = 1; break;
	    case SI: so = 0; break;
	    case '\n': precharcount = 0; break;
	    default: break;
	  }
	}
	oput(s,1);
      } else {					/* check only ascii */
	switch (*s) {
	  case '>': oputs("&gt;"); break;
	  case '<': oputs("&lt;"); break;
	  case '"': oputs("&quot;"); break;
	  case '&': oputs("&amp;"); break;
          case ' ':
		if (precharcount >= 84 && flagpre) {
		  oput("\n",1);		/* break in ascii sequence */
		  precharcount = 0;
		} else
		  oput(s,1);
		break;
	  default:
		  oput(s,1);
		  if (!(*s & 0xe0)) {
		    switch (*s) {
		      case SO: so = 1; break;
		      case ESC: state = 1; break;
		      case SI: so = 0; break;	/* shouldn't happen */
		      case '\n': precharcount = 0; break;
		      default: break;
		    }
		  }
	}
      }		/* by now all output is done, now ESC interpretation */
      if (state) {
		/* ESC code - don't count */
	  if (precharcount) precharcount--;
	  state++;
	  switch (state) {
	    case 2: break;			/* this was the ESC */
	    case 3: switch (*s) {
			case 'N': ss23 = (cs & 1) + 1; state = 0; break;
			case 'O': ss23 = 2; state = 0; break;
			case '(': state = 20; so = 0; break;	/* JP ascii */
			case '$': break;		/* var S2/SS2/SS3 des*/
			case '.': state = 10;	/* g3 settings, one more char */
			default: state = 0; break;	/* or JP */
		}
		break;
	    case 4: switch (*s) {	/* s2/ss2/ss3 or JP 2 byte shift */
		   case 'B':
		   case '@': lastjp[0] = *s;
			     so = 1; state = 0; break;	/* JP */
		   default: break;			/* other SS2/3 des */
		 }
		 break;
	    case 5:  state = 0; break;		/* 4th char of ESC $ *|+|) X */
	    case 11: state = 0; break;		/* 3nd char of ESC . */
	    case 21: state = 0; break;		/* ESC ( X for JP */
	    default: die_prog("bad state in html_put"); break;
	  }
      } else if (so && flagpre && precharcount >= 84) {
		/* 84 is nicer than 78/80 since most use GUI browser */
		/* iso-2022-* line splitter here. SO only, SI done above */
		/* For JP need even precharcount, add ESC ( B \n ESC $B */
	if (so && !(precharcount & 1)) {	/* even */
	  precharcount = 0;			/* reset */
	  if (cs == CS_2022_JP) {		/* JP uses ESC like SI/SO */
	    oputs(TOASCII);
	    oput("\n",1);
	    oputs(TOJP);
	    oput(lastjp,1);
	  } else {
	    if (so) {
		/* For iso-2022-CN: nothing if SI, otherwise SI \n SO */
		/* For iso-2022-KR same */
	      oputs(SI_LF_SO);
	    } else
	      oput("\n",1);
	  }
	}
      }
    }
  }
}

static const char hexchar[] = "0123456789ABCDEF";
static char enc_url[] = "%00";

void urlencode_put (const char *s,unsigned int l)
{
  for (;l--;s++) {
    unsigned char ch;
    ch = (unsigned char) *s;
    if (ch <= 32 || ch > 127 || byte_chr("?<>=/:%+#\"",10,ch) != 10) {
      enc_url[2] = hexchar[ch & 0xf];
      enc_url[1] = hexchar[(ch >> 4) & 0xf];
      oput(enc_url,3);
    } else
      oput(s,1);
  }
}

void urlencode_puts(const char *s)
{
  urlencode_put(s,str_len(s));
}

void anchor_put(char *s, unsigned int l)
/* http://, ftp:// only */
{
  char *cpl,*cpafter,*cpstart,*cpend;
  unsigned int pos,i;

  pos = byte_chr(s,l,':');
  if (pos + 3 >= l || !pos) {			/* no ':' no URL (most lines) */
    html_put(s,l);
    return;
  }

  cpl = s;
  cpafter = s + l;
  for (;;) {
    cpstart = (char *) 0;
    if (s[pos + 1] == '/' && s[pos + 2] == '/') {
      cpend = s + pos + 2;
      for (i = pos - 1; i + 6 >= pos; i--) {		/* pos always >=1 */
        if ((s[i] < 'a' || s[i] > 'z') && (s[i] < 'A' || s[i] > 'Z')) {
          cpstart = s + i + 1;	/* "[:alpha:]{1,5}://" accepted */
	  break;
        }
	if (!i && i + 6 < pos) {
	  cpstart = s;
	  break;
	}
      }
    }
    if (cpstart) {					/* found URL */
      while (cpend < cpafter && str_chr(" \t\n",*cpend) == 3) cpend++;
      cpend--;						/* locate end */
      while (cpend > cpstart && str_chr(".,;])>\"\'",*cpend) != 8) cpend--;
      html_put(cpl,cpstart - cpl);			/* std txt */
      oputs("<a href=\"");				/* link start */
      oput(cpstart,cpend - cpstart + 1);		/* link */
      oputs("\">");
      html_put(cpstart,cpend - cpstart + 1);		/* visible */
      oputs("</a>");					/* end */
      cpl = cpend + 1;
      pos = cpend - s;
      if (pos >= l) return;
    } else
      pos++;
    pos += byte_chr(s + pos,l - pos,':');
    if (pos + 3 >= l) {
      html_put(cpl,cpafter - cpl);	/* std txt */
      return;
    }
  }
}

int checkhash(const char *s)
{
  int l = HASHLEN;
  while (l--) {
    if (*s < 'a' || *s > 'p') return 0;	/* illegal */
    s++;
  }
  if (*s) return 0;			/* extraneous junk */
  return 1;
}

int makefn(stralloc *sa,int item,unsigned long n,const char *hash)
{
  if (!stralloc_copys(sa,"archive/")) die_nomem();
  if (item == ITEM_MESSAGE) {
    if (!stralloc_catb(sa,strnum,fmt_ulong(strnum, n / 100))) die_nomem();
    if (!stralloc_cats(sa,"/")) die_nomem();
    if (!stralloc_catb(sa,strnum,fmt_uint0(strnum,(unsigned int) (n % 100),2)))
			die_nomem();
  } else if (item == ITEM_DATE) {
    if (!stralloc_cats(sa,"threads/")) die_nomem();
    if (!stralloc_catb(sa,strnum,fmt_ulong(strnum,n)))
	die_nomem();
  } else if (item == ITEM_INDEX) {
    if (!stralloc_catb(sa,strnum,fmt_ulong(strnum, n / 100))) die_nomem();
    if (!stralloc_cats(sa,"/index")) die_nomem();
  } else {
    if (item == ITEM_AUTHOR) {
      if (!stralloc_cats(sa,"authors/")) die_nomem();
    } else {
      if (!stralloc_cats(sa,"subjects/")) die_nomem();
    }
    if (!hash) return 0;
    if (!stralloc_catb(sa,hash,2)) die_nomem();
    if (!stralloc_cats(sa,"/")) die_nomem();
    if (!stralloc_catb(sa,hash+2,HASHLEN-2)) die_nomem();
  }
  if (!stralloc_0(sa)) die_nomem();
  return 1;
}

void alink(struct msginfo *infop,int item,int axis,
	   unsigned long msg,const char *data,unsigned int l)
/* links with targets other msg -> msg. If the link is for author, we    */
/* still supply subject, since most navigation at the message level will */
/* be along threads rather than author and we don't have an author index.*/
{
  const char *cp;

  cp = (const char *) 0;
	/* this should be separate routine. Works because all index views */
	/* have at least a subject link */
  if (axis == ITEM_SUBJECT && infop->target == msg)
    oputs("<a name=\"b\"></a>");
  oput(url.s,url.len);
  cmdstr[0] = ITEM[item];
  cmdstr[1] = ITEM[axis];
  cmdstr[2] = DIRECT[DIRECT_SAME + 1];
  if (item == ITEM_MESSAGE && axis == ITEM_AUTHOR) {
    if (infop->subject) {
      cmdstr[1] = ITEM[ITEM_SUBJECT];
      cp = infop->subject;	/* always HASLEN in length due to decode_cmd */
    }
  }
  oputs(cmdstr);		/* e.g. map: */
  oput(strnum,fmt_ulong(strnum,msg));
  if (!cp && l >= HASHLEN)
    cp = data;
  if (infop->date) {
    oput(":",1);
    oput(strnum,fmt_ulong(strnum,infop->date));
  }
  if (cp) {
    oput(":",1);
    oput(cp,HASHLEN);
  }
  switch (item) {
    case ITEM_MESSAGE: oputs("\" class=\"mlk\">"); break;
    case ITEM_AUTHOR: oputs("#b\" class=\"alk\">"); break;
    case ITEM_SUBJECT: oputs("#b\" class=\"slk\">"); break;
    default: oputs("#b\">"); break;
  }
  if (HASHLEN + 1 < l)
    html_put(data + HASHLEN + 1,l - HASHLEN - 1);
  else
    oputs("(none)");
  oputs("</a>");
}

void linktoindex(struct msginfo *infop,int item)
/* for links from message view back to author/subject/threads index */
{
  oput(url.s,url.len);
  cmdstr[0] = ITEM[item];
  cmdstr[1] = ITEM[item];
  cmdstr[2] = DIRECT[DIRECT_SAME + 1];
  oputs(cmdstr);		/* e.g. map: */
  oput(strnum,fmt_ulong(strnum,infop->target));
  if (infop->date) {
    oput(":",1);
    oput(strnum,fmt_ulong(strnum,infop->date));
  }
  switch (item) {
    case ITEM_AUTHOR:
      if (infop->author) {
	oput(":",1);
	oputs(infop->author);
      }
      break;
    case ITEM_SUBJECT:
      if (infop->subject) {
	oput(":",1);
	oputs(infop->subject);
      }
      break;
    default:
      break;
  }
  oputs("#b\"");
}

void link_msg(struct msginfo *infop,int axis,int direction)
/* Creates <a href="mapa:123:aaaaa...."> using a maximum of available */
/* info only for links where the target is a message */
{
  unsigned long msg;
  char *acc;
  oput(url.s,url.len);
  cmdstr[0] = ITEM[ITEM_MESSAGE];
  cmdstr[1] = ITEM[axis];
  cmdstr[2] = DIRECT[direction + 1];
  msg = infop->target;
  acc = 0;
      switch(axis) {
	case ITEM_SUBJECT:
	  if (infop->subject)
	    acc = infop->subject;
	  if (infop->subjnav)	/* translate to message navigation */
	    if (infop->subjnav[direction]) {
	      msg = infop->subjnav[direction];
	      cmdstr[2] = DIRECT[DIRECT_SAME + 1];
	  }
	  acc = infop->subject;
	  break;
	case ITEM_AUTHOR:
	  if (infop->author)
	    acc = infop->author;
	  if (infop->authnav)	/* translate to message navigation */
	    if (infop->authnav[direction]) {
	      msg = infop->authnav[direction];
	      cmdstr[2] = DIRECT[DIRECT_SAME + 1];
	    }
	  acc = infop->author;
	  break;
	default:
	  break;
	}
	oputs(cmdstr);
	oput(strnum,fmt_ulong(strnum,msg));
	if (acc) {
	  oputs(":");
	  oputs(acc);
	}
	oputs("\">");
}

void justpress()
{
  oputs("?subject=");
  urlencode_puts("Just Click \"SEND\"!");
}

void homelink()
{
  const char *cp,*cp1,*cp2;

  if (home && *home) {
    cp = home;
    for(;;) {
      cp1 = cp;
      while(*cp1 && *cp1 != '=') cp1++;
      if (!*cp1) break;
      cp2 = cp1;
      while(*cp2 && *cp2 != ',') cp2++;
      oputs("<a href=\"");
      oput(cp1 + 1,cp2 - cp1 - 1);
      oputs("\">");
      oput(cp,cp1 - cp);
      oputs("</a>\n");
      if (!*cp2) break;
      cp = cp2 + 1;
    }
  }
}

void subfaqlinks()
{
  oputs("<a href=\"mailto:");
  oputs(local);
  oputs(SUBSCRIBE);
  oputs("@");
  oputs(host);
  justpress();
  oputs(TXT_CGI_SUBSCRIBE);
  oputs("<a href=\"mailto:");
  oputs(local);
  oputs(FAQ);
  oputs("@");
  oputs(host);
  justpress();
  oputs(TXT_CGI_FAQ);
}

void msglinks(struct msginfo *infop)
/* Creates the html for all links from one message view */
{
  oputs("<div class=\"msglinks\"><strong>Msg by: ");
  link_msg(infop,ITEM_SUBJECT,DIRECT_PREV);
  oputs("[&lt;-</a> ");
  linktoindex(infop,ITEM_SUBJECT);
  oputs(">thread</a> ");
  link_msg(infop,ITEM_SUBJECT,DIRECT_NEXT);
  oputs("-&gt;]</a> \n");
  link_msg(infop,ITEM_MESSAGE,DIRECT_PREV);
  oputs("[&lt;-</a> ");
  linktoindex(infop,ITEM_INDEX);
  oputs(">time</a> ");
  link_msg(infop,ITEM_MESSAGE,DIRECT_NEXT);
  oputs("-&gt;]</a> \n");
  link_msg(infop,ITEM_AUTHOR,DIRECT_PREV);
  oputs("[&lt;-</a> ");
  linktoindex(infop,ITEM_AUTHOR);
  oputs(">author</a> ");
  link_msg(infop,ITEM_AUTHOR,DIRECT_NEXT);
  oputs("-&gt;]</a> |\n");
  linktoindex(infop,ITEM_DATE);
  oputs(">[Threads]</a>\n");
  homelink();
  oputs("\n<a href=\"mailto:");
  oputs(local);
  oputs(GET);
  strnum[fmt_ulong(strnum,infop->target)] = '\0';
  oputs(strnum);
  oputs("@");
  oputs(host);
  justpress();
  oputs("\">[Email Msg]</a>\n");
  oputs("<a href=\"mailto:");
  oputs(local);
  oputs(THREAD);
  oputs(strnum);
  oputs("@");
  oputs(host);
  justpress();
  oputs("\">[Email Thread]</a>\n");
  subfaqlinks();
  oputs("</strong></div>\n");
}

#define SPC_BASE 1
#define SPC_BANNER 2

void html_header(const char *t,const char *s,unsigned int l,const char *class,int flagspecial)
/* flagspecial: 0x1 => robot index; no style sheet, no BASE */
/* flagspecial: 0x2 => banner, if available */
{
  oputs("Content-Type: text/html; charset=");
  oput(curcharset.s,curcharset.len);

  oputs("\nCache-Control: ");
  switch (cache) {
    case 0:
	oputs("no-cache");		/* known upper border */
	break;
    case 1:
	oputs("max-age=300");		/* 5 min - most lists aren't that fast*/
	break;
    case 2:
	oputs("max-age=1209600");	/* 14 days is a long time */
	break;
  }
  oputs("\n\n");
  oputs("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n");
  oputs("<html xmlns=\"http://www.w3.org/1999/xhtml\"><head>\n<title>");
  if (local) {
    oputs(local);
    oputs("@");
    oputs(host);
    oputs(": ");
  }
  if (t) oputs(t);
  if (s) html_put(s,l);
  oputs("</title>\n");
  if (class && *class && stylesheet && *stylesheet) {
    oputs("<link href=\"");
    oputs(stylesheet);
    oputs("\" rel=\"stylesheet\" type=\"text/css\" />\n");
  }
  if (!flagrobot)	/* robot access allowed to follow */
    oputs("<meta name=\"robots\" content=\"noindex\" />\n");
  if (flagrobot < 2)
    oputs("<meta name=\"robots\" content=\"nofollow\" />\n");
  if (flagspecial & SPC_BASE)
    oput(base.s,base.len);
  oputs("</head>\n");
  if (class && *class) {
    oputs("<body class=\"");
    oputs(class);
    oputs("\">\n");
  } else
    oputs("<body>\n");

}

void html_footer(int flagspecial)
{
  if ((flagspecial & SPC_BANNER) && banner && *banner) {
    oputs("<div class=\"banner\">\n");
    if (*banner == '<') oputs(banner);
    else
      strerr_die2x(100,FATAL,"Sorry - banner programs not supported");
    oputs("</div>\n");
  }
  oputs("</body>\n</html>\n");
  substdio_flush(&ssout);
}

/* DATE functions */

void datelink(struct msginfo *infop,unsigned long d,int direction)
/* output a date with link back to thread index */
{
  oput(url.s,url.len);
  cmdstr[0] = ITEM[ITEM_DATE];
  cmdstr[1] = ITEM[ITEM_DATE];
  cmdstr[2] = DIRECT[direction + 1];
  oputs(cmdstr);
  if (direction == DIRECT_LAST)
    oput("0",1);	/* suppress msgnum to avoid going there */
  else
    oput(strnum,fmt_ulong(strnum,infop->target));
  oputs(":");
  oput(strnum,fmt_ulong(strnum,d));
  oputs("#b\">");
  switch (direction) {
    case DIRECT_SAME:
	if (dateline(&dtline,d) < 0) die_nomem();
	oput(dtline.s,dtline.len);
	break;
    case DIRECT_PREV:
	oputs("[&lt;-]");
	break;
    case DIRECT_NEXT:
	oputs("[-&gt;]");
	break;
    case DIRECT_FIRST:
	oputs("[&lt;&lt;-]");
	break;
    case DIRECT_LAST:
	oputs("[-&gt;&gt;]");
	break;
  }
  oputs("</a>");
}

void finddate(struct msginfo *infop)
/* DIRECT_SAME works as DIRECT_PREV, dvs returns previous date or last date */
{
  DIR *archivedir;
  direntry *d;
  unsigned long ddate, startdate;
  unsigned long below, above;

  below = 0L;
  above = MAXULONG;	/* creating a Y 0xffffff problem */
  startdate = infop->date;
  archivedir = opendir("archive/threads/");
  if (!archivedir) {
    strerr_die2sys((errno == error_noent) ? 100 : 111,
		   FATAL,MSG1("ERR_OPEN","/archive/threads"));
  }
  while ((d = readdir(archivedir))) {		/* dxxx/ */
    if (str_equal(d->d_name,".")) continue;
    if (str_equal(d->d_name,"..")) continue;
    scan_ulong(d->d_name,&ddate);
    if (!ddate) continue;	/* just in case some smart guy ... */
    if (startdate) {
      if (ddate > startdate && ddate < above) above = ddate;
      if (ddate < startdate && ddate > below) below = ddate;
    } else {
      if (ddate < above) above = ddate;
      if (ddate > below) below = ddate;
    }
  }
  closedir(archivedir);

  if (infop->direction == DIRECT_NEXT && (above != MAXULONG || !below))
	/* we always give a valid date as long as there is at least one */
    infop->date = above;
  else
    infop->date = below;
  return;
}

void latestdate(struct msginfo *infop,int flagfail)
{
  if (!flagfail) {
    datetime_tai(&dt,now());
    infop->date = ((unsigned long) dt.year + 1900) * 100 + dt.mon + 1;
  } else {
    infop->date = 0;
    infop->direction = DIRECT_PREV;
    finddate(infop);
  }
}

void firstdate(struct msginfo *infop)
{
    infop->date = 0;
    infop->direction = DIRECT_NEXT;
    finddate(infop);
}

void gtdate(struct msginfo *infop,int flagfail)
/* infop->date has to be 0 or valid on entry. Month outside of [1-12] on */
/* entry causes GIGO */
{
  if (!flagfail) {				/* guess */
    if (infop->direction == DIRECT_NEXT) {
      infop->date++;
      if (infop->date % 100 > 12) infop->date += (100 - 12);
    } else if (infop->direction == DIRECT_PREV) {
      infop->date--;
      if (!infop->date % 100) infop->date -= (100 - 12);
    }
  } else
    finddate(infop);
  return;
}

void indexlinks(struct msginfo *infop)
{
  unsigned long tmpmsg;

  tmpmsg = infop->target;
  infop->target = 1;
  oputs("<div class=\"idxlinks\"><strong>");
  linktoindex(infop,ITEM_INDEX);
  oputs(">[&lt;&lt;-]</a>\n");
  if (tmpmsg >= 100) infop->target = tmpmsg - 100;
  linktoindex(infop,ITEM_INDEX);
  oputs(">[&lt;-]</a>\n");
  infop->target = tmpmsg + 100;
  linktoindex(infop,ITEM_INDEX);
  oputs(">[-&gt;]</a>\n");
  infop->target = MAXULONG;
  linktoindex(infop,ITEM_INDEX);
  oputs(">[-&gt;&gt;]</a> |\n");
  infop->target = tmpmsg;
  linktoindex(infop,ITEM_DATE);
  oputs(">[Threads by date]</a>\n");
  subfaqlinks();
  homelink();
  oputs("</strong></div>\n");
}

int show_index(struct msginfo *infop)
{
  unsigned long thismsg;
  unsigned int pos,l;
  char ch;

  (void) makefn(&fn,ITEM_INDEX,msginfo.target,"");
  if ((fd = open_read(fn.s)) == -1) {
    if (errno == error_noent)
      return 0;
    else
      strerr_die2sys(111,FATAL,MSG1("ERR_OPEN",fn.s));
  }
  substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));
  if (!stralloc_copyb(&line,strnum,
	fmt_ulong(strnum,(unsigned long) (infop->target / 100))))
		die_nomem();
  if (!stralloc_cats(&line,"xx")) die_nomem();
  html_header("Messages ",line.s,line.len,"idxbody",SPC_BANNER | SPC_BASE);
  indexlinks(infop);
  oputs("<div><hr /></div><h1 id=\"idxhdr\">");
  oputs("Messages ");
  oput(line.s,line.len);
  oputs("</h1>\n");
  oputs("<div class=\"idx\"><hr />\n");
  for (;;) {
    if (getln(&ssin,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,MSG1("ERR_READ",fn.s));
    if (!match)
      break;
    pos = scan_ulong(line.s,&thismsg);
    l = pos;
    ch = line.s[pos++];
    pos++;
    if (line.len < pos + 1 + HASHLEN)
       strerr_die2x(100,FATAL,"index line with truncated subject entry");
    if (!stralloc_copyb(&subject,line.s+pos,HASHLEN)) die_nomem();
    if (!stralloc_0(&subject)) die_nomem();
    infop->axis = ITEM_SUBJECT;
    infop->subject = subject.s;
    oput(strnum,fmt_uint0(strnum,(unsigned int) thismsg % 100,2));
    oputs(": ");
    alink(infop,ITEM_MESSAGE,ITEM_SUBJECT,thismsg,line.s+pos,line.len - pos - 1);
    oputs("\n");
    if (ch == ':') {
      if (getln(&ssin,&line,&match,'\n') == -1)
        strerr_die2sys(111,FATAL,MSG1("ERR_READ",fn.s));
      if (!match)
        break;
      pos = byte_chr(line.s,line.len,';');
      if (pos != line.len) {
	infop->date = date2yyyymm(line.s);
	oputs("(");
	alink(infop,ITEM_AUTHOR,ITEM_AUTHOR,thismsg,line.s+pos+1,
		line.len - pos - 2);
	oputs(")<br />\n");
      }
    }
  }
  close(fd);
  oputs("\n<hr /></div>\n");
  indexlinks(infop);
  html_footer(SPC_BANNER);
  return 1;
}

void objectlinks(struct msginfo *infop,int item)
{
  oputs("<div class=\"objlinks\"><strong>\n");
  if (item == ITEM_DATE) {
    datelink(infop,0,DIRECT_FIRST);
    datelink(infop,infop->date,DIRECT_PREV);
    datelink(infop,infop->date,DIRECT_NEXT);
    datelink(infop,0,DIRECT_LAST);
    oputs("\n");
  } else {
    if (!infop->target) infop->axis = ITEM_DATE;
    linktoindex(infop,ITEM_DATE);
    oputs(">[Threads by date]</a>\n");
  }
  if (item != ITEM_INDEX) {
    linktoindex(infop,ITEM_INDEX);
    oputs(">[Messages by date]</a>\n");
  }
  homelink();
  subfaqlinks();
  oputs("</strong></div>\n");
}

int show_object(struct msginfo *infop,int item)
/* shows thread, threads, author */
/* infop has the info needed to access the author/subject/thread file */
{
  unsigned long lastdate,thisdate,thismsg;
  char linkitem;
  char targetitem;
  unsigned int pos;

  lastdate = 0L;
  targetitem = ITEM_MESSAGE;			/* default message is target */
  switch (item) {
    case ITEM_SUBJECT:
	if (!makefn(&fn,ITEM_SUBJECT,0L,infop->subject)) return 0;
	break;
    case ITEM_AUTHOR:
	if (!makefn(&fn,ITEM_AUTHOR,0L,infop->author)) return 0;
	break;
    case ITEM_DATE:
	if (!makefn(&fn,ITEM_DATE,infop->date,"")) return 0;
	break;
    default:
	die_prog("Bad object type in show_object");
  }
  if ((fd = open_read(fn.s)) == -1) {
    if (errno == error_noent)
      return 0;
    else
      strerr_die2sys(111,FATAL,MSG1("ERR_OPEN",fn.s));
  }
  substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));
  if (item != ITEM_DATE) {
    if (getln(&ssin,&line,&match,'\n') == -1)	/* read subject */
      strerr_die2sys(111,FATAL,MSG1("ERR_READ",fn.s));
    if (!match || line.len < HASHLEN + 2)
      strerr_die2x(111,FATAL,MSG1("ERR_READ_NOTHING",fn.s));
  }
  switch (item) {
    case ITEM_SUBJECT:
	html_header("Thread on: ",line.s + HASHLEN + 1,
		line.len - HASHLEN - 2,"subjbody",SPC_BANNER | SPC_BASE);
	objectlinks(infop,item);
	oputs("<div><hr /></div><h1>On: ");
	oput(line.s+HASHLEN+1,line.len-HASHLEN-2);
	oputs("</h1>\n");
	break;
    case ITEM_AUTHOR:
	html_header("Posts by: ",line.s + HASHLEN + 1,
		line.len - HASHLEN - 2,"authbody",SPC_BANNER | SPC_BASE);
	objectlinks(infop,item);
	oputs("<div><hr /></div><h1>By: ");
	oput(line.s+HASHLEN+1,line.len-HASHLEN-2);
	oputs("</h1>\n");
	break;
    case ITEM_DATE:
/*	targetitem = ITEM_SUBJECT;*/	/* thread index is target */
	thisdate = infop->date;
	if (dateline(&dtline,infop->date) < 0) die_nomem();
	html_header("Threads for ",
		dtline.s,dtline.len,"threadsbody",SPC_BANNER | SPC_BASE);
	objectlinks(infop,item);
	oputs("<div><hr /></div><h1>Threads for ");
	oput(dtline.s,dtline.len);
	oputs("</h1>\n");
	break;
    default: die_prog("unrecognized object type in show_object");
  }

  oputs("<div class=\"obj\">\n");
  for (;;) {
    if (getln(&ssin,&line,&match,'\n') == -1)	/* read subject */
      strerr_die2sys(111,FATAL,MSG1("ERR_READ",fn.s));
    if (!match)
      break;
    pos = scan_ulong(line.s,&thismsg);
    if (line.s[pos++] != ':')
	strerr_die4x(100,FATAL,"entry in ",fn.s," lacks message number");
    if (item != ITEM_DATE) {		/* no date for threads by date */
      pos += scan_ulong(line.s+pos,&thisdate);
      infop->date = thisdate;
      if (line.s[pos++] != ':')
	strerr_die4x(100,FATAL,"entry in ",fn.s," lacks date");
    }
    if (line.len < pos + HASHLEN + 2)
	strerr_die4x(100,FATAL,"entry in ",fn.s," lacks hash");
    if (thisdate != lastdate) {
      oputs("<h2>");
      datelink(infop,thisdate,DIRECT_SAME);
      lastdate = thisdate;
      oputs("</h2>\n");
	/* moved <h2> out of <ul> */
      /* if (!lastdate)
	  oputs("<ul>\n");
      else
        oputs("<p>");
      oputs("<li>\n"); */

	oputs("<ul>\n");

    }
    /* li at beginning of each link */
    oputs("<li>");

    if (item == ITEM_SUBJECT)
      linkitem = ITEM_AUTHOR;
    else
      linkitem = ITEM_SUBJECT;
    alink(infop,targetitem,linkitem,thismsg,line.s+pos,line.len - pos - 1);
    oputs("</li>\n");
  }
  close(fd);
  oputs("</ul>\n");
  if (!infop->target)
    oputs("<a name=\"b\"></a>");
  oputs("<hr /></div>\n");
  objectlinks(infop,item);
  html_footer(SPC_BANNER);
  return 1;
}

void clear_mime()
{
  mime_current->charset.len = 0;	/* exist but need emptying */
  mime_current->boundary.len = 0;
  mime_current->ctype.len = 0;
  mime_current->mimetype = MIME_NONE;
  mime_current->ctenc = CTENC_NONE;
  mime_current->cs = CS_NONE;
}

void new_mime()
{
    mime_tmp = mime_current;
    if (mime_current)
      mime_current = mime_current->next;
    if (!mime_current) {
      if (!(mime_current = (mime_info *) alloc(sizeof (mime_info))))
	die_nomem();
      mime_current->charset = sainit;		/* init */
      mime_current->boundary = sainit;
      mime_current->ctype = sainit;
      mime_current->next = (mime_info *) 0;
      mime_current->previous = mime_tmp;
    }
    clear_mime();
    if (mime_tmp)
      mime_current->level = mime_tmp->level + 1;
    else
      mime_current->level = 1;
}

void mime_getarg(stralloc *sa,char **s, unsigned int *l)
/* copies next token or "token" into sa and sets s & l appropriately */
/* for continuing the search */
{
  char *cp, *cpafter, *cpnext;

      if (!*l || !**s) return;
      if (**s == '"') {
	(*s)++; (*l)--;
	cp = *s; cpnext = cp + *l; cpafter = cpnext;
	while (cp < cpafter) {
	  if (*cp == '"') {
	    break;
	  }
	  cp++;
	}
	cpnext = cp;
      } else {
	cp = *s; cpnext = cp + *l; cpafter = cpnext;
	while (cp < cpafter) {
	  if (*cp == ' ' || *cp == '\t' || *cp == '\n' || *cp == ';') {
	    break;
	  }
	  cp++;
	}
	cpnext = cp;
      }
      if (!stralloc_copyb(sa,*s,cp - *s)) die_nomem();
      *l = cpafter - cpnext;		/* always >= 0 */
      *s = cpnext;
      return;
}

void decode_mime_type(char *s,unsigned int l,unsigned int flagmime)
{
  char *st;
  unsigned int r,lt;
  if (!flagmime || !l) {		/* treat non-MIME as plain text */
    mime_current->mimetype = MIME_TEXT_PLAIN;
    if (!stralloc_copys(&curcharset,charset)) die_nomem();
	/* should be us-ascii, but this is very likely better */
    return;
  }
  r = MIME_APPLICATION_OCTETSTREAM;
  while (l && (*s == ' ' || *s == '\t')) { s++; l--; }	/* skip LWSP */
  mime_getarg(&(mime_current->ctype),&s,&l);
  st = mime_current->ctype.s;
  lt = mime_current->ctype.len;
  if (case_startb(st,lt,"text")) {			/* text types */
    r = MIME_TEXT; st+= 4; lt-= 4;
    if (case_startb(st,lt,"/plain")) {
      r = MIME_TEXT_PLAIN; st+= 6; lt-= 6;
    } else if (case_startb(st,lt,"/html")) {
      r = MIME_TEXT_HTML; st+= 5; lt-= 5;
    } else if (case_startb(st,lt,"/enriched")) {
      r = MIME_TEXT_ENRICHED; st+= 9; lt-= 9;
    } else if (case_startb(st,lt,"/x-vcard")) {
      r = MIME_TEXT_ENRICHED; st+= 8; lt-= 8;
    }
  } else if (case_startb(st,lt,"multipart")) {		/* multipart types */
    r = MIME_MULTI; st += 9; lt-= 9;
    if (case_startb(st,lt,"/alternative")) {
      r = MIME_MULTI_ALTERNATIVE; st+= 12; lt-= 12;
    } else if (case_startb(st,lt,"/mixed")) {
      r = MIME_MULTI_MIXED; st+= 6; lt-= 6;
    } else if (case_startb(st,lt,"/digest")) {
      r = MIME_MULTI_DIGEST; st+= 7; lt-= 7;
    } else if (case_startb(st,lt,"/signed")) {
      r = MIME_MULTI_SIGNED; st+= 7; lt-= 7;
    }
  } else if (case_startb(st,lt,"message")) {		/* message types */
    r = MIME_MESSAGE; st += 7; lt -= 7;
    if (case_startb(st,lt,"/rfc822")) {
      r = MIME_MESSAGE_RFC822; st+= 7; lt-= 7;
    }
  }
  mime_current->mimetype = r;
  while (l) {
    while (l && (*s == ' ' || *s == '\t' || *s == ';' || *s == '\n')) {
	 s++; l--; }					/* skip ;LWSP */
    if (case_startb(s,l,"boundary=")) {
      s += 9; l-= 9;
      mime_getarg(&(mime_current->boundary),&s,&l);
    } else if (case_startb(s,l,"charset=")) {
      s += 8; l-= 8;
      mime_getarg(&(mime_current->charset),&s,&l);
      cs = decode_charset(mime_current->charset.s,
		mime_current->charset.len);
      if (cs == CS_BAD) cs = csbase;			/* keep base cs */
      else
	if (!stralloc_copy(&curcharset,&mime_current->charset)) die_nomem();
    } else {						/* skip non LWSP */
      for (;;) {
	if (!l) break;
	if (*s == '"') {
	  s++, l--;
	  while (l && *s != '"') { s++, l--; }
	  if (l) { s++, l--; }
	  break;
	} else {
	  if (!l || *s == ' ' || *s == '\t' || *s == '\n') break;
	  s++; l--;
	}
      }
    }
  }
  return;
}

void decode_transfer_encoding(char *s,unsigned int l)
{
  unsigned int r;
  mime_current->ctenc = CTENC_NONE;
  if (!l || (mime_current->mimetype & MIME_MULTI)) return;
			/* base64/QP ignored for multipart */
  r = CTENC_NONE;
  while (l && (*s == ' ' || *s == '\t')) { s++; l--; }	/* skip LWSP */
  s[l-1] = 0;
  if (case_startb(s,l,"quoted-printable")) {
    r = CTENC_QP;
  } else if (case_startb(s,l,"base64")) {
    r = CTENC_BASE64;
  }
  mime_current->ctenc = r;
  return;
}

int check_boundary()
/* return 0 if no boundary, 1 if start, 2 if end */
{
  mime_info *tmp;

  if (*line.s != '-' || line.s[1] != '-') return 0;
  tmp = mime_current;
  while (tmp) {
    if (tmp->boundary.len) {
    if (line.len > tmp->boundary.len + 2 &&
	!case_diffb(line.s+2,tmp->boundary.len,tmp->boundary.s)) {
      if (line.s[tmp->boundary.len + 2] == '-' &&
		line.s[tmp->boundary.len + 3] == '-') {	/* end */
	mime_current = tmp;
	clear_mime();
	return 2;

      } else {						/* start */
	mime_current = tmp;
	new_mime();
	return 1;
      }
    }
    }
    tmp = tmp->previous;
  }
  if (!stralloc_copys(&curcharset,charset)) die_nomem();
			/* suprtfluous since header done by now */
  cs = csbase;
  return 0;
}

void start_message_page(struct msginfo *infop)
/* header etc for message. Delayed to collect subject so that we can put */
/* that in TITLE. This in turn needed for good looking robot index.      */
/* Yep, not pretty, but it works and it's abhorrent to seek()/rewind     */
/* and another hack: it's hard to mix charsets within a doc. So, we disp */
/* messages entirely in the charset of the message. This is ok, since    */
/* headers will be us-ascii or have encoded segments usually matching    */
/* the charset in the message. Of course, we should be able to used e.g. */
/* <div charset=iso-2022-jp> with internal resources as well as internal */
/* ones. One might make other-charset messages external resources as well*/
/* Now, the problem is that we need to "preview" MIME info _before_      */
/* seeing the start boundary. */
{
  if (!stralloc_copyb(&decline,strnum,fmt_ulong(strnum,infop->target)))
	die_nomem();
  if (!stralloc_cats(&decline,":")) die_nomem();
  if (!stralloc_0(&decline)) die_nomem();
  decodeHDR(hdr[HDR_SUBJECT - 1].s,hdr[HDR_SUBJECT - 1].len,&line);
  if (!mime_current)
    new_mime();			/* allocate */
  else
    clear_mime();
  decode_mime_type(hdr[HDR_CT - 1].s,hdr[HDR_CT - 1].len,
	hdr[HDR_VERSION - 1].len);
  html_header(decline.s,line.s,line.len - 1,
		"msgbody",SPC_BASE);
  decline.len = 0;		/* reset */
  msglinks(infop);
  oputs("<div class=\"message\">\n");
}

void show_part(struct msginfo *infop,int flagshowheaders,int flagstartseen)
/* if flagshowheaders we display headers, otherwise not */
/* if flagstartseen we've already see the start boundary for this part, */
/* if not we'll ignore what's there up to it */
/* if flagskip we skip this part */
{
  char *cp;
  int flaginheader;
  int whatheader;
  int flaggoodfield;
  int flaghtml;
  int btype,i;
  unsigned int colpos;
  char linetype;

  flaginheader = 1;
  for (i = 0; i < NO_HDRS; i++) hdr[i].len = 0;
  flaggoodfield = 1;
  match = 1;
  recursion_level++;			/* one up */
  for (flaghtml = whatheader = 0;;) {
    if (!match) return;
    if (getln(&ssin,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,MSG1("ERR_READ",fn.s));
    if (!match) return;
    if ((btype = check_boundary())) {
      if (decline.len) {		/* flush last line that doesn't */
	if (flaghtml)			/* end in \n for QP/base64 */
	  oput(decline.s,decline.len);
	else
          anchor_put(decline.s,decline.len);
        decline.len = 0;
      }
      if (flagpre) {			/* ending part was <pre> */
	oputs("</pre>");
	toggle_flagpre(0);
      }
      if (mime_current->level < recursion_level) {
        return;
      }
      if (btype == 1) {
	flagstartseen = 1;
	flaggoodfield = 1;
	flaginheader = 1;
      } else
	flagstartseen = 0;
      continue;
    }
    if (!flagstartseen) continue;	/* skip to start */
    if (flaginheader) {
      if (line.len == 1) {
	if (flagshowheaders) {		/* rfc822hdr only */
	  if (flagtoplevel)
	    start_message_page(infop);	/* so we can put subj in TITLE */
	  oputs("<div class=\"rfc822hdr\"><hr />\n");
	  for (i = 0; i < NO_HDRS; i++) {
	    if (!hdr[i].len || !headers_shown[i]) continue;
	    if (i == HDR_SUBJECT - 1 && flagtoplevel)
	      oputs("<span class=\"subject\">");
	    oputs("<em>");
	    oputs(constmap_get(&headermap,i + 1));
	    oputs(":</em>");
	    decodeHDR(hdr[i].s,hdr[i].len,&line);
	    if (i == HDR_SUBJECT - 1 && flagtoplevel) {
	      oputs("<a class=\"relk\" href=\"mailto:");
	      oputs(local);
	      oput("@",1);
	      oputs(host);
	      oputs("?subject=");
	      urlencode_put(line.s + 1,line.len - 2);
	      oputs("\">");
	    }
	    if (flagobscure && i == HDR_FROM - 1) {
	      int k;
	      oputs(" ");
	      k = author_name(&cp,line.s,line.len);
	      decodeHDR(cp,k,&decline);
	      html_put(decline.s,decline.len);
	    } else {
	      decodeHDR(hdr[i].s,hdr[i].len,&decline);
              html_put(decline.s,decline.len - 1);
	    }
	    if (i == HDR_SUBJECT - 1 && flagtoplevel)
	      oputs("</a></span>");
	    oputs("\n<br />");
	  }
	  oputs("</div>\n");
	}
        flaginheader = 0;
	flagtoplevel = 0;
        flaggoodfield = 1;
	flaghtml = 0;
	if (!flagmime)
	  flagmime = hdr[HDR_VERSION - 1].len;	/* MIME-Version header */
	decode_mime_type(hdr[HDR_CT - 1].s,hdr[HDR_CT - 1].len,flagmime);
	decode_transfer_encoding(hdr[HDR_CTENC - 1].s,hdr[HDR_CTENC - 1].len);
	content.len = 0; encoding.len = 0;
	switch (mime_current->mimetype) {
	  case MIME_MULTI_SIGNED:
	  case MIME_MULTI_MIXED:
	  case MIME_MULTI_ALTERNATIVE:
	  case MIME_MULTI_DIGEST:
		show_part(infop,0,0);
		recursion_level--;
		flagstartseen = 0;
		flaginheader = 1;
		continue;
	  case MIME_MESSAGE_RFC822:
		oputs("\n<pre>");
		toggle_flagpre(1);
		flagshowheaders = 1;
		flaginheader = 1;
		flagmime = 0;		/* need new MIME-Version header */
		continue;
	  case MIME_TEXT_HTML:
		if (flagshowhtml) {
		  oputs("<hr />\n");
		  flaghtml = 1;
		} else {
		  oputs("<strong>[\"");
		  oput(mime_current->ctype.s,mime_current->ctype.len);
		  oputs("\" not shown]</strong>\n");
		  flaggoodfield = 0;	/* hide */
		}
		continue;
	  case MIME_TEXT_PLAIN:
	  case MIME_TEXT:		/* in honor of Phil using "text" on */
	  case MIME_NONE:		/* the qmail list and rfc2045:5.2 */
		oputs("<hr />\n<pre>\n");
		toggle_flagpre(1);
		continue;
	  case MIME_TEXT_VCARD:
	  default:		/* application/octetstream...*/
		oputs("<hr /><strong>[\"");
		oput(mime_current->ctype.s,mime_current->ctype.len);
		oputs("\" not shown]</strong>\n");
		flaggoodfield = 0;	/* hide */
		continue;
	}
      } else if (line.s[0] != ' ' && line.s[0] != '\t') {
	linetype = ' ';
        flaggoodfield = 0;
	colpos = byte_chr(line.s,line.len,':');
	if ((whatheader = constmap_index(&headermap,line.s,colpos))) {
          flaggoodfield = 1;
	  if (!stralloc_copyb(&hdr[whatheader - 1],line.s + colpos + 1,
		line.len - colpos - 1)) die_nomem();
	}
      } else {
	if (whatheader)
	  if (!stralloc_catb(&hdr[whatheader - 1],line.s,line.len))
		die_nomem();
      }
    } else {
      if (flaggoodfield) {
	if (mime_current->ctenc) {
	  if (mime_current->ctenc == CTENC_QP)
	    decodeQ(line.s,line.len,&decline);
	  else
	    decodeB(line.s,line.len,&decline);
	  if (decline.s[decline.len - 1] == '\n') {	/* complete line */
	    if (!stralloc_copy(&line,&decline)) die_nomem();
	    decline.len = 0;
	  } else				/* incomplete - wait for next */
	    line.len = 0;			/* in case URL is split */
	}
	if (flaghtml)
	  oput(line.s,line.len);
	else {
          anchor_put(line.s,line.len);		/* body */
	}
      }
    }
  }
}

int show_message(struct msginfo *infop)
{
  char *psz;

  if(!stralloc_copys(&headers,(char *) headers_used)) die_nomem();
  if (!stralloc_0(&headers)) die_nomem();
  psz = headers.s;
  while (*psz) {
    if (*psz == '\\') *psz = '\0';
    ++psz;
  }
  if (!constmap_init(&headermap,headers.s,headers.len,0))
	die_nomem();

  (void) makefn(&fn,ITEM_MESSAGE,msginfo.target,"");
  if ((fd = open_read(fn.s)) == -1) {
    if (errno == error_noent)
      return 0;
    else
      strerr_die2sys(111,FATAL,MSG1("ERR_OPEN",fn.s));
  }
  substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));
  toggle_flagpre(0);
  recursion_level = 0;	/* recursion level for show_part */
  flagmime = 0;		/* no active mime */
  flagtoplevel = 1;	/* top message/rfc822 get special rx */
  new_mime();		/* initiate a MIME info storage slot */

  show_part(infop,1,1);	/* do real work, including html header etc */
  if (flagpre)
    oputs("</pre>\n");
  close(fd);
  oputs("<hr /></div>\n");
  msglinks(infop);
  html_footer(0);
  return 1;
}

int decode_item(char ch)
{
  switch (ch) {
	case 'm': return ITEM_MESSAGE;
	case 'a': return ITEM_AUTHOR ;
	case 's': return ITEM_SUBJECT;
	case 'd': return ITEM_DATE   ;
	case 'i': return ITEM_INDEX  ;
	default: cgierr("Navigation command contains ",
		"illegal item code","");
  }
  return 0;	/* never reached */
}

int decode_direction(char ch)
{
  switch (ch) {
	case 's': return DIRECT_SAME;
	case 'n': return DIRECT_NEXT;
	case 'p': return DIRECT_PREV;
	default: cgierr("Navigation command contains ",
		"illegal direction code","");
  }
  return 0;	/* never reached */
}

int decode_cmd(char *s,struct msginfo *infop)
/* decodes s into infop. Assures that no security problems slip through by */
/* checking everything */
/* commands xyd:123[:abc]. x what we want, y is the axis, d the direction. */
/* 123 is the current message number. abc is a date/subject/author hash,   */
/* depending on axis, or empty if not available. */
/* returns: 0 no command+msgnum. */
/*          1 empty or at least cmd + msgnum. */
/*            Guarantee: Only legal values accepted */
{
  char ch;

  infop->source = 0L;
  infop->date = 0L;
  infop->author = (char *)0;
  infop->subject = (char *)0;
  infop->cgiarg = (char *)0;

  if (!s || !*s) {	/* main index */
    infop->item = ITEM_DATE;
    infop->axis = ITEM_DATE;
    infop->direction = DIRECT_SAME;
    latestdate(&msginfo,0);
    infop->target = MAXULONG;
    return 1;
  }
  ch = *(s++);
  if (ch >= '0' && ch <= '9') {	/* numeric - simplified cmd: msgnum ... */
    s--;
    infop->item = ITEM_MESSAGE;
    infop->axis = ITEM_MESSAGE;
    infop->direction = DIRECT_SAME;
  } else {			/* what:axis:direction:msgnum ... */
    infop->item = decode_item(ch);
    ch = *(s++);
    infop->axis = decode_item(ch);
    ch = *(s++);
    infop->direction = decode_direction(ch);
    if (*(s++) != ':') return 0;
  }
  s+= scan_ulong(s,&(infop->source));
  if (*(s++) != ':') return 0;
  if (*s >= '0' && *s <= '9') {	/* numeric nav hint [date] */
    s+= scan_ulong(s,&(infop->date));
    if (!*s++) return 1;	/* skip any char - should be ':' unless NUL */
  }
  if (checkhash(s)) {		/* Ignore if illegal rather than complaining*/
    if (!stralloc_copyb(&charg,s,HASHLEN)) die_nomem();
    if (!stralloc_0(&charg)) die_nomem();
    infop->cgiarg = charg.s;
  }
  return 1;
}

int msg2hash(struct msginfo *infop)
{
  unsigned int pos;
  unsigned long tmpmsg;

  if (!infop->source) die_prog("source is 0 in msg2hash");
  (void) makefn(&fn,ITEM_INDEX,infop->source,"");
  if ((fd = open_read(fn.s)) == -1) {
    if (errno == error_noent)
      return 0;
    else
      strerr_die2sys(111,FATAL,MSG1("ERR_OPEN",fn.s));
  }
  substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));
  for (;;) {
        if (getln(&ssin,&line,&match,'\n') == -1)
          strerr_die2sys(111,FATAL,MSG1("ERR_READ","index"));
        if (!match)
	  return 0;				/* didn't find message */
	if (*line.s == '\t') continue;		/* author line */
        pos = scan_ulong(line.s,&tmpmsg);
	if (tmpmsg == infop->source) {
          if (line.s[pos++] != ':' || line.s[pos++] != ' ')
	    strerr_die3x(100,MSG("ERR_SYNTAX"),fn.s,": missing subject separator");
	  if (line.len < HASHLEN + pos)
	    strerr_die3x(100,MSG("ERR_SYNTAX"),fn.s,": missing subject hash");
	  if (!stralloc_copyb(&subject,line.s+pos,HASHLEN)) die_nomem();
	  if (!stralloc_0(&subject)) die_nomem();
	  infop->subject = subject.s;
          if (getln(&ssin,&line,&match,'\n') == -1)
            strerr_die2sys(111,FATAL,MSG1("ERR_READ","index"));
          if (!match)
	    strerr_die3x(100,MSG("ERR_SYNTAX"),fn.s,
		": author info missing. Truncated?");
	  pos = byte_chr(line.s,line.len,';');
	  if (pos == line.len)
	    strerr_die3x(100,MSG("ERR_SYNTAX"),fn.s,"missing ';' after date");
	  if (pos > 1)
	    infop->date = date2yyyymm(line.s+1);	/* ';' marks end ok */
	  pos++;
	  if (line.len < HASHLEN + pos)
	    strerr_die3x(100,MSG("ERR_SYNTAX"),fn.s,": missing author hash");
	  if (!stralloc_copyb(&author,line.s+pos,HASHLEN)) die_nomem();
	  if (!stralloc_0(&author)) die_nomem();
	  infop->author = author.s;
	  close(fd);
	  return 1;	/* success */
        }
  }
  close(fd);
  return 0;		/* failed to match */
}

void setmsg(struct msginfo *infop)
/* Reads the file corresponding to infop->axis and assumes fn.s is set */
/* correctly for this. Sets up a msgnav structure and links it in      */
/* correction for axis=author/subject. For axis=date it supports also  */
/* direction=DIRECT_FIRST which will return the first message of the   */
/* first thread in the date file. DIRECT_LAST is not supported.        */
/* DIRECT_FIRST is supported ONLY for date. */
{
  if (infop->direction == DIRECT_SAME) {
    infop->target = infop->source;
    return;
  }
  if ((fd = open_read(fn.s)) == -1) {
    if (errno == error_noent)
      strerr_die2x(100,FATAL,MSG1("ERR_OPEN_LISTMSGS",fn.s));
    else
      strerr_die2sys(111,FATAL,MSG1("ERR_OPEN",fn.s));
  }
  substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));
  if (infop->axis != ITEM_DATE) {
    if (getln(&ssin,&line,&match,'\n') == -1)	/* first line */
      strerr_die2sys(111,FATAL,MSG1("ERR_READ",fn.s));
    if (!match)
      strerr_die3x(100,MSG("ERR_SYNTAX"),fn.s,": first line missing");
  }
  msgnav[3] = 0L;		/* next */
  msgnav[4] = 0L;		/* after */
  infop->target = 0L;
  for (;;) {
    if (getln(&ssin,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,MSG1("ERR_READ",fn.s));
    if (!match) break;
    msgnav[0] = msgnav[1];
    msgnav[1] = msgnav[2];
    pos = scan_ulong(line.s,&(msgnav[2]));
    if (infop->direction == DIRECT_FIRST && infop->axis == ITEM_DATE) {
      if (pos + HASHLEN + 1 < line.len)
        if (!stralloc_copyb(&subject,line.s+pos+1,HASHLEN)) die_nomem();
	if (!stralloc_0(&subject)) die_nomem();
      break;
    }
    if (msgnav[2] == infop->source) {
      if (getln(&ssin,&line,&match,'\n') == -1)
        strerr_die2sys(111,FATAL,MSG1("ERR_READ",fn.s));
      if (!match) break;
      (void) scan_ulong(line.s,&(msgnav[3]));
      if (getln(&ssin,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,MSG1("ERR_READ",fn.s));
      if (!match) break;
      (void) scan_ulong(line.s,&(msgnav[4]));
      break;
    }
  }
  close(fd);
  switch (infop->axis) {
    case ITEM_AUTHOR:
      infop->authnav = msgnav + 2 + infop->direction;
      infop->target = *(infop->authnav);
      infop->subject = (char *)0;	/* what we know is not for this msg */
      infop->date = 0;
      break;
    case ITEM_SUBJECT:
      if (infop->direction == DIRECT_FIRST)
        infop->target = msgnav[2];
      else {
        infop->subjnav = msgnav + 2 + infop->direction;
        infop->target = *(infop->subjnav);
      }
      infop->author = (char *)0;	/* what we know is not for this msg */
      infop->date = 0;
      break;
    case ITEM_DATE:
      infop->target = msgnav[2];
      infop->subject = (char *)0;	/* what we know is not for this msg */
      infop->author = (char *)0;	/* what we know is not for this msg */
      break;
    default:
      die_prog("Bad item in setmsg");
  }
  return;
}

void auth2msg(struct msginfo *infop)
{
  if (!infop->author) die_prog("no such author in authmsg");
  if (!makefn(&fn,ITEM_AUTHOR,0L,infop->author)) die_prog("auth2msg");
  setmsg(infop);
}

void subj2msg(struct msginfo *infop)
{
  if (!infop->subject) die_prog("no such subject in subj2msg");
  if (!makefn(&fn,ITEM_SUBJECT,0L,infop->subject)) die_prog("subj2msg");
  setmsg(infop);
}

void date2msg(struct msginfo *infop)
/* this is all a terrible hack */
{
  (void) makefn(&fn,ITEM_DATE,infop->date,"");
  infop->direction = DIRECT_FIRST;
  infop->axis = ITEM_DATE;
  setmsg(infop);		/* got first thread */
  infop->subject = subject.s;
  infop->axis = ITEM_SUBJECT;
  subj2msg(infop);		/* get 1st message no in that thread */
}

void findlastmsg(struct msginfo *infop)
{
  if (!getconf_ulong(&(infop->target),"num",0))
    cgierr("Sorry, there are no messages in the archive","","");
}

int do_cmd(struct msginfo *infop)
/* interprets msginfo to create msginfo. Upon return, msginfo can be trusted */
/* to have all info needed, and that all info is correct. There may be more */
/* info than needed. This can be used to build more specific links. NOTE:   */
/* there is no guarantee that a message meeting the criteria actually exists*/
{
  infop->target = infop->source;

  switch (infop->item) {
	case ITEM_MESSAGE:	/* we want to get a message back */
	  {
	    switch (infop->axis) {
	      case ITEM_MESSAGE:
	        if (infop->direction == DIRECT_SAME)
		  break;
	        else if (infop->direction == DIRECT_NEXT)
		  (infop->target)++;
	        else {		/* previous */
		  cache = 2;
		  if (infop->target >= 2)
		    (infop->target)--;
		  else
		    infop->target = 1;
	        }
		break;
	      case ITEM_AUTHOR:
	          infop->author = infop->cgiarg;
		if (!infop->author)	 /* we don't know author hash */
		  if (!msg2hash(infop)) return 0;
	        auth2msg(infop);
	        break;
	      case ITEM_SUBJECT:
	          infop->subject = infop->cgiarg;
	        if (!infop->subject)	 /* we don't know Subject hash */
		  if (!msg2hash(infop)) return 0;
	        subj2msg(infop);
	        break;
	    }
	    break;
	  }
	case ITEM_AUTHOR:
	  switch (infop->axis) {
	    case ITEM_MESSAGE:
	      if (!infop->author)
		if (!msg2hash(infop)) return 0;
	      break;
	    case ITEM_AUTHOR:
	      infop->author = infop->cgiarg;
	      if (!infop->author)
		if (!msg2hash(infop)) return 0;
	        auth2msg(infop);
	      break;
	    case ITEM_SUBJECT:
	      infop->subject = infop->cgiarg;
	      if (!infop->subject)	 /* we don't know Subject hash */
		if (!msg2hash(infop)) return 0;
	      subj2msg(infop);
	      break;
	    }
	    break;
	case ITEM_SUBJECT:
	  switch (infop->axis) {
	    case ITEM_MESSAGE:
	      if (!msg2hash(infop)) return 0;
	      break;
	    case ITEM_AUTHOR:
	      infop->author = infop->cgiarg;
	      if (!infop->author)
		if (!msg2hash(infop)) return 0;
	      auth2msg(infop);
	      break;
	    case ITEM_SUBJECT:
	      infop->subject = infop->cgiarg;
	      if (!infop->subject)	 /* we don't know Subject hash */
		if (!msg2hash(infop)) return 0;
	      subj2msg(infop);
	      break;
	    }
	    break;
	  case ITEM_DATE:	/* want a date reference */
	    switch (infop->axis) {
	      case ITEM_MESSAGE:
	      case ITEM_AUTHOR:
	      case ITEM_SUBJECT:
	      case ITEM_DATE:
		if (!infop->date && infop->source)
		  if (!msg2hash(infop)) return 0;
		  gtdate(infop,0);
		break;
	    }
	    break;
	  case ITEM_INDEX:	/* ignore direction etc - only for index */
	    if (!infop->target)
	      infop->target = infop->source;
	    break;
  }
  return 1;
}

void list_lists()
{
  unsigned long lno;
  cache = 2;
  flagrobot = 2;
  html_header("Robot index of lists",0,0,0,0);
  for (;;) {
    if (getln(&ssin,&cfline,&match,'\n') == -1)		/* read line */
      strerr_die2sys(111,FATAL,MSG1("ERR_READ",fn.s));
    if (!match)
      break;
    if (cfline.s[0] == '#') continue;			/* skip comment */
    cfline.s[cfline.len - 1] = '\0';			/* so all are sz */
    (void) scan_ulong(cfline.s,&lno);			/* listno for line */
    if (lno) {				/* don't expose default list */
      oputs("<a href=\"");
      oput(strnum,fmt_ulong(strnum,lno));
      oputs("/index\">[link]</a>\n");
   }
  }
  html_footer(0);
}

void list_list(unsigned long listno)
/* Make one link [for list_set()] per set of 100 archive messages. */
/* Assumption: Any directory DIR/archive/xxx where 'xxx' is a numeric,*/
/* is part of the list archive and has in it an index file and one    */
/* or more messages. */
{
  DIR *archivedir;
  direntry *d;
  unsigned long msgset;

  flagrobot = 2;
  strnum[fmt_ulong(strnum,listno)] = '\0';
  archivedir = opendir("archive/");
  if (!archivedir)
    strerr_die2sys((errno == error_noent) ? 100 : 111,
		   FATAL,MSG1("ERR_OPEN","archive"));
  cache = 1;
  html_header("Robot index for message sets in list",0,0,0,0);

  while ((d = readdir(archivedir))) {
    if (d->d_name[scan_ulong(d->d_name,&msgset)])
	continue;		/* not numeric */
    oputs("<a href=\"../");	/* from /ezcgi/0/index to /ezcgi/listno/index*/
    oputs(strnum);
    oputs("/index/");
    oputs(d->d_name);
    oputs("\">[link]</a>\n");
  }
  closedir(archivedir);
  html_footer(0);
}

void list_set(unsigned long msgset)
{
  unsigned int msgfirst,msgmax;
  unsigned long lastset;

  flagrobot = 2;
  findlastmsg(&msginfo);
  if (!stralloc_copys(&line,"<a href=\"../")) die_nomem();
  if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,msgset))) die_nomem();
  lastset = msginfo.target / 100;
  cache = 2;
  msgfirst = 0;
  if (!msgset)
    msgfirst = 1;
  msgmax = 99;
  if (msgset > lastset) {		/* assure empty list */
    msgmax = 0;
    msgfirst = 1;
  } else if (msgset == lastset) {
    cache = 0;				/* still changing */
    msgmax = msginfo.target % 100;
  }
  html_header("Robot index for messages in set",0,0,0,0);
  while (msgfirst <= msgmax) {
    oput(line.s,line.len);
    oput(strnum,fmt_uint0(strnum,msgfirst,2));
    oputs("\">[link]</a>\n");
    msgfirst++;
  }
  html_footer(0);
}

/**************** MAY BE SUID ROOT HERE ****************************/
void drop_priv(int flagchroot)
{
  if (!uid) strerr_die2x(100,FATAL,MSG("ERR_SUID"));		/* not as root */
  if (!euid) {
    if (flagchroot)
      if (chroot(dir) == -1)				/* chroot listdir */
        strerr_die4sys(111,FATAL,"failed to chroot ",dir,": ");
    if (setuid(uid) == -1)				/* setuid */
      strerr_die2sys(111,FATAL,MSG("ERR_SETUID"));
  }
  euid = (unsigned long) geteuid();
  if (!euid) strerr_die2x(100,FATAL,MSG("ERR_SUID"));		/* setuid didn't do it*/
}
/*******************************************************************/

int main(int argc,char **argv)
{
  char *cp,*cppath;
  unsigned long listno,thislistno,tmpuid,msgset;
  unsigned long msgnum = 0;
  unsigned long port = 0L;
  unsigned long tmptarget;
  unsigned int pos,l;
  int flagindex = 0;
  int flagchroot = 1;		/* chroot listdir if SUID root */
  int ret;
  char sep;

/******************** we may be SUID ROOT ******************************/
  uid = (unsigned long) getuid();			/* should be http */
  euid = (unsigned long) geteuid();			/* chroot only if 0 */

  if (!euid) {
    if (!stralloc_copys(&line,auto_etc())) die_nomem();
    if (!stralloc_cats(&line,EZ_CGIRC)) die_nomem();
    if (!stralloc_0(&line)) die_nomem();
    if ((fd = open_read(line.s)) == -1)			/* open config */
      strerr_die2sys(111,FATAL,MSG1("ERR_OPEN",line.s));
  } else {
    if ((fd = open_read(EZ_CGIRC_LOC)) == -1)		/* open local config */
      strerr_die2sys(111,FATAL,MSG1("ERR_OPEN",EZ_CGIRC_LOC));
  }

  substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));	/* set up buffer */
	/* ##### tainted info #####*/

  cmd = env_get("QUERY_STRING");			/* get command */
  cppath = env_get("PATH_INFO");			/* get path_info */

  if (!cmd && !cppath)
    cmd = argv[1];

  if (!cppath || !*cppath) {
    if (cmd && *cmd) {
      cmd += scan_ulong(cmd,&thislistno);
      if (*cmd == ':') cmd++;				/* allow ':' after ln*/
    } else
      thislistno = 0L;
  } else {
    if (*cppath == '/') cppath++;
      cppath += scan_ulong(cppath,&thislistno);		/* this listno */
      if (!thislistno || *cppath++ == '/') {
	if (str_start(cppath,"index")) {
          cppath += 5;
	  flagindex = 1;
	  if (!thislistno) {				/* list index */
	    drop_priv(0);	/* <---- dropping privs */
	    list_lists();
	    close(fd);
	    _exit(0);
	  }
	}
      }							/* rest done per list */
    }

  for (sep = pos = 0;;) {
    if (getln(&ssin,&cfline,&match,'\n') == -1)		/* read line */
      strerr_die2sys(111,FATAL,MSG1("ERR_READ",fn.s));
    if (!match)
      break;
    if (*cfline.s == '#' || cfline.len == 1) continue;	/* skip comment/blank */
    cfline.s[cfline.len - 1] = '\0';			/* so all are sz */
    pos = scan_ulong(cfline.s,&listno);			/* listno for line */
    if (thislistno != listno) continue;
    sep = cfline.s[pos++];
    if (cfline.s[pos] == '-') {				/* no chroot if -uid*/
      flagchroot = 0;
      pos++;
    }
    pos += scan_ulong(cfline.s+pos,&tmpuid);		/* listno for line */
    if (tmpuid) uid = tmpuid;				/* override default */
    if (!cfline.s[pos++] == sep)
      die_syntax("missing separator after user id");
    if (cfline.s[pos] != '/')
	die_syntax("dir");				/* absolute path */
    l = byte_chr(cfline.s + pos, cfline.len - pos,sep);
    if (l == cfline.len - pos)				/* listno:path:...*/
      die_syntax("missing separator after path");
    dir = cfline.s + pos;
    pos += l;
    cfline.s[pos++] = '\0';				/* .../dir\0 */
    break;	/* do rest after dropping priv */
  }
  close(fd);						/* don't accept uid 0*/
  if (!dir) {
    drop_priv(0);	/* don't trust cgierr. No dir, no chroot */
    cgierr("list ",MSG("ERR_NOEXIST"),"");
  }
  wrap_chdir(dir);
  drop_priv(flagchroot);

/******************************* RELAX **********************************/

/********************* continue to process config line ******************/

  flagrobot = 0;
  if (cfline.s[pos] == '-') {
    flagobscure = 1;
    pos++;
  }
  local = cfline.s + pos;
  l = byte_chr(cfline.s + pos, cfline.len - pos,sep);	/* ... home */
  if (l < cfline.len - pos) {				/* optional */
    pos += l;
    cfline.s[pos++] = '\0';
    home = cfline.s + pos;
    l = byte_chr(cfline.s + pos, cfline.len - pos,sep);	/* ... charset */
    if (l < cfline.len - pos) {				/* optional */
      pos += l;
      cfline.s[pos++] = '\0';
      charset = cfline.s + pos;
      l = byte_chr(cfline.s+pos,cfline.len - pos,sep);	/* ... stylesheet */
      if (l < cfline.len - pos) {			/* optional */
        pos += l;
        cfline.s[pos++] = '\0';
        stylesheet = cfline.s + pos;
        l = byte_chr(cfline.s+pos,cfline.len-pos,sep);	/* ... bannerURL */
	if (l < cfline.len - pos) {			/* optional */
	  pos += l;
	  cfline.s[pos++] = '\0';
	  banner = cfline.s + pos;
	}
      }
    }
  }
  if (!charset || !*charset)				/* rfc822 default */
    charset = EZ_CHARSET;
  if (!stralloc_copys(&curcharset,charset)) die_nomem();
  csbase = decode_charset(curcharset.s,curcharset.len);
  if (csbase == CS_BAD) csbase = CS_NONE;
  cs = csbase;
  pos = + str_rchr(local,'@');
  if (!local[pos])
    die_syntax("listaddress lacks '@'");		/* require host */
  local[pos++] = '\0';
  host = local + pos;

/********************* Accomodate robots and PATH_INFO ****************/

  if (flagindex) {
    if (*(cppath++) == '/') {		/* /2/index/123 */
      cppath += scan_ulong(cppath,&msgset);
      list_set(msgset);
    } else				/* /2/index */
      list_list(thislistno);
    _exit(0);
  }

  if (cppath && *cppath) {		/* /2/msgnum */
    flagrobot = 1;			/* allow index, but "nofollow" */
    scan_ulong(cppath,&msgnum);
  }					/* dealt with normally */

/********************* Get info from server on BASE etc ****************/

  if (!stralloc_copys(&url,"<a href=\"")) die_nomem();
  if (!stralloc_copys(&base,"<base href=\"http://")) die_nomem();
  cp = env_get("SERVER_PORT");
  if (cp) {			/* port */
    (void) scan_ulong(cp,&port);
    if ((unsigned int) port == 443) {		/* https: */
      if (!stralloc_copys(&base,"<base href=\"https://")) die_nomem();
    }
  }
  if ((cp = env_get("HTTP_HOST")) || (cp = env_get("SERVER_NAME"))) {
    if (!stralloc_cats(&base,cp)) die_nomem();
    if (port && (unsigned int) port != 80 && (unsigned int) port != 443) {
      if (!stralloc_cats(&base,":")) die_nomem();
      if (!stralloc_catb(&base,strnum,fmt_ulong(strnum,port))) die_nomem();
    }
  }
  if ((cp = env_get("SCRIPT_NAME")) != 0) {
    if (!stralloc_cats(&base,cp)) die_nomem();
    pos = str_rchr(cp,'/');
    if (cp[pos])
      if (!stralloc_cats(&url,cp + pos + 1)) die_nomem();
  }
  if (!stralloc_cats(&base,"\" />\n")) die_nomem();
  if (!stralloc_cats(&url,"?")) die_nomem();
  if (thislistno) {
    if (!stralloc_catb(&url,strnum,fmt_ulong(strnum,thislistno))) die_nomem();
    if (!stralloc_cats(&url,":")) die_nomem();
  }

  cache = 1;				/* don't know if we want to cache */

/****************************** Get command ****************************/

  if (msgnum) {				/* to support /listno/msgno */
   msginfo.target = msgnum;
   msginfo.item = ITEM_MESSAGE;
   cache = 2;
  } else {
      (void) decode_cmd(cmd,&msginfo);
      if (!do_cmd(&msginfo))
	cgierr("I'm sorry, Dave ... I can't do that, Dave ...","","");
  }

  switch (msginfo.item) {
    case ITEM_MESSAGE:
	if (!(ret = show_message(&msginfo))) {	/* assume next exists ... */
	  cache = 0;				/* border cond. - no cache */
	  msginfo.target = msginfo.source;	/* show same */
	  msginfo.subjnav = 0;
	  msginfo.authnav = 0;
	  ret = show_message(&msginfo);
	}
	break;
    case ITEM_AUTHOR:
	if (!(ret = show_object(&msginfo,ITEM_AUTHOR)))
	  cgierr ("I couldn't find the author for that message","","");
	break;
    case ITEM_SUBJECT:
	if (!(ret = show_object(&msginfo,ITEM_SUBJECT)))
	  cgierr ("I couldn't find the subject for that message","","");
	break;
    case ITEM_DATE:
	if (!(ret = show_object(&msginfo,ITEM_DATE))) {
	  finddate(&msginfo);
	  ret = show_object(&msginfo,ITEM_DATE);
	}
	break;
    case ITEM_INDEX:
	ret = 1;
	if (show_index(&msginfo)) break;/* msgnumber valid */
	tmptarget = msginfo.target;
	findlastmsg(&msginfo);
	cache = 0;			/* latest one - no cache */
	if (msginfo.target > tmptarget) {
	  cache = 2;			/* first one won't change */
	  msginfo.target = 1;		/* try */
	  if (show_index(&msginfo)) break;
	  msginfo.date = 0;		/* first indexes missing */
          firstdate(&msginfo);		/* instead get first msg of first */
	  date2msg(&msginfo);		/* thread. */
	  if (show_index(&msginfo)) break;
	} else
	  ret = show_index(&msginfo);
	break;
    default:
	strerr_die2x(100,FATAL,"bad item in main");
  }
  if (!ret) {
    findlastmsg(&msginfo);		/* as last resort; last msgindex */
    cache = 0;
    ret = show_message(&msginfo);
  }

 _exit(0);
 (void)argc;
}
