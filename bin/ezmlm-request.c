#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "stralloc.h"
#include "subfd.h"
#include "strerr.h"
#include "error.h"
#include "qmail.h"
#include "env.h"
#include "sender.h"
#include "sig.h"
#include "open.h"
#include "getln.h"
#include "case.h"
#include "str.h"
#include "datetime.h"
#include "now.h"
#include "quote.h"
#include "readwrite.h"
#include "exit.h"
#include "substdio.h"
#include "getconf.h"
#include "constmap.h"
#include "fmt.h"
#include "getconfopt.h"
#include "byte.h"
#include "seek.h"
#include "messages.h"
#include "copy.h"
#include "cookie.h"
#include "subdb.h"
#include "mime.h"
#include "hdr.h"
#include "die.h"
#include "idx.h"
#include "config.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-request: fatal: ";
const char INFO[] = "ezmlm-request: info: ";
const char USAGE[] =
"ezmlm-request: usage: ezmlm-request [-f lists.cfg] dir";

static const char *cfname = (char *) 0;
static unsigned long copylines = 0; /* Number of lines from the message to copy */
static struct option options[] = {
  OPT_CSTR(cfname,'f',0),
  OPT_CSTR(cfname,'F',0),
  OPT_ULONG(copylines,0,"copylines"),
  OPT_END
};

static char *userlocal = (char *) 0;
static char *userhost = (char *) 0;
static char *listlocal = (char *) 0;
static char *listhost = (char *) 0;
static char *command = (char*)"help";
stralloc line = {0};
static stralloc listname = {0};
static stralloc hostname = {0};
stralloc quoted = {0};
char boundary[COOKIE] = "zxcaeedrqcrtrvthbdty";	/* cheap "rnd" MIME boundary */

static struct constmap headerremovemap;
static int headerremoveflag = 0;
static struct constmap commandmap;
static int flaggotsub = 0;		/* Found a subject */
	/* cmdstring has all commands seperated by '\'. cmdxlate maps each */
	/* command alias to the basic command, which is used to construct  */
	/* the command address (positive numbers) or handled by this       */
	/* program (negative numbers). Note: Any command not matched is    */
	/* used to make a command address, so ezmlm request can handle     */
	/* ("transmit") user-added commands.                               */
static const char cmdstring[] =
		"system\\help\\"			/* 1,2 */
		"subscribe\\unsubscribe\\index\\"	/* 3,4,5 */
		"info\\list\\query\\"			/* 6,7,8 */
		"sub\\unsub\\remove\\signoff\\"		/* 9,10,11,12 */
		"lists\\which\\"			/* 13,14 */
		"ind\\rev\\review\\recipients\\"	/* 15,16,17,18 */
		"who\\showdist\\"			/* 19,20 */
		"put\\set";				/* 21,22 */

	/* map aliases. -> 0 not recognized. -> 1 recognized will be made    */
	/* help and arguments scrapped. < 0 handled locally. HELP without    */
	/* args also handled locally */
			/* the last are not supported -> help */
static const int cmdxlate[] = { 0,1,2,3,4,5,6,7,8,3,4,4,4,-13,-14,5,7,7,7,7,7,
				1,1 };

	/* If there are no arguments (listlocal = 0) then commands are mapped*/
	/* through this. This way, help, list, query, ... can mean something */
	/* here even though they have local funcions at the lists if used    */
	/* with arguments. (Made same lengh as cmdxlate in case of bugs.)    */
	/* Note: This is used ONLY for the global interface */
static const int noargsxlate[] = { 0,1,-2,3,4,5,-2,-13,-14,9,10,11,12,13,14,15,16,17,
				   18,19,20,21,22 };

	/* these need to be defined as the index of the corresponding      */
	/* commands. They are handled by ezmlm-request. NOTE: Help is >0!  */
#define EZREQ_LISTS 13
#define EZREQ_WHICH 14
#define EZREQ_HELP  2
#define EZREQ_BAD 1

static substdio sstext;
static char textbuf[1024];

struct qmail qq;

static char inbuf[1024];
static substdio ssin = SUBSTDIO_FDBUF(read,0,inbuf,(int) sizeof(inbuf));
static substdio ssin2 = SUBSTDIO_FDBUF(read,0,inbuf,(int) sizeof(inbuf));

static stralloc mydtline = {0};

