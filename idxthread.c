/*$Id$*/

/* idxthread.c contains routines to from the ezmlm-idx subject index build */
/* a structure of unique subjects as well as a table of messages with      */
/* pointers to the subject. This leads to information on message threads   */
/* arranged chronologically within the thread, and with the threads        */
/* arranged chronologically by the first message within the range.         */
/* idx_mkthreads() will arrange the author list in a similar manner. This  */
/* saves some space, and takes a little extra time. It's needed when       */
/* generating an author index. */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "error.h"
#include "alloc.h"
#include "str.h"
#include "stralloc.h"
#include "strerr.h"
#include "lock.h"
#include "open.h"
#include "getln.h"
#include "scan.h"
#include "byte.h"
#include "idx.h"
#include "errtxt.h"
#include "substdio.h"
#include "fmt.h"
#include "readwrite.h"
#include "makehash.h"
#include "yyyymm.h"

#define DATENO 100
static stralloc line = {0};		/* primary input */
static stralloc authline = {0};		/* second line of primary input */
static stralloc dummyind = {0};

static substdio ssindex;
static char indexbuf[1024];

static char strnum[FMT_ULONG];

struct stat st;
			/* if no data, these may be the entire table, so */
			/* need to be static */
static subentry sdummy;
static authentry adummy;

int fdlock;

/* NOTE: These do NOT prevent double locking */
static void lockup(void)
{
  fdlock = open_append("lock");
  if (fdlock == -1)
    strerr_die2sys(111,FATAL,ERR_OPEN_LOCK);
  if (lock_ex(fdlock) == -1) {
    close(fdlock);
    strerr_die2sys(111,FATAL,ERR_OBTAIN_LOCK);
  }
}

static void unlock(void)
{
    close(fdlock);
}

static void newsub(subentry *psubt,const char *subject,unsigned int sublen,
		   unsigned long msg)
/* Initializes subentry pointed to by psubt, adds a '\0' to subject,    */
/* allocates space and copies in subject, and puts a pointer to it in   */
/* the entry. */
{
  const char *cpfrom;
  char *cpto;
  unsigned int cpno;

  psubt->higher = (subentry *) 0;
  psubt->lower = (subentry *) 0;
  psubt->firstmsg = msg;
  psubt->lastmsg = msg;
  psubt->msginthread = 1;
  if (!(psubt->sub = alloc ((sublen) * sizeof(char))))
    die_nomem();
  cpto = psubt->sub;
  cpno = sublen;
  cpfrom = subject;
  while (cpno--) *(cpto++) = *(cpfrom++);
  psubt->sublen = sublen;
}

static void newauth(authentry *pautht,		/* entry for current message */
		    const char *author,/* pointer to author string (not sz!) */
		    unsigned int authlen,	/* lenth of author */
		    unsigned long msg)		/* sz */
/* Allocates space for author of length authlen+1 adding a terminal '\0' */
/* and puts the pointer in pautht->auth. Analog to newsub().             */
{
  const char *cpfrom;
  char *cpto;
  unsigned int cpno;

  pautht->higher = (subentry *) 0;
  pautht->lower = (subentry *) 0;
  pautht->firstmsg = msg;
  if (!(pautht->auth = alloc ((authlen) * sizeof(char))))
    die_nomem();
  cpto = pautht->auth;
  cpno = authlen;
  cpfrom = author;
  while (cpno--) *(cpto++) = *(cpfrom++);
  pautht->authlen = authlen;
}

static void init_dummy(void)
{
  unsigned int i;

  if (!stralloc_ready(&dummyind,HASHLEN + 1)) die_nomem();
  for (i = 0; i< HASHLEN; i++)
    dummyind.s[i] = 'a';
  dummyind.len = HASHLEN;
  if (!stralloc_append(&dummyind," ")) die_nomem();
}

void idx_mkthreads(msgentry **pmsgtable,	/* table of message<->subject */
		   subentry **psubtable,	/* subject no, len, str char * */
		   authentry **pauthtable,	/* author no, len, str char* */
		   dateentry **pdatetable,	/* message per date */
		   unsigned long msg_from,	/* first message in range */
		   unsigned long msg_to,	/* last message in range */
		   unsigned long msg_latest,	/* latest message in archive (for locking) */
		   int locked)			/* if already locked */

