/*$Id$*/
#ifndef IDX_H
#define IDX_H

/* Range for '-thread' to protect large archives. A '-thread' search */
/* will start at most THREAD_BEFORE messages before the action argument */
/* and go to at most THREAD_AFTER messages after the action argument. */
#define THREAD_BEFORE 2000
#define THREAD_AFTER 2000

/* Maximum number of messages returned by get */
/* You also have to update /text/bottom in ezmlmrc if you change this */
#define MAXGET 100

/* Number of messages before latest digest to return for list-get.99999_x */
/* This is still subject to the MAXGET restriction */
#define HISTGET 30

/* Maximum subject index entries returned by index */
/* Must be multiple of 100 */
/* You also have to update /text/bottom in ezmlmrc if you change this */
#define MAXINDEX 2000

/* Max dir/text file size allowed by -edit */
#define MAXEDIT 10240

/* Timeout in seconds before a bounce warning is sent. Default is  */
/* 1000000, i.e. 11.57 days. Setting it lower reduces the number of */
/* messages in the bouce dir, but makes it more likely that an address */
/* is unsubscribed due to a temporary error. This compile-time default */
/* should rarely need changing, as it can be overridden with the ezmlm-warn */
/* -t switch */
#define BOUNCE_TIMEOUT 1000000L

/* ezmlm-limit defaults. Convert to moderation or defer if more than  */
/* LIMMSG messages arrive within LIMSECS */
#define LIMMSG 30L
#define LIMSECS 3600L

/* Command names and alternative command names */
/* all alternates must be defined! */
/* The language-specific blocks need to undefine and redefine commands */

#define ALT_LIST "list"
#define ALT_LISTN "listn"
#define ALT_EDIT "edit"
#define ALT_FAQ "faq"
#define ALT_GET "get"
#define ALT_HELP "help"
#define ALT_INDEX "index"
#define ALT_INFO "info"
#define ALT_LOG "log"
#define ALT_REQUEST "request"
#define ALT_SUBSCRIBE "subscribe"
#define ALT_THREAD "thread"
#define ALT_UNSUBSCRIBE "unsubscribe"
#define ALT_QUERY "query"

/* to get alternative command names, you need to undefine and redefine */
/* them. Do this within a language block and send it to the author for */
/* inclusing in future versions. If it's here already, just uncomment the */
/* define for the appropriate language. */
/* #define LANG_FR 1 */

/* French Version */
#ifdef LANG_FR
#undef ALT_SUBSCRIBE
#define ALT_SUBSCRIBE "-inscription"
#undef ALT_UNSUBSCRIBE
#define ALT_UNSUBSCRIBE "-desinscription"
#endif
/* end French Version */

/* Text that is used in the outgoing messages (there is some other text, but */
/* it needs to stay constant in order to comply with rfc1153 */

/* Topics (messages nnn through mmm):\n */
#define TXT_TOP_TOPICS "Topics"
#define TXT_TOP_MESSAGES " (messages "
#define TXT_TOP_THROUGH " through "
#define TXT_TOP_LAST "):\n"

/* in digest */
#define TXT_ADMINISTRIVIA "\nAdministrivia:\n\n"
#define TXT_SUPPRESSED "\n<suppressed>\n\n"

/* for the message author line: 000 by */
/* keep this short! */
#define TXT_BY " by: "

/* Since this is now run-time configurable, we'll go with the lowest */
/* common denominator (per rfc2046). -> ISO-8859-1 if you don't like that */
#define TXT_DEF_CHARSET "us-ascii"

/* should start with 20 'a' [in place of hash] */
#define TXT_NOINDEX "aaaaaaaaaaaaaaaaaaaa <- subject index not available for message(s) ->\n"

/* When copy of the message is suppressed (is this really used?)*/
#define TXT_SUPPRESSED "\n<suppressed>\n\n"

/* Subject: MODERATE for local@host */
#define TXT_MODERATE "MODERATE for "

/* Subject: Confirm post to local@host */
#define TXT_CONFIRM_POST "Confirm post to "

/* Subject: Returned post for local@host */
/* (used both for rejected and timed-out posts) */
#define TXT_RETURNED_POST "Returned post for "

/* Subject: CONFIRM subscribe to | unsubscribe from */
#define TXT_USRCONFIRM "confirm "
#define TXT_MODCONFIRM "CONFIRM "
#define TXT_SUBSCRIBE_TO "subscribe to "
#define TXT_UNSUBSCRIBE_FROM "unsubscribe from "

/* Subject: WELCOME to */
#define TXT_WELCOME "Subject: WELCOME to "

/* Subject: GOODBYE from */
#define TXT_GOODBYE "Subject: GOODBYE from "