static int code_qput(const char *s,unsigned int n)
{
  static stralloc qline = {0};

    if (!flagcd)
      qmail_put(&qq,s,n);
    else {
      if (flagcd == 'B')
        encodeB(s,n,&qline,0);
      else
        encodeQ(s,n,&qline);
      qmail_put(&qq,qline.s,qline.len);
    }
    return 0;		/* always succeeds */
}

/* Checks the argument. Only  us-ascii letters, numbers, ".+-_" are ok. */
/* NOTE: For addresses this is more restrictive than rfc821/822.        */
static void checkarg(const char *s)
{
  const char *cp;
  char ch;
  cp = s;
  if (!cp) return;				/* undef is ok */
  while ((ch = *cp++)) {
    if (ch >= 'a' && ch <= 'z')
	 continue;				/* lc letters */
    if (ch >= '0' && ch <='9')			/* digits */
	continue;
    if (ch == '.' || ch == '-' || ch == '_' || ch == '+')
	continue;				/* ok chars */
    if (ch >= 'A' && ch <= 'Z') continue;	/* UC LETTERS */
    strerr_die4x(100,MSG(ERR_NOT_CLEAN),": \"",s,"\"");
  }
  return;
}

/* parses line poited to by cp into sz:s as per:                        */
/* 1. listlocal-command-userlocal=userhost@listhost                     */
/* 2. command userlocal@userhost                                        */
/* 3. command userlocal@userhost listlocal@listhost                     */
/* 4. command listlocal@listhost                                        */
/* 5. command listlocal[@listhost] userlocal@userhost                   */
/* 6. which [userlocal@userhost]					*/
/* The first 3 are valid only if !cfname, i.e. -request operation and   */
/* listlocal and listhost are always set to outlocal@outhost. Options   */
/* 4-5 are for the global address (cfname is set). Here listhost is     */
/* taken from the first list in *cfname matching listlocal, or set to   */
/* outhost, if not specified. If specified, it's accepted if it matches */
/* a list in *cfname and silently set to outhost otherwise. Pointers to */
/* unspecified parts are set to NULL in this routine to be dealt with   */
/* elsewhere. "Which" special argument order (6) is fixed elsewhere.    */
/* If listhost is not given, "@outhost" is added. Absence of 'userhost' */
/* is accepted to allow commands that take arguments that are not       */
/* addresses (e.g. -get12-34).                                          */

static void parseline(char *cp)
{
  char *cp1;
  char *cp2;
  char *cp3;

  cp1 = cp;
  while (*cp1) {				/* make tabs into spaces */
    if (*cp1 == '\t') *cp1 = ' ';
    ++cp1;
  }
  if (str_len(cp) >= outlocal.len && cp[outlocal.len] == '-'
      && case_startb(cp,outlocal.len,outlocal.s)) {	/* normal ezmlm cmd */
    command = cp + listname.len;		/* after the '-' */
    listlocal = listname.s;
    listhost = hostname.s;
    cp1 = command;
    while (*cp1 && *cp1 != '-') ++cp1;		/* find next '-' */
    if (*cp1) {
      *cp1 = '\0';
      userlocal = ++cp1;			/* after '-' */
      cp1 = cp1 + str_rchr(cp1,'@');		/* @ _or_ end */
      *cp1 = '\0';				/* last '=' in userlocal */
      cp1 = userlocal + str_rchr(userlocal,'=');
      if (*cp1) {				/* found '=' */
        *cp1 = '\0';				/* zap */
        userhost = cp1 + 1;			/* char after '=' */
      }
    }
  } else {				/* '@' before ' ' means complete cmd */
    if (str_chr(cp,'@') < str_chr(cp,' '))	/* addr where local failed */
	strerr_die2x(100,FATAL,MSG(ERR_REQ_LOCAL));
						/* to match */
    command = cp;
    cp1 = cp + str_chr(cp,' ');
    if (*cp1) {
      *cp1++ = '\0';
      while (*cp1 && *cp1 == ' ') ++cp1;	/* skip spaces */
    }
    cp2 = 0;
    if (*cp1) {					/* argument */
      cp2 = cp1 + str_chr(cp1,' ');
      cp3 = cp2;
      while (*cp2 && *cp2 == ' ') ++cp2;	/* skip spaces */
      *cp3 = '\0';

      if (!*cp2)
        cp2 = 0;
      else {
        cp3 = cp2 + str_chr(cp2,' ');
        *cp3 = '\0';
      }
    } else
      cp1 = 0;

    if (!cfname && !cp2) {	/* the single arg is user if we serve a */
      cp2 = cp1;		/* list. It's list if we serve "domo@" */
      cp1 = 0;
    }
    if (cp2) {
      userlocal = cp2;
      cp2 += str_chr(cp2,'@');
      if (*cp2) {
        *cp2++ = '\0';
        userhost = cp2;
      }
    }
    if (cp1) {
      listlocal = cp1;
      cp1 += str_chr(cp1,'@');
      if (*cp1) {
        *cp1++ = '\0';
        listhost = cp1;
      }
    }
  }
  checkarg(command);			/* better safe than sorry */
  checkarg(userlocal); checkarg(userhost);
  checkarg(listlocal); checkarg(listhost);
}