/* Threads messages msg_from -> msg_to into pmsgtable & psubtable. When  */
/* reading the latest index file (containing msg_latest) it locks the    */
/* directory, unless it is already locked (as in digest creation).       */
/* msgtable has the subject number 1.. (0 if there is no subject match,  */
/* which should happen only if the subject index is corrupt.)            */

/* 19971107 Changed to deal with index files that are missing, or have   */
/* missing entries, not necessarily reflecting missing archive files.    */
/* This all to make ezmlm-get more robust to get maximal info out of     */
/* corrupted archives.                                                   */
{
  unsigned long idxlatest;	/* need to lock for this (last) index file */
  unsigned long msg;		/* current msg number */
  unsigned long endmsg;		/* max msg in this idx file */
  unsigned long tmpmsg;		/* index entry's msg number */
  unsigned long idx;		/* current index file no */
  unsigned long idxto;		/* index containing end of range */
  unsigned long ulmrange;	/* total # of messages in range */
  char *subject;		/* subject on line */
  unsigned int sublen;		/* length of subject */
  char *auth;
  unsigned int authlen;
  unsigned int pos,posa;
  unsigned long submax;		/* max subject num in subtable */
  subentry *psubnext;		/* points to next entry in subtable */
  subentry *psubt;		/* points to entry in subtable */
  authentry *pauthnext;		/* points to next entry in authtable */
  authentry *pautht;		/* points to entry in authtable */
  int fd;			/* index file handle */
  int flagmissingindex;		/* current index file is missing */
  int flagauth;			/* read index entry has author info */
  int hasauth;			/* current msg's entry has author info */
  msgentry *pmsgt;
  int res;
  int match;
  unsigned int datepos,datemax;
  unsigned int datetablesize,datetableunit;
  unsigned int lastdate = 0;
  unsigned int thisdate;
  msgentry *x, *y;

				/* a few unnecessary sanity checks */
  if (msg_to > msg_latest)
    msg_to = msg_latest;
  if (msg_to < msg_from)
    strerr_die2x(100,FATAL,"Program error: bad range in idx_mkthreads");
  ulmrange = msg_to - msg_from + 1;
  if (!(*pmsgtable = (msgentry *) alloc(ulmrange * sizeof(msgentry))))
        die_nomem();
  y = *pmsgtable;
  x = y + ulmrange;		/* clear */
  while (--x >= y) {
    x->subnum = 0;
    x->authnum = 0;
    x->date = 0;
  }
				/* max entries - acceptable waste for now */
  if (!(*psubtable = (subentry *) alloc((ulmrange+1) * sizeof(subentry))))
        die_nomem();

  if (!(*pauthtable = (authentry *) alloc((ulmrange+1) * sizeof(authentry))))
        die_nomem();
  datetableunit = DATENO * sizeof(dateentry);
  datetablesize = datetableunit;
  if (!(*pdatetable = (dateentry *) alloc(datetablesize)))
        die_nomem();
  datepos = 0;
  datemax = DATENO - 2;		/* entry 0 and end marker */
  lastdate = 0;

  idxlatest = msg_latest / 100;
  idxto = msg_to / 100;
  submax = 0;
  psubnext = *psubtable;	/* dummy node to get tree going. Basically, */
  psubt = &sdummy;		/* assure that subject > psubt-sub and that */
  init_dummy();			/* below ok unless HASHLEN > 40 */
  psubt->sub = (char*)"                                       ";
  psubt->sublen = 40;		/* there is something to hold psubt->higher */
  psubt->higher = (subentry *) 0;
  psubt->lower = (subentry *) 0;
  pauthnext = *pauthtable;
  pautht = &adummy;
  pautht->auth = psubt->sub;
  pautht->authlen = psubt->sublen;
  pautht->higher = (authentry *) 0;
  pautht->lower = (authentry *) 0;
  for (idx = msg_from / 100; idx <= idxto; idx++) {
				/* make index file name */
    if (!stralloc_copys(&line,"archive/")) die_nomem();
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,idx))) die_nomem();
    if (!stralloc_cats(&line,"/index")) die_nomem();
    if (!stralloc_0(&line)) die_nomem();
    if (!locked && idx == idxlatest)
      lockup();
    flagmissingindex = 0;
    fd = open_read(line.s);
    if (fd == -1) {
      if (errno == error_noent) {	/* this means the index is not here */
					/* but the lists is supposedly indexed*/
        flagmissingindex = 1;
      } else
        strerr_die4sys(111,FATAL,ERR_OPEN,line.s,": ");
    } else
      substdio_fdbuf(&ssindex,read,fd,indexbuf,sizeof(indexbuf));

    msg = 100L * idx;			/* current msg# */
    endmsg = msg + 99L;			/* max msg in this index */
    if (!msg) msg = 1L;			/* for start to make msg > tmpmsg */
    tmpmsg = 0L;			/* msg number of read index line */
    if (endmsg > msg_to)		/* skip non-asked for subjects */
      endmsg = msg_to;
    for (; msg <= endmsg; msg++) {
      if (!flagmissingindex && (msg > tmpmsg)) {
        flagauth = 0;
        if (getln(&ssindex,&line,&match,'\n') == -1)
          strerr_die3sys(111,FATAL,ERR_READ,"index: ");
        if (!match)
          flagmissingindex = 1;
        else {
          pos = scan_ulong(line.s,&tmpmsg);
          if (line.s[pos++] == ':') {
            if (getln(&ssindex,&authline,&match,'\n') == -1)
              strerr_die3sys(111,FATAL,ERR_READ,"index: ");
            if (!match)
              flagmissingindex = 1;
            else {
              flagauth = 1;
	    }
            pos++;
          }
        }
      }
      if (msg < msg_from)	/* Nothing before start of range */
        continue;
      if (msg == tmpmsg) {
        subject = line.s + pos;
        sublen = line.len - pos;
	if (sublen <= HASHLEN)
	  strerr_die2x(100,FATAL,ERR_BAD_INDEX);
        hasauth = flagauth;
      } else {
        subject = dummyind.s;
        sublen = dummyind.len;
        hasauth = 0;
      }
      for(;;) {		/* search among already known subjects */
        res = str_diffn(psubt->sub,subject,HASHLEN);
        if (res < 0) {
          if (psubt->higher)
            psubt = psubt->higher;
          else {
            newsub(psubnext,subject,sublen,msg);
            psubt->higher = psubnext;
            psubt = psubnext;
            psubnext++;
            break;
          }
        } else if (res > 0) {
          if (psubt->lower)
            psubt = psubt->lower;
          else {
            newsub(psubnext,subject,sublen,msg);
            psubt->lower = psubnext;
            psubt = psubnext;
            psubnext++;
            break;
          }
        } else {
          psubt->lastmsg = msg;
	  (psubt->msginthread)++;	/* one more message in thread */
	  break;
        }
      }
				/* first subnum =1 (=0 is empty for thread) */
      pmsgt = *pmsgtable + msg - msg_from;
      pmsgt->subnum = (unsigned int) (psubt - *psubtable + 1);
      pmsgt->date = lastdate;
      if (hasauth) {
	pos = 0;
	while (authline.s[pos] && authline.s[pos] != ' ') pos++;
	if (authline.s[++pos]) {
	  thisdate = date2yyyymm(authline.s + pos);
	  if (thisdate) pmsgt->date = thisdate;
	  if (pmsgt->date > lastdate) {
	    lastdate = pmsgt->date;
	    if (datepos >= datemax) {		/* more space */
	      datemax += DATENO;
	      if (!alloc_re((void**)pdatetable,
			    datetablesize,datetablesize+datetableunit))
		die_nomem();
	    }
	    (*pdatetable)[datepos].msg = msg;	/* first msg this mo */
	    (*pdatetable)[datepos].date = lastdate;
	    datepos++;
	  }
	  posa = byte_chr(authline.s,authline.len,';');
	  if (authline.len > posa + HASHLEN + 1 && authline.s[pos+1] != ' ') {
					/* old: "; auth", new: ";hash auth" */
	    auth = authline.s + posa + 1;
	    authlen = authline.len - posa - 1;
	  } else {
	    auth = dummyind.s;
	    authlen = dummyind.len;
	  }
	}
			/* allright! Same procedure, but for author */
        for (;;) {	/* search among already known authors */
          res = str_diffn(pautht->auth,auth,HASHLEN);
          if (res < 0) {
            if (pautht->higher)
              pautht = pautht->higher;
            else {
              newauth(pauthnext,auth,authlen,msg);
              pautht->higher = pauthnext;
              pautht = pauthnext;
              pauthnext++;
              break;
            }
          } else if (res > 0) {
            if (pautht->lower)
              pautht = pautht->lower;
            else {
              newauth(pauthnext,auth,authlen,msg);
              pautht->lower = pauthnext;
              pautht = pauthnext;
              pauthnext++;
              break;
            }
          } else {
            break;
          }
        }			/* link from message to this author */
        pmsgt->authnum = (unsigned int) (pautht - *pauthtable + 1);
        pautht = *pauthtable;
      }

      psubt = *psubtable;	/* setup psubt. Done here rather than before */
				/* the for loop, so that we can start off    */
				/* the dummy node. */
    }
    if (fd != -1)
      close(fd);
    if (!locked && idx == idxlatest)
      unlock();			/* 'locked' refers to locked before calling */
  }
  psubnext->sub = (char *) 0;		/* end of table marker */
  pauthnext->auth = (char *) 0;		/* end of table marker */
  (*pdatetable)[datepos].msg = msg_to + 1;
  (*pdatetable)[datepos].date = lastdate + 1;
}