/* Subject: ezmlm response\n */
#define TXT_EZMLM_RESPONSE "Subject: ezmlm response\n"

/* Subject: majordomo results\n\n [where "majordomo" is outlocal] */
#define TXT_RESULTS " results\n\n"

/* Subject: Edit file xxx for list@host */
#define TXT_EDIT_RESPONSE "Subject: EDIT "
#define TXT_EDIT_FOR " for "

/* Subject: Editable text files\n */
#define TXT_EDIT_LIST "Subject: List of editable text files\n"

/* markers for ezmlm-manage text file edit */
/* MUST start with '%' */
#define TXT_EDIT_START "%%% START OF TEXT FILE"
#define TXT_EDIT_END "%%% END OF TEXT FILE"

#define TXT_EDIT_SUCCESS "Subject: Success editing "

/* Text for '-list' command */
#define TXT_LISTMEMBERS "\nSubscribers to this list are:\n\n"

/* Output formats - letter used to override default */
#define FORMATS "mrvnx"
#define MIME 'm'
#define RFC1153 'r'
/* ---------------- virgin = MIME without header processing */
#define VIRGIN 'v'
/* NATIVE 'n' = VIRGIN without threading */
#define NATIVE 'n'
/* MIXED => multipart/mixed MIME instead of multipart/digest. Needed to bypass*/
/* pine bug when content-transfer-encoding is used (pine fails to show the */
/* initial encoded text/plain part of mulpart/digest, but not of ../mixed) */
#define MIXED 'x'
/* default output format. */
#define DEFAULT_FORMAT MIME

/* Use MIME enclosure for message to moderate by default (1) or not (0) */
/* ezmlm-store switches -m/-M override */
#define MOD_MIME 1

/* Used to add "filname=listname.msgno" to digest part content-type line.
   This confuses the heck out of Outlook Express 5.0. To circumvent this
   bug the addition has been removed. Uncomment the next line to get it
   anyway. */
/* #define DIGEST_PART_FILENAME */

/* Mode of messages in archive. For ezmlm-0.53 this is 0744, but for  */
/* "secret" lists it may make more sense to make it 0700.             */
#define MODE_ARCHIVE 0744

/* ezmlm-get actions  (ACTION_GET also for -get in ezmlm-manage) */
#define ACTION_GET "get"
#define ACTION_INDEX "index"
#define ACTION_THREAD "thread"

/* ezmlm-request actions */
#define ACTION_REQUEST "request"

/* actions for post acceptance/rejection */
#define ACTION_ACCEPT "accept-"
#define ACTION_REJECT "reject-"

/* actions for post confirmation/discard */
#define ACTION_CONFIRM "confirm-"
#define ACTION_DISCARD "discard-"
 
/* ezmlm-manage actions */
#define ACTION_LIST "list"
#define ACTION_LISTN "listn"
#define ACTION_HELP "help"
#define ACTION_INFO "info"
#define ACTION_FAQ "faq"
#define ACTION_LOG "log"
#define ACTION_SUBSCRIBE "subscribe"
#define ACTION_UNSUBSCRIBE "unsubscribe"
#define ACTION_QUERY "query"
#define ACTION_EDIT "edit"
/* if you change this, you MUST ADJUST LENGTH_ED as well! */
#define ACTION_ED "ed."
#define LENGTH_ED 3

/* ACTION_XC has to be a string "-xc." where x is any letter. All commands */
/* should have different letters. They no longer have to match the first    */
/* letter of subscribe/unsubscribe. */
/* The third char of ACTION_SC/TC/UV/VC has to be 'c' */

/* user subscription confirm */
#define ACTION_SC "sc."
/* moderator subscription confirm */
#define ACTION_TC "tc."
/* user unsubscribe confirm */
#define ACTION_UC "uc."
/* moderator unsubscribe confirm */
#define ACTION_VC "vc."

/* name addition for digest, i.e. list-"digest" Don't change! */
#define ACTION_DIGEST "digest"

/* name addition for dir/extra db, i.e. list-"allow" */
#define ACTION_ALLOW "allow"
/* name addition for dir/blacklist db, i.e. list-"deny" */
#define ACTION_DENY "deny"

/* defaults for message time out in moderation queue. If modsub is 0 */
/* or empty, DELAY_DEFAULT is used. If it is set, it is made to be  */
/* within DELAY_MIN .. DELAY_MAX. All in hours. */
#define DELAY_MIN 24
#define DELAY_DEFAULT 120
#define DELAY_MAX 240

/* Mode of messages in moderation queue. The owner mode is |'d with 7.*/
/* The group/world mode can be set to anything, but it really doesn't */
/* make sense to make these messages visible to anyone else.          */
#define MODE_MOD_MSG 0700