int main(int argc,char **argv)
{
  const char *local;
  char *action;
  char *def;
  const char *sender;
  char *psz;
  const char *err;
  int cmdidx;
  int flagok;
  int flagnosubject;
  int match;
  int flaginheader;
  int flagbadfield;
  int flagmultipart = 0;
  int fd;
  unsigned int pos,pos1,len,last;
  char strnum[FMT_ULONG];
  stralloc usr = {0};
  stralloc lhost = {0};
  stralloc subject = {0};
  stralloc headerremove = {0};
  stralloc cmds = {0};
  stralloc from = {0};
  stralloc to = {0};

  (void)umask(022);
  sig_pipeignore();

  getconfopt(argc,argv,options,1,0);

	/* do minimum to identify request for this program in case */
	/* it's invoked in line with e.g. ezmlm-manage */

  def = env_get("DEFAULT");
  if (def) {
    action = def;
  } else if (cfname) {		/* just list-mdomo */
    local = env_get("LOCAL");
    if (!local) strerr_die2x(100,FATAL,MSG(ERR_NOLOCAL));
    len = str_len(local);
    if (len >= 8 && !case_diffb(local + len - 8,8,"-return-")) {
      action = (char*)"return-";	/* our bounce with qmail<1.02 */
    } else
      action = (char*)"";	/* list-mdomo-xxx won't work for older lists */
  } else
    strerr_die3x(100,FATAL,MSG(ERR_NODEFAULT)," and -f not used");
	/* at this point action = "request" or "request-..." for std use; */
	/* "" for majordomo@ */
  if (!cfname) {				/* expect request */
    if (case_starts(action,ACTION_REQUEST))
      action += str_len(ACTION_REQUEST);
    else if (case_starts(action,ALT_REQUEST))
      action += str_len(ALT_REQUEST);
    else
      _exit(0);					/* not for us */
  }

  if (!stralloc_copy(&listname,&outlocal)) die_nomem();
  if (!stralloc_copy(&hostname,&outhost)) die_nomem();
  if (!stralloc_0(&listname)) die_nomem();
  if (!stralloc_0(&hostname)) die_nomem();

  sender = get_sender();
  if (!sender) strerr_die2x(99,INFO,MSG(ERR_NOSENDER));
  if (!*sender)
    strerr_die2x(99,INFO,MSG(ERR_BOUNCE));
  if (!sender[str_chr(sender,'@')])
    strerr_die2x(99,INFO,MSG(ERR_ANONYMOUS));
  if (str_equal(sender,"#@[]"))
    strerr_die2x(99,INFO,MSG(ERR_BOUNCE));

  if (getconf(&headerremove,"headerkeep",0))
    headerremoveflag = 1;
  else
    getconf(&headerremove,"headerremove",1);
  constmap_init(&headerremovemap,headerremove.s,headerremove.len,0);

  if (!stralloc_copys(&mydtline,
       "Delivered-To: request processor for ")) die_nomem();
  if (!stralloc_cat(&mydtline,&outlocal)) die_nomem();
  if (!stralloc_cats(&mydtline,"@")) die_nomem();
  if (!stralloc_cat(&mydtline,&outhost)) die_nomem();
  if (!stralloc_cats(&mydtline,"\n")) die_nomem();

  flagnosubject = 1;
  if (action[0]) {	/* mainly to allow ezmlm-lists or ezmlm-which with */
    flagnosubject = 0;	/* a command address rather than a complete msg */
    command = action;
    if (str_start(action,"return"))		/* kill bounces */
      strerr_die2x(0,INFO,MSG(ERR_BOUNCE));
    pos = 1 + str_chr(action + 1,'-');
    if (action[pos]) {				/* start of target */
      action[pos] = '\0';
      userlocal = action + pos + 1;
      pos = str_rchr(userlocal,'=');		/* the "pseudo-@" */
      if (userlocal[pos]) {
	userlocal[pos] = '\0';
        userhost = userlocal + pos + 1;
      }
    }
  } else {
    for (;;) {			/* Get Subject: */
      if (gethdrln(&ssin,&line,&match,'\n') == -1)
        strerr_die2sys(111,FATAL,MSG(ERR_READ_INPUT));
      if (line.len <= 1)
        break;

          if (case_startb(line.s,line.len,"mailing-list:"))
            strerr_die2x(100,FATAL,MSG(ERR_MAILING_LIST));
          else if (case_startb(line.s,line.len,"Subject:")) {
            flaggotsub = 1;
	    concatHDR(line.s+8,line.len-8,&subject);
          } else if (case_startb(line.s,line.len,"content-type:")) {
	    pos = 13; last = line.len - 2;	/* not cont-line - ok */
            while (pos <= last &&
		(line.s[pos] == ' ' || line.s[pos] == '\t')) ++pos;
	    if (case_startb(line.s+pos,line.len - pos,"multipart/"))
	      flagmultipart = 1;
	  } else if (line.len == mydtline.len)
            if (!byte_diff(line.s,line.len,mydtline.s))
               strerr_die2x(100,FATAL,MSG(ERR_LOOPING));
      if (!match)
	break;
    }
    if (!cfname) {		 /* listserv@/majordomo@ ignore */
      char ch;
      if (!stralloc_0(&subject)) die_nomem();
      ch = *subject.s;		/* valid commands/list names start w letter */
      if ((ch <= 'z' && ch >= 'a') || (ch <= 'Z' && ch >= 'A') || ch == '_') {
        parseline(subject.s);
        flagnosubject = 0;
      }
    }
    if (cfname || flagnosubject) {
      for (;;) {					/* parse body */
        if (getln(&ssin,&line,&match,'\n') == -1)
        strerr_die2sys(111,FATAL,MSG(ERR_READ_INPUT));
        if (!match) break;
	if (line.len == 1 && flagmultipart != 2) continue;
		/* lazy MIME cludge assumes first '--...' is start border */
		/* which is virtually always true */
	if (flagmultipart == 1) {		/* skip to first border */
	  if (*line.s != '-' || line.s[1] != '-') continue;
	  flagmultipart = 2;
	  continue;
	} else if (flagmultipart == 2) {	/* skip content info */
	  if (line.len != 1) continue;
	  flagmultipart = 3;			/* may be part within part */
	  continue;				/* and blank line */
	} else if (flagmultipart == 3) {
	  if (*line.s == '-' && line.s[1] == '-') {
	    flagmultipart = 2;			/* part within part */
	    continue;
	  }
	}
        {
         char ch;
         ch = *line.s;
        if (line.len == 1 ||
          !((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')))
          continue;				/* skip if not letter pos 1 */
        }
			/* Here we have a body line with something */
        if (!stralloc_copy(&subject,&line)) die_nomem();	/* save it */
        subject.s[subject.len-1] = '\0';
        parseline(subject.s);
        break;
      }
    }
  }
	/* Do command substitution */
  if (!stralloc_copys(&cmds,cmdstring)) die_nomem();
  if (!stralloc_0(&cmds)) die_nomem();
  psz = cmds.s;
  while (*psz) {
    if (*psz == '\\') *psz = '\0';
    ++psz;
  }
  if (!constmap_init(&commandmap,cmds.s,cmds.len,0)) die_nomem();
  cmdidx = cmdxlate[constmap_index(&commandmap,command,str_len(command))];
  if (cmdidx == EZREQ_BAD) {	/* recognized, but not supported -> help */
    listlocal = 0;		/* needed 'cause arguments are who-knows-what */
    listhost = 0;
    userlocal = 0;
    userhost = 0;
    cmdidx = EZREQ_HELP;
  }
  if (cfname && !listlocal && !userlocal && cmdidx > 0)
    cmdidx = noargsxlate[cmdidx];	 /* some done differently if no args */

	/* =0 not found. This is treated as a list command! */
  if (cmdidx < 0 && !cfname) {
    cmdidx = EZREQ_HELP;
  }
  if (qmail_open(&qq) == -1)
    strerr_die2sys(111,FATAL,MSG(ERR_QMAIL_QUEUE));

  if (cmdidx >= 0) {
	/* Things handled elsewhere. We do want to handle a simple HELP */
	/* without arguments for e.g. majordomo@ from our own help file */

    if (!stralloc_copys(&from,sender)) die_nomem();
    if (!stralloc_0(&from)) die_nomem();
    if (!listlocal) {
      if (cfname)
        strerr_die1x(100,MSG(ERR_REQ_LISTNAME));
      else
       listlocal = listname.s;	/* This is at the -request address */
    }
	/* if !cfname listhost is made outhost. If cfname, listhost=outhost */
	/* is ok. listhost=0 => first match in config. Other listhost is ok */
	/* only if match is found. Otherwise it's set to outhost. */

    if (!cfname || (listhost && !case_diffs(listhost,hostname.s)))
      listhost = hostname.s;
    else {			 /* Check listhost against config file */
      pos = str_len(listlocal);
      fd = open_read(cfname);
      if (fd == -1)
        strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,cfname));
      substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));
      flagok = 0;			/* got listhost match */
      for (;;) {
        if (getln(&sstext,&line,&match,'\n') == -1)
          strerr_die2sys(111,FATAL,MSG1(ERR_READ,cfname));
        if (!match)
          break;
        if (line.len <= 1 || line.s[0] == '#')
          continue;
        if ((pos < line.len) && (line.s[pos] == '@') &&
		!byte_diff(line.s,pos,listlocal)) {
          last = byte_chr(line.s,line.len,':');
          if (!stralloc_copyb(&lhost,line.s+pos+1,last-pos-1)) die_nomem();
          if (!stralloc_0(&lhost)) die_nomem();
          if (listhost) {
            if (!case_diffs(listhost,lhost.s)) {
              flagok = 1;
              break;			/* host did match */
            } else
              continue;			/* host didn't match */
          } else {			/* none given - grab first */
            listhost = lhost.s;
            flagok = 1;
            break;
          }
        }
      }
      if (!flagok)
        listhost = hostname.s;
      close(fd);
    }
    if (!listhost)
      listhost = hostname.s;
    if (!userlocal) {
      if (!stralloc_copys(&usr,sender)) die_nomem();
      if (!stralloc_0(&usr)) die_nomem();
      userlocal = usr.s;
      userhost = usr.s + byte_rchr(usr.s,usr.len-1,'@');
      if (!*userhost)
        userhost = 0;
      else {
        *userhost = '\0';
        ++userhost;
      }
    }

    if (!stralloc_copys(&to,listlocal)) die_nomem();
    if (!stralloc_cats(&to,"-")) die_nomem();
    if (cmdidx) {			/* recognized - substitute */
      if (!stralloc_cats(&to,constmap_get(&commandmap,cmdidx)))
		 die_nomem();
    } else				/* not recognized - use as is */
      if (!stralloc_cats(&to,command)) die_nomem();

    if (!stralloc_cats(&to,"-")) die_nomem();
    if (!stralloc_cats(&to,userlocal)) die_nomem();
    if (userhost) {			/* doesn't exist for e.g. -get */
      if (!stralloc_cats(&to,"=")) die_nomem();
      if (!stralloc_cats(&to,userhost)) die_nomem();
    }
    if (!stralloc_cats(&to,"@")) die_nomem();
    if (!stralloc_cats(&to,listhost)) die_nomem();
    if (!stralloc_0(&to)) die_nomem();

    qmail_put(&qq,mydtline.s,mydtline.len);

    flaginheader = 1;
    flagbadfield = headerremoveflag;

    if (seek_begin(0) == -1)
      strerr_die2sys(111,FATAL,MSG(ERR_SEEK_INPUT));
    substdio_fdbuf(&ssin,read,0,inbuf,sizeof(inbuf));

    for (;;) {
      if (getln(&ssin,&line,&match,'\n') == -1)
        strerr_die2sys(111,FATAL,MSG(ERR_READ_INPUT));

      if (flaginheader && match) {
        if (line.len == 1)
          flaginheader = 0;
        if ((line.s[0] != ' ') && (line.s[0] != '\t')) {
          flagbadfield = headerremoveflag;
          if (constmap(&headerremovemap,line.s,byte_chr(line.s,line.len,':')))
	    flagbadfield = !headerremoveflag;
        }
      }
      if (!(flaginheader && flagbadfield))
        qmail_put(&qq,line.s,line.len);
      if (!match)
        break;
    }
  } else {				/* commands we deal with */
    cmdidx = - cmdidx;			/* now positive */
    if (cmdidx == EZREQ_WHICH) {	/* arg is user, not list */
      userlocal = listlocal; listlocal = 0;
      userhost = listhost; listhost = 0;
      initsub(0);
    }
    if (!stralloc_copy(&from,&outlocal)) die_nomem();
    if (!stralloc_cats(&from,"-return-@")) die_nomem();
    if (!stralloc_cat(&from,&outhost)) die_nomem();
    if (!stralloc_0(&from)) die_nomem();

    if (userlocal) {
      if (!stralloc_copys(&to,userlocal)) die_nomem();
      if (!stralloc_cats(&to,"@")) die_nomem();
      if (userhost) {
        if (!stralloc_cats(&to,userhost)) die_nomem();
       } else {
        if (!stralloc_cat(&to,&outhost)) die_nomem();
      }
    } else
      if (!stralloc_copys(&to,sender)) die_nomem();
    if (!stralloc_0(&to)) die_nomem();

					/* we don't want to be send to a list*/
    hdr_adds("Mailing-List: ezmlm-request");
    if (listid.len > 0)
      hdr_add2("List-ID: ",listid.s,listid.len);
    hdr_datemsgid(now());
    hdr_from((cmdidx == EZREQ_HELP) ? "-return-" : "-help");
    qmail_put(&qq,mydtline.s,mydtline.len);
    if (!quote2(&line,to.s)) die_nomem();
    hdr_add2("To: ",line.s,line.len);
    hdr_mime(flagcd ? CTYPE_MULTIPART : CTYPE_TEXT);
    hdr_subject(MSG(SUB_RESULTS));
    qmail_puts(&qq,"\n");
    hdr_ctboundary();
    copy(&qq,"text/top",flagcd);
   if (cmdidx == EZREQ_LISTS || cmdidx == EZREQ_WHICH) {
      switch (cmdidx) {
        case EZREQ_LISTS:
          code_qput("LISTS:",6);
          break;
        case EZREQ_WHICH:
          code_qput("WHICH (",7);
          code_qput(to.s,to.len - 1);
          code_qput("):\n\n",4);
          break;
        default: break;
      }
      fd = open_read(cfname);
      if (fd == -1)
        strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,cfname));
      substdio_fdbuf(&sstext,read,fd,textbuf,sizeof(textbuf));
      for (;;) {
        if (getln(&sstext,&line,&match,'\n') == -1)
          strerr_die2sys(111,FATAL,MSG1(ERR_READ,cfname));
        if (!match)
          break;
        if (line.len <= 1 || line.s[0] == '#')
          continue;
        if (!stralloc_0(&line)) die_nomem();
        pos = str_chr(line.s,':');
        if (!line.s[pos])
          break;
        line.s[pos] = '\0';
        ++pos;
        pos1 = pos + str_chr(line.s + pos,':');
        if (line.s[pos1]) {
          line.s[pos1] = '\0';
          ++pos1;
        } else
          pos1 = 0;

        switch (cmdidx) {
          case EZREQ_LISTS:
            code_qput("\n\n\t",3);
            code_qput(line.s,pos-1);
            code_qput("\n",1);
            if (pos1) {
              code_qput(line.s+pos1,line.len-2-pos1);
            }
            break;
          case EZREQ_WHICH:
	    // FIXME: this probably won't work for all cases
            if (issub(line.s+pos,to.s,0)) {
              code_qput(line.s,pos-1);
              code_qput("\n",1);
            }
	    closesub();		/* likely different dbs for different lists */
            break;
        }
      }
      code_qput("\n",1);
      close(fd);
    } else
      copy(&qq,"text/help",flagcd);

    copy(&qq,"text/bottom",flagcd);
    if (flagcd) {
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
    if (!quote2(&line,sender)) die_nomem();
    qmail_put(&qq,line.s,line.len);
    qmail_puts(&qq,">\n");
    if (seek_begin(0) == -1)
      strerr_die2sys(111,FATAL,MSG(ERR_SEEK_INPUT));
    if (qmail_copy(&qq,&ssin2,copylines) != 0)
      strerr_die2sys(111,FATAL,MSG(ERR_READ_INPUT));
    if (flagcd)
      hdr_boundary(1);
  }
  qmail_from(&qq,from.s);
  qmail_to(&qq,to.s);
  if (*(err = qmail_close(&qq)) != '\0')
      strerr_die4x(111,FATAL,MSG(ERR_TMP_QMAIL_QUEUE),": ",err + 1);

  strnum[fmt_ulong(strnum,qmail_qp(&qq))] = 0;
  strerr_die3x(99,INFO, "qp ",strnum);
}