void idx_mkthread(msgentry **pmsgtable,		/* pointer to table of message<->subject */
		  subentry **psubtable,		/* ptr to tbl of subject no, len, str char * */
		  authentry **pauthtable,
		  unsigned long msg_from,	/* first message in range */
		  unsigned long msg_to,		/* last message in range */
		  unsigned long msg_master,	/* master message for single thread, else 0*/
		  unsigned long msg_latest,	/* latest message in archive (for locking) */
		  int locked)			/* if already locked */

/* Works like idx_mkthreads, except that it finds the subject for message   */
/* msg_master, then identifies messages in the range that have the same     */
/* subject. msgtable entries with subject 0 do not match, with '1' do match.*/
{
  unsigned long idxlatest;	/* need to lock for this (last) index file */
  unsigned long idxto;		/* index for last msg in range */
  unsigned long idx;		/* current index file no */
  unsigned long msg;		/* index entry's msg number */
  unsigned long ulmrange;	/* total # of messages in range */
  subentry *psubt;		/* points to last entry in subtable */
  int ffound;			/* msg subject was found in subtable */
  int flagauth;			/* there is author info */
  int firstfound = 1;		/* = 1 until first message in thread found */
  int res;			/* comparison result */
  char *auth;
  unsigned int authlen;
  authentry *pauthnext;		/* points to next entry in authtable */
  authentry *pautht;		/* points to entry in authtable */
  unsigned int pos;
  int fd;			/* index file handle */
  int match;
  msgentry *pmsgt;
  msgentry *x,*y;

  if ((ulmrange = msg_to - msg_from +1) <= 0)
    strerr_die2x(100,FATAL,"Program error: bad range in idx_mkthreads");
  if (!(*pmsgtable = (msgentry *) alloc(ulmrange * sizeof(msgentry))))
         die_nomem();
  y = *pmsgtable;
  x = y + ulmrange;
  while (--x >= y) {
    x->subnum = 0;
    x->authnum = 0;
    x->date = 0;
  }

  if (!(*psubtable = (subentry *) alloc(2 * sizeof(subentry))))
          die_nomem();

  if (!(*pauthtable = (authentry *) alloc((ulmrange + 1) * sizeof(authentry))))
          die_nomem();

  pauthnext = *pauthtable;
  pautht = &adummy;
  init_dummy();
  pautht->auth = (char*)"                     ";
  pautht->authlen = 21;
  pautht->higher = (authentry *) 0;
  pautht->lower = (authentry *) 0;
  idxlatest = msg_latest / 100;
  idxto = msg_to / 100;
  idx = msg_master / 100;	/* index for master subject */

				/* Get master subject */
  if (!stralloc_copys(&line,"archive/")) die_nomem();
  if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,idx))) die_nomem();
  if (!stralloc_cats(&line,"/index")) die_nomem();
  if (!stralloc_0(&line)) die_nomem();
  ffound = 0;
  if (!locked && idx == idxlatest)
    lockup();
  fd = open_read(line.s);
  psubt = *psubtable;
  if (fd == -1) {
    if (errno != error_noent)
      strerr_die4sys(111,FATAL,ERR_OPEN,line.s,": ");
    else
      strerr_die2x(111,FATAL,ERR_NOINDEX);	/* temp - admin can fix! */
  } else {
    substdio_fdbuf(&ssindex,read,fd,indexbuf,sizeof(indexbuf));
    for(;;) {
      if (getln(&ssindex,&line,&match,'\n') == -1)
          strerr_die3sys(111,FATAL,ERR_OPEN,"index: ");
      if (!match)
        break;
      pos=scan_ulong(line.s,&msg);
      if (line.s[pos++] == ':') {	 /* marker for author info */
        pos++;
        flagauth = 1;
      } else
        flagauth = 0;
      if (msg == msg_master) {
        newsub(psubt,line.s+pos,line.len-pos,msg);
					/* need to update msg later! */
        ffound = 1;
        break;
      }
      if (flagauth) {			/* skip author line */
        if (getln(&ssindex,&line,&match,'\n') == -1)
          strerr_die3sys(111,FATAL,ERR_OPEN,"index: ");
      if (!match)
        break;
      }
    }
    close(fd);
  }
  if (!locked && idx == idxlatest)
    unlock();
  if (!ffound)
      strerr_die2x(100,FATAL,ERR_NOINDEX);
  for (idx = msg_from / 100; idx <= idxto; idx++) {
		/* make index file name */
    if (!stralloc_copys(&line,"archive/")) die_nomem();
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,idx))) die_nomem();
    if (!stralloc_cats(&line,"/index")) die_nomem();
    if (!stralloc_0(&line)) die_nomem();
    if (!locked && idx == idxlatest)
      lockup();
    fd = open_read(line.s);
    if (fd == -1) {
      if (errno != error_noent)
        strerr_die4sys(111,FATAL,ERR_OPEN,line.s,": ");
    } else {
      substdio_fdbuf(&ssindex,read,fd,indexbuf,sizeof(indexbuf));
      for(;;) {
        if (getln(&ssindex,&line,&match,'\n') == -1)
          strerr_die3sys(111,FATAL,ERR_READ,"index: ");
        if (!match)
          break;
        pos=scan_ulong(line.s,&msg);
        if (line.s[pos++] == ':') {
          pos++;
          flagauth = 1;
          if (getln(&ssindex,&authline,&match,'\n') == -1)
            strerr_die3sys(111,FATAL,ERR_READ,"index: ");
          if (!match)
            break;
        } else
          flagauth = 0;
        if (msg < msg_from)	/* Nothing before start of range */
          continue;
        if (msg > msg_to)	/* Don't do anything after range */
          break;
        if (!str_diffn(psubt->sub,line.s+pos,HASHLEN)) {
          pmsgt = *pmsgtable + msg - msg_from;
          if (firstfound) {	/* update to first message with this subj */
            psubt->firstmsg = msg;
            firstfound = 0;
          }
	  psubt->lastmsg = msg;
          pmsgt->subnum = 1;
          if (flagauth) {
	    if (*authline.s)
	      pmsgt->date = date2yyyymm(authline.s + 1);
            pos = byte_chr(authline.s,authline.len,';');
	    if (authline.len > pos + HASHLEN + 1 && authline.s[pos+1] != ' ') {
					/* old: "; auth", new: ";hash auth" */
	      auth = authline.s + pos + 1;
	      authlen = authline.len - pos - 1;
	    } else {
	      auth = dummyind.s;
	      authlen = dummyind.len;
	    }
	    for (;;) {		/* search among already known authors */
	      res = str_diffn(pautht->auth,auth,HASHLEN);
	      if (res < 0) {
	        if (pautht->higher)
	          pautht = pautht->higher;
	        else {
	          newauth(pauthnext,auth,authlen,msg);
	          pautht->higher = pauthnext;
	          pautht = pauthnext;
	          pauthnext++;
	          break;
	        }
	      } else if (res > 0) {
	        if (pautht->lower)
	          pautht = pautht->lower;
	        else {
	          newauth(pauthnext,auth,authlen,msg);
	          pautht->lower = pauthnext;
	          pautht = pauthnext;
	          pauthnext++;
	          break;
	        }
	      } else {
	        break;
	      }
	    }			/* link from message to this author */
	    pmsgt->authnum = (unsigned int) (pautht - *pauthtable + 1);
            pautht = *pauthtable;
	  }

        }
      }
      close(fd);
    }
    if (!locked && idx == idxlatest)
      unlock();
  }
  ++psubt;
  psubt->sub = (char *) 0;	/* end of table marker */
  pauthnext->auth = (char *) 0;	/* end of table marker */
}