/* name and location of system-wide customized ezmlmrc. This is where */
/* ezmlm-make looks first (unless the -c switch is specified) before  */
/* falling back to the (usually unchanged) version in the ezmlm bin   */
/* directory. */
#define TXT_ETC_EZMLMRC "/etc/ezmlm/ezmlmrc"

/* same name added to auto_bin. Note leading slash! */
#define TXT_EZMLMRC "/ezmlmrc"

/* same in dot dir for local config (-c) */
#define TXT_DOTEZMLMRC ".ezmlmrc"

/* name of config file for ezmlm-cron */
#define TXT_EZCRONRC "ezcronrc"

/* default timestamp for ezmlm-limit */
#define TXT_LOOPNUM "loopnum"

/* ezmlm-cgi config file for normal SUID root install */
#define EZ_CGIRC "/etc/ezmlm/ezcgirc"

/* ezmlm-cgi config file for local install we expect to find the file in PWD */
#define EZ_CGIRC_LOC ".ezcgirc"

/* default charset for ezmlm-cgi [config file overrides per list] */
#define EZ_CHARSET "iso-8859-1"

/*------------ Specific to SQL version ------------------------------*/
/* cookie tag for SQL version of sublisting */
/* NOTE: Need to include terminal space! */
#define TXT_TAG "X-Ezauth: "

/* max no of bounces that ezmlm-receipt stores */
#define MAX_MAIN_BOUNCES 50

/* Length of domain field for SQL version. It does only the text after */
/* the last '.' in the address, so there is no reason to set it to */
/* anything other than '3'. We truncate it rather than relying on the */
/* SQL Server since we can't be sure that the SQL Server doesn't have */
/* buffer overrun holes and the address is user-controlled */
#define DOMAIN_LENGTH 3

/* programs used for outgoing mail. Normally, qmail-queue is used. Replace */
/* with qmail-qmqpc to use only qmqp for outgoing mail. QMQPC is for */
/* large lists when DIR/qmqpservers is present. Only posts and digests will */
/* use QMQP. If the normal qmail-qmqpc is used the contents of */
/* DIR/qmqpcservers are ignored. With a patch, qmail-qmqpc will use the */
/* servers on it's command line. In this case, the IP addresses listed one */
/* per line in DIR/qmqpservers will be tried until a working one is found. */
/* the option is mainly to allow large list clusters on a single host to use */
/* different QMQPC hosts as exploders.*/
#define PROG_QMAIL_QUEUE "bin/qmail-queue"
#define PROG_QMAIL_QMQPC "bin/qmail-qmqpc"

/*---------- Things below this line are not configurable -----------*/
/* file in DIR that has the qmqpc servers (if any) */
#define QMQPSERVERS "qmqpservers"
/* database types */
#define FLD_DIGEST 1
#define FLD_ALLOW 2
#define FLD_DENY 3
/* Action types */
#define AC_NONE 0
#define AC_GET 1
#define AC_DIGEST 2
#define AC_THREAD 3
#define AC_INDEX 4
#define AC_LIST 5
#define AC_HELP 6
#define AC_EDIT 7
#define AC_DENY 8
#define AC_LOG 9
#define AC_SUBSCRIBE 10
#define AC_UNSUBSCRIBE 11
#define AC_SC 12
#define AC_LISTN 13

typedef struct msgentry {	/* one per message in range */
  unsigned long subnum;		/* subject number */
  unsigned long authnum;	/* message author number */
  unsigned int date;		/* yyyymm as number */
} msgentry;

typedef struct subentry {	/* one per unique subject in message range */
  void *higher;
  void *lower;
  char *sub;			/* string with terminating '\0' */
				/* when building, higher/lower=0 marks end */
				/* of branch. When printing, start at the  */
				/* beginning of the table and go up until  */
				/* sub = 0. */
  unsigned int sublen;
  unsigned long firstmsg;	/* the first message with this subject*/
  unsigned long lastmsg;	/* the last message with this subject*/
  unsigned char msginthread;	/* number of messages seen in this thread */
} subentry;

typedef struct authentry {	/* one per unique author in message range */
  void *higher;
  void *lower;
  char *auth;			/* string with terminating '\0' */
				/* when building, higher/lower=0 marks end */
				/* of branch. When printing, start at the  */
				/* beginning of the table and go up until  */
				/* auth = 0. */
  unsigned long authlen;
  unsigned long firstmsg;	/* the first message with this author */
				/* lastmsg not very useful as author are less */
				/* clustered than threads */
} authentry;

typedef struct dateentry {	/* date yyyymm and 1st message of that date */
  unsigned int date;
  unsigned int msg;
} dateentry;

extern void die_nomem(void);

#endif

