/*$Id$*/

#include <unistd.h>
#include "substdio.h"
#include "readwrite.h"
#include "wait.h"
#include "env.h"
#include "str.h"
#include "exit.h"
#include "fork.h"
#include "fd.h"
#include "qmail.h"
#include "auto_qmail.h"
#include "alloc.h"
#include "stralloc.h"
#include "idx.h"

static char *binqqargs[2] = { PROG_QMAIL_QUEUE, 0 } ;

int qmail_open(struct qmail *qq, const stralloc *sa)
{
  int pim[2];
  int pie[2];
  unsigned i,j;
  char *cp;
  char **cpp;

  qq->msgbytes = 0L;
  if (pipe(pim) == -1) return -1;
  if (pipe(pie) == -1) { close(pim[0]); close(pim[1]); return -1; }

  switch(qq->pid = vfork()) {
    case -1:
      close(pim[0]); close(pim[1]);
      close(pie[0]); close(pie[1]);
      return -1;
    case 0:
      close(pim[1]);
      close(pie[1]);
      if (fd_move(0,pim[0]) == -1) _exit(120);
      if (fd_move(1,pie[0]) == -1) _exit(120);
      if (chdir(auto_qmail) == -1) _exit(61);
      if ((cp = env_get("QMAILQUEUE")) != 0)
	binqqargs[0] = cp;
      else if (sa && sa->len) {		/* count args */
	j = 2;				/* empty sa - qmqpc c control args */
	for (i = 0; i < sa->len; i++) {
	  if (sa->s[i] == '\0') j++;
	}				/* make space */
	if (!(cpp = (char **) alloc(j * sizeof (char *)))) _exit(51);
	cpp[0] = PROG_QMAIL_QMQPC;
	cp = sa->s;
	j = 1;
	for (i = 0; i < sa->len; i++) {
	  if (sa->s[i]) continue;
	  cpp[j++] = cp;
	  cp = sa->s + i + 1;
	}
	cpp[j] = (char *) 0;
	execv(*cpp,cpp);
	_exit(120);
      }
      execv(*binqqargs,binqqargs);
      _exit(120);
  }

  qq->fdm = pim[1]; close(pim[0]);
  qq->fde = pie[1]; close(pie[0]);
  substdio_fdbuf(&qq->ss,write,qq->fdm,qq->buf,sizeof(qq->buf));
  qq->flagerr = 0;
  return 0;
}

unsigned long qmail_qp(struct qmail *qq)
{
  return qq->pid;
}

void qmail_fail(struct qmail *qq)
{
  qq->flagerr = 1;
}

void qmail_put(struct qmail *qq, const char *s, int len)
{
  if (!qq->flagerr) if (substdio_put(&qq->ss,s,len) == -1) qq->flagerr = 1;
  qq->msgbytes += len;
}

void qmail_puts(struct qmail *qq, const char *s)
{
  register int len;
  if (!qq->flagerr) {
    len = str_len(s);
    if (substdio_put(&qq->ss,s,len) == -1) qq->flagerr = 1;
  }
  qq->msgbytes += len;
}

void qmail_from(struct qmail *qq, const char *s)
{
  if (substdio_flush(&qq->ss) == -1) qq->flagerr = 1;
  close(qq->fdm);
  substdio_fdbuf(&qq->ss,write,qq->fde,qq->buf,sizeof(qq->buf));
  qmail_put(qq,"F",1);
  qmail_puts(qq,s);
  qmail_put(qq,"",1);
}

void qmail_to(struct qmail *qq, const char *s)
{
  qmail_put(qq,"T",1);
  qmail_puts(qq,s);
  qmail_put(qq,"",1);
}

const char *qmail_close(struct qmail *qq)
{
  int wstat;
  int exitcode;

  qmail_put(qq,"",1);
  if (!qq->flagerr) if (substdio_flush(&qq->ss) == -1) qq->flagerr = 1;
  close(qq->fde);

  if (wait_pid(&wstat,qq->pid) != qq->pid)
    return "Zqq waitpid surprise (#4.3.0)";
  if (wait_crashed(wstat))
    return "Zqq crashed (#4.3.0)";
  exitcode = wait_exitcode(wstat);

  switch(exitcode) {
    case 115: /* compatibility */
    case 11: return "Denvelope address too long for qq (#5.1.3)";
    case 31: return "Dmail server permanently rejected message (#5.3.0)";
    case 51: return "Zqq out of memory (#4.3.0)";
    case 52: return "Zqq timeout (#4.3.0)";
    case 53: return "Zqq write error or disk full (#4.3.0)";
    case 0: if (!qq->flagerr) return ""; /* fall through */
    case 54: return "Zqq read error (#4.3.0)";
    case 55: return "Zqq unable to read configuration (#4.3.0)";
    case 56: return "Zqq trouble making network connection (#4.3.0)";
    case 61: return "Zqq trouble in home directory (#4.3.0)";
    case 63:
    case 64:
    case 65:
    case 66:
    case 62: return "Zqq trouble creating files in queue (#4.3.0)";
    case 71: return "Zmail server temporarily rejected message (#4.3.0)";
    case 72: return "Zconnection to mail server timed out (#4.4.1)";
    case 73: return "Zconnection to mail server rejected (#4.4.1)";
    case 74: return "Zcommunication with mail server failed (#4.4.2)";
    case 91: /* fall through */
    case 81: return "Zqq internal bug (#4.3.0)";
    case 120: return "Zunable to exec qq (#4.3.0)";
    default:
      if ((exitcode >= 11) && (exitcode <= 40))
	return "Dqq permanent problem (#5.3.0)";
      return "Zqq temporary problem (#4.3.0)";
  }
}