void idx_mklist(msgentry **pmsgtable,	/* pointer to table of message<->subject */
		subentry **psubtable,	/* ptr to tbl of subject no, len, str char * */
		authentry **pauthtable,
		unsigned long msg_from,		/* first message in range */
		unsigned long msg_to)		/* last message in range */
/* Like mkthreads, except that it works without a subject index. The result */
/* is just a dummy subject and a sequential list of messages. This to allow */
/* use of the same routines when creating digest from lists that have no    */
/* subject index (for whatever reason). */
{
  unsigned long ulmrange;
  msgentry *x,*y;
  subentry *psubt;
  authentry *pautht;

  if ((ulmrange = msg_to - msg_from +1) <= 0)
    strerr_die2x(111,FATAL,"bad range in idx_mkthreads :");

  if (!(*pmsgtable = (msgentry *) alloc(ulmrange * sizeof(msgentry))))
         die_nomem();

  y = *pmsgtable;
  x = y + ulmrange;
  while (--x >= y) {
    x->subnum = 1;
    x->authnum = 0;
    x->date = 0;
  }

  if (!(*psubtable = (subentry *) alloc(2 * sizeof(subentry))))
          die_nomem();
  psubt = *psubtable;
  newsub(psubt,dummyind.s,dummyind.len,msg_from);
  psubt->lastmsg = msg_to;
  ++psubt;
  psubt->sub = (char *) 0;
  if (!(*pauthtable = (authentry *) alloc(sizeof(authentry))))
          die_nomem();		/* nodata. Avoid dangling ptr. */
  pautht = *pauthtable;
  pautht->auth = 0;		/* tells app that there are no author data */
  pautht->higher = (authentry *) 0;
  pautht->lower = (authentry *) 0;
}

void idx_destroythread(msgentry *msgtable,
		       subentry *subtable,
		       authentry *authtable)
/* Frees space allocated by idxthread routines. This is needed only if */
/* one does several threadings in one program run. Otherwise, exit()   */
/* should free all allocated memory, which will be faster. */
{
  subentry *psubt;
  authentry *pautht;

  psubt = subtable;		/* free subjects */
  while(psubt->sub) {
    alloc_free(psubt->sub);
    psubt++;
  }

  pautht = authtable;		/* free authors */
  while(pautht->auth) {
    alloc_free(pautht->auth);
    pautht++;
  }

  alloc_free(subtable);		/* free subtable */
  alloc_free(authtable);	/* free authtable */
  alloc_free(msgtable);		/* free msgtable */
  subtable = (subentry *) 0;	/* kill pointers */
  authtable = (authentry *) 0;
  msgtable = (msgentry *) 0;
}
