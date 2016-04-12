#include <unistd.h>
#include "stralloc.h"
#include "str.h"
#include "byte.h"
#include "case.h"
#include "readwrite.h"
#include "substdio.h"
#include "subfd.h"
#include "getln.h"
#include "strerr.h"
#include "messages.h"
#include "mime.h"
#include "die.h"

const char FATAL[] = "ezmlm-weed: fatal: ";
const char USAGE[] =
"ezmlm-weed: usage: ezmlm-weed";

static const char warn1[] = "    **********************************************";
static const char warn2[] = "    **      THIS IS A WARNING MESSAGE ONLY      **";
static const char warn3[] = "    **  YOU DO NOT NEED TO RESEND YOUR MESSAGE  **";
static const char warn4[] = "    **********************************************";

static void get(stralloc *sa)
{
  int match;
  if (gethdrln(subfdin,sa,&match,'\n') == -1)
    strerr_die2sys(111,FATAL,MSG(ERR_READ_INPUT));
  if (!match) _exit(0);
}

static stralloc line = {0};
static stralloc boundary = {0};

static int isboundary(void)
/* returns 1 if line.len contains the mime bondary, 0 otherwise */
{
    if (line.s[0] == '-' && line.s[1] == '-' && line.len >= boundary.len + 3)
      if (!byte_diff(line.s + 2,boundary.len,boundary.s))	/* boundary */
        return 1;
    return 0;
}

