#ifndef ERRTXT_H
#define ERRTXT_H

extern const char *ERR_NOMEM;
extern const char *ERR_NOCMD;
extern const char *ERR_CLOSE;
extern const char *ERR_WRITE;
extern const char *ERR_READ;
extern const char *ERR_READ_KEY;
extern const char *ERR_FLUSH;
extern const char *ERR_SEEK;
extern const char *ERR_SYNC;
extern const char *ERR_CHMOD;
extern const char *ERR_STAT;
extern const char *ERR_DELETE;
extern const char *ERR_READ_INPUT;
extern const char *ERR_SEEK_INPUT;
extern const char *ERR_CREATE;
extern const char *ERR_MOVE;
extern const char *ERR_OPEN;
extern const char *ERR_OBTAIN;
extern const char *ERR_OPEN_LOCK;
extern const char *ERR_OBTAIN_LOCK;
extern const char *ERR_NOLOCAL;
extern const char *ERR_NODEFAULT;
extern const char *ERR_NODTLINE;
extern const char *ERR_NOSENDER;
extern const char *ERR_NOHOST;
extern const char *ERR_NOEXIST;
extern const char *ERR_NOEXIST_KEY;
extern const char *ERR_SWITCH;
extern const char *ERR_BOUNCE;
extern const char *ERR_ANONYMOUS;
extern const char *ERR_NOT_PUBLIC;
extern const char *ERR_NOT_ARCHIVED;
extern const char *ERR_NOT_INDEXED;
extern const char *ERR_NOT_AVAILABLE;
extern const char *ERR_NOT_ALLOWED;
extern const char *ERR_BAD_ADDRESS;
extern const char *ERR_BAD_RETURN_ADDRESS;
extern const char *ERR_BAD_REQUEST;
extern const char *ERR_QMAIL_QUEUE;
extern const char *ERR_TMP_QMAIL_QUEUE;
extern const char *ERR_NOT_PARENT;
extern const char *ERR_SUBLIST;
extern const char *ERR_MAILING_LIST;
extern const char *ERR_LOOPING;
extern const char *ERR_SUBSCRIBER_CAN;
extern const char *ERR_571;
extern const char *ERR_EMPTY_DIGEST;
extern const char *ERR_EMPTY_LIST;
extern const char *ERR_NOINDEX;
extern const char *ERR_BAD_INDEX;
extern const char *ERR_BAD_DIGCODE;
extern const char *ERR_UNEXPECTED;
extern const char *ERR_BAD_ALL;
extern const char *ERR_MIME_QUOTE;
extern const char *ERR_SUBST_UNSAFE;

/* ezmlm-request unique */
extern const char *ERR_REQ_LISTNAME;
extern const char *ERR_REQ_LOCAL;

/* ezmlm-reject unique */
extern const char *ERR_MAX_SIZE;
extern const char *ERR_MIN_SIZE;
extern const char *ERR_SIZE_CODE;
extern const char *ERR_NO_ADDRESS;
extern const char *ERR_NO_SUBJECT;
extern const char *ERR_SUBCOMMAND;
extern const char *ERR_BODYCOMMAND;
extern const char *ERR_BAD_TYPE;
extern const char *ERR_BAD_PART;
extern const char *ERR_JUNK;

/* ezmlm-manage unique */
extern const char *ERR_SUB_NOP;
extern const char *ERR_UNSUB_NOP;
extern const char *ERR_BAD_NAME;
extern const char *ERR_NO_MARK;
extern const char *ERR_EDSIZE;
extern const char *ERR_BAD_CHAR;
extern const char *ERR_EXTRA_SUB;
extern const char *ERR_EXTRA_UNSUB;


/* ezmlm-moderation functions unique */
extern const char *ERR_MOD_TIMEOUT;
extern const char *ERR_MOD_ACCEPTED;
extern const char *ERR_MOD_REJECTED;
extern const char *ERR_MOD_COOKIE;
extern const char *ERR_FORK;
extern const char *ERR_EXECUTE;
extern const char *ERR_CHILD_CRASHED;
extern const char *ERR_CHILD_FATAL;
extern const char *ERR_CHILD_TEMP;
extern const char *ERR_CHILD_UNKNOWN;
extern const char *ERR_UNIQUE;
extern const char *ERR_NO_POST;

/* ezmlm-make unique */
extern const char *ERR_VERSION;
extern const char *ERR_ENDTAG;
extern const char *ERR_LINKDIR;
extern const char *ERR_FILENAME;
extern const char *ERR_PERIOD;
extern const char *ERR_SLASH;
extern const char *ERR_NEWLINE;
extern const char *ERR_QUOTE;
extern const char *ERR_SYNTAX;
extern const char *ERR_MKTAB;

/* ezmlm-limit unique */
extern const char *ERR_EXCESS_MOD;
extern const char *ERR_EXCESS_DEFER;

/* ezmlm-cron unique */
extern const char *ERR_SAME_HOST;
extern const char *ERR_DOW;
extern const char *ERR_NOT_CLEAN;
extern const char *ERR_SUID;
extern const char *ERR_UID;
extern const char *ERR_EUID;
extern const char *ERR_BADUSER;
extern const char *ERR_BADHOST;
extern const char *ERR_BADLOCAL;
extern const char *ERR_LISTNO;
extern const char *ERR_NO_MATCH;
extern const char *ERR_SETUID;
extern const char *ERR_CFHOST;
extern const char *ERR_EXCLUSIVE;
extern const char *ERR_CRONTAB;

/* ezmlm-gate */
extern const char *ERR_REJECT;

/* issub/subscribe ... */
extern const char *ERR_ADDR_AT;
extern const char *ERR_ADDR_LONG;
extern const char *ERR_ADDR_NL;

/* sql */
extern const char *ERR_COOKIE;
extern const char *ERR_NOT_ACTIVE;
extern const char *ERR_PARSE;
extern const char *ERR_DONE;
extern const char *ERR_MAX_BOUNCE;
extern const char *ERR_NO_PLUGIN;
extern const char *ERR_NO_ABSOLUTE;
extern const char *ERR_NO_LEVELS;

#endif