int main(void)
{
  stralloc line1 = {0};
  stralloc line2 = {0};
  stralloc line3 = {0};
  stralloc line4 = {0};
  stralloc line5 = {0};
  stralloc line6 = {0};
  stralloc line7 = {0};
  stralloc line8 = {0};
  stralloc dsnline = {0};

  int flagmds = 0;
  int flagsw = 0;
  int flagsr = 0;
  int flagas = 0;
  int flagbw = 0;
  int flagdsn = 0;

  unsigned int i,j;

  for (;;) {
    get(&line);
    if (line.len == 1) break;
    if (stralloc_starts(&line,"Subject: success notice"))
      _exit(99);
    if (stralloc_starts(&line,"Subject: deferral notice"))
      _exit(99);
    if (stralloc_starts(&line,"Precedence: bulk"))
      _exit(99);
    if (stralloc_starts(&line,"Precedence: junk"))
      _exit(99);
    if (stralloc_starts(&line,"Auto-Submitted:"))
      _exit(99);
    /* Mailing list signatures */
    if (stralloc_starts(&line,"Precedence: list"))
      _exit(99);
    if (stralloc_starts(&line,"Mailing-List:"))
      _exit(99);
    if (stralloc_starts(&line,"List-ID:"))
      _exit(99);
    if (stralloc_starts(&line,"X-Mailing-List:"))
      _exit(99);
    if (stralloc_starts(&line,"X-ML-Name:"))
      _exit(99);
    if (stralloc_starts(&line,"List-Help:"))
      _exit(99);
    if (stralloc_starts(&line,"List-Unsubscribe:"))
      _exit(99);
    if (stralloc_starts(&line,"List-Subscribe:"))
      _exit(99);
    if (stralloc_starts(&line,"List-Post:"))
      _exit(99);
    if (stralloc_starts(&line,"List-Owner:"))
      _exit(99);
    if (stralloc_starts(&line,"List-Archive:"))
      _exit(99);
/* for Novell Groupwise */
    if (stralloc_starts(&line,"Subject: Message status - delivered"))
      _exit(99);
    if (stralloc_starts(&line,"Subject: Message status - opened"))
      _exit(99);
    if (stralloc_starts(&line,"Subject: Out of Office AutoReply:"))
      _exit(99);
    /* Other autoresponders */
    if (stralloc_starts(&line,"X-Amazon-Auto-Reply:"))
      _exit(99);
    if (stralloc_starts(&line,"X-Mailer: KANA Response"))
      _exit(99);
    if (stralloc_starts(&line,"Thread-Topic: AutoResponse"))
      _exit(99);
    if (stralloc_starts(&line,"Subject: AutoResponse -"))
      _exit(99);
    
    if (stralloc_starts(&line,"From: Mail Delivery Subsystem <MAILER-DAEMON@"))
      flagmds = 1;
    if (stralloc_starts(&line,"Subject: Warning: could not send message"))
      flagsw = 1;
    if (stralloc_starts(&line,"Subject: Returned mail: warning: cannot send message"))
      flagsr = 1;
    if (stralloc_starts(&line,"Auto-Submitted: auto-generated (warning"))
      flagas = 1;
    if (case_startb(line.s,line.len,"Content-type: multipart/report")) {
      concatHDR(line.s,line.len,&dsnline);
    }
  }			/* end of header */

  if (dsnline.len > 0) {	/* always only one recipient/action */
    flagdsn = 0;	/* will be set for correct report type */
    for (i=0; i < dsnline.len; i += 1+byte_chr(dsnline.s+i,dsnline.len-i,';')) {
      while (dsnline.s[i] == ' ' || dsnline.s[i] == '\t')
	if (++i >= dsnline.len) break;
      if (case_startb(dsnline.s + i,dsnline.len - i,"report-type=")) {
	i += 12;
	while (dsnline.s[i] ==' ' || dsnline.s[i] =='\t' || dsnline.s[i] =='"')
	  if (++i >= dsnline.len) break;
	if (case_startb(dsnline.s + i,dsnline.len - i,"delivery-status"))
	  flagdsn = 1;
      } else if (case_startb(dsnline.s + i,dsnline.len - i,"boundary=")) {
	i += 9;
	while (dsnline.s[i] ==' ' || dsnline.s[i] =='\t')
	  if (++i >= dsnline.len) break;
	if (dsnline.s[i] == '"') {
	  if (++i >= dsnline.len) break;
	  j = i + byte_chr(dsnline.s + i,dsnline.len - i,'"');
	  if (j >= dsnline.len) break;
	} else {
	  j = i;
	  while (dsnline.s[j] !=' ' && dsnline.s[j] !='\t' &&
		dsnline.s[j] !=';')
	    if (++j >= dsnline.len) break;
	}				/* got boundary */
	stralloc_copyb(&boundary,dsnline.s+i,j-i);
      }
    }
  }
  if (flagdsn && boundary.len) {	/* parse DSN message */
    get(&line);			/* if bad format we exit(0) via get() */
    for (;;) {
      if (isboundary()) {
      if (line.len == boundary.len + 5 && line.s[line.len - 1] == '-'
		&& line.s[line.len - 2] == '-')
        _exit(99);			/* end: not failure report */
        get(&line);			/* Content-type */
        if (case_startb(line.s,line.len,"content-type:")) {
	  i = 13;
	  while (line.s[i] == ' ' || line.s[i] == '\t')
		if (++i >= line.len) break;
	  if (case_startb(line.s+i,line.len-i,"message/delivery-status")) {
	    for (;;) {
	      get(&line);
	      if (isboundary()) break;
	      if (case_startb(line.s,line.len,"action:")) {
	        i = 8;
	        while (line.s[i] == ' ' || line.s[i] == '\t')
		  if (++i >= line.len) break;
	        if (case_startb(line.s + i, line.len - i,"failed"))
		  _exit(0);	/* failure notice */
		else
		  _exit(99);	/* there shouldn't be more than 1 action */
	      }
            }
	  }
        }
      } else
	get(&line);
    }
  }

  get(&line1);
  get(&line2);
  get(&line3);
  get(&line4);
  get(&line5);
  get(&line6);
  get(&line7);
  get(&line8);

  if (stralloc_starts(&line1,"This is a MIME-encapsulated message"))
  if (stralloc_starts(&line3,"--"))
  if (stralloc_starts(&line5,warn1))
  if (stralloc_starts(&line6,warn2))
  if (stralloc_starts(&line7,warn3))
  if (stralloc_starts(&line8,warn4))
    flagbw = 1;

  if (stralloc_starts(&line1,warn1))
  if (stralloc_starts(&line2,warn2))
  if (stralloc_starts(&line3,warn3))
  if (stralloc_starts(&line4,warn4))
    flagbw = 1;

  if (flagmds && flagsw && flagas && flagbw) _exit(99);
  if (flagmds && flagsr && flagbw) _exit(99);

  _exit(0);
}
