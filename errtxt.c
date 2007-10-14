#include "errtxt.h"

/* Error messages. If you translate these, I would urge you to keep the */
/* English version as well. I'm happy to include any bilingual versions */
/* of this file with future versions of ezmlm-idx.                      */

const char *ERR_NOMEM = "out of memory";
const char *ERR_NOCMD = "command not available";
const char *ERR_CLOSE = "unable to close ";
const char *ERR_WRITE = "unable to write ";
const char *ERR_READ = "unable to read ";
const char *ERR_READ_KEY = "unable to read key";
const char *ERR_FLUSH = "unable to flush ";
const char *ERR_SEEK = "unable to seek ";
const char *ERR_SYNC = "unable to sync ";
const char *ERR_CHMOD = "unable to chmod ";
const char *ERR_STAT = "unable to stat ";
const char *ERR_DELETE = "unable to delete ";
const char *ERR_READ_INPUT = "unable to read input: ";
const char *ERR_SEEK_INPUT = "unable to seek input: ";
const char *ERR_CREATE = "unable to create ";
const char *ERR_MOVE = "unable to move ";
const char *ERR_OPEN = "unable to open ";
const char *ERR_OBTAIN = "unable to obtain ";
const char *ERR_OPEN_LOCK = "unable to open lock: ";
const char *ERR_OBTAIN_LOCK = "unable to obtain lock: ";
const char *ERR_NOLOCAL = "LOCAL not set";
const char *ERR_NODEFAULT = "DEFAULT not set";
const char *ERR_NODTLINE = "DTLINE not set";
const char *ERR_NOSENDER = "SENDER not set";
const char *ERR_NOHOST = "HOST not set";
const char *ERR_NOEXIST = " does not exist";
const char *ERR_NOEXIST_KEY = " key does not exist";
const char *ERR_SWITCH = "unable to switch to ";
const char *ERR_BOUNCE = "I do not reply to bounce messages (#5.7.2)";
const char *ERR_ANONYMOUS = "I do not reply to senders without host names (#5.7.2)";
const char *ERR_NOT_PUBLIC = "Sorry, I've been told to reject all requests (#5.7.2)";
const char *ERR_NOT_ARCHIVED = "Sorry, this list is not archived (#5.1.1)";
const char *ERR_NOT_INDEXED = "Sorry, this list is not indexed (#5.1.1)";
const char *ERR_NOT_AVAILABLE = "Command not available (#5.1.1)";
const char *ERR_NOT_ALLOWED = "Command allowed only to moderators (#5.7.1)";
const char *ERR_BAD_ADDRESS = "I don't accept messages at this address (local doesn't match) (#5.1.1)";
const char *ERR_BAD_RETURN_ADDRESS = "Invalid bounce or receipt address format (#5.1.1)";
const char *ERR_BAD_REQUEST = "Illegal request format (#5.7.1)";
const char *ERR_QMAIL_QUEUE = "unable to run qmail-queue: ";
const char *ERR_TMP_QMAIL_QUEUE = "temporary qmail-queue error: ";
const char *ERR_NOT_PARENT = "this message is not from my parent list (#5.7.2)";
const char *ERR_SUBLIST = "sublist messages must have a Mailing-List header (#5.7.2)";
const char *ERR_MAILING_LIST = "message already has a Mailing-List header (maybe I should be a sublist) (#5.7.2)";
const char *ERR_LOOPING = "this message is looping: it already has my Delivered-To line (#5.4.6)";
const char *ERR_SUBSCRIBER_CAN = "only subscribers can ";
const char *ERR_571 = " (#5.7.1)";
const char *ERR_EMPTY_DIGEST = "nothing to digest";
const char *ERR_EMPTY_LIST = "no messages in archive";
const char *ERR_NOINDEX = "Sorry, I can't find the index for this message";
const char *ERR_BAD_INDEX = "Old format or corrupted index. Run ezmlm-idx! (#5.3.0)";
const char *ERR_BAD_DIGCODE = "incorrect digest code (#5.7.1)";
const char *ERR_UNEXPECTED = "program logic error (#5.3.0)";
const char *ERR_BAD_ALL = "Sorry, after removing unacceptable MIME parts from your message I was left with nothing (#5.7.0)";
const char *ERR_MIME_QUOTE = "MIME boundary lacks end quote";
const char *ERR_SUBST_UNSAFE = "Sorry, substitution of target addresses into headers with <#A#> or <#t#> is unsafe and not permitted.";

/* ezmlm-request unique */
const char *ERR_REQ_LISTNAME = "This command requires a mailing list name (#5.1.1)";
const char *ERR_REQ_LOCAL = "the local part of the command string does not match this list (#5.1.1)";

/* ezmlm-reject unique */
const char *ERR_MAX_SIZE = "Sorry, I don't accept messages larger than ";
const char *ERR_MIN_SIZE = "Sorry, I don't accept messages shorter than ";
const char *ERR_SIZE_CODE = " (#5.2.3)";
const char *ERR_NO_ADDRESS = "List address must be in To: or Cc: (#5.7.0)";
const char *ERR_NO_SUBJECT = "Sorry, I don't accept message with empty Subject (#5.7.0)";
const char *ERR_SUBCOMMAND = "Sorry, I don't accept commands in the subject line. Please send a message to the -help address shown in the the ``Mailing-List:'' header for command info (#5.7.0)";
const char *ERR_BODYCOMMAND = "Sorry, as the message starts with ``[un]subscribe'' it looks like an adminstrative request. Please send a message to the -help address shown in the ``Mailing-List:'' header for [un]subscribe info (#5.7.0)";
const char *ERR_BAD_TYPE = "Sorry, I don't accept messages of MIME Content-Type '";
const char *ERR_BAD_PART = "Sorry, a message part has an unacceptable MIME Content-Type: ";
const char *ERR_JUNK = "Precedence: junk - message ignored";

/* ezmlm-manage unique */
const char *ERR_SUB_NOP = "target is already a subscriber";
const char *ERR_UNSUB_NOP = "target is not a subscriber";
const char *ERR_BAD_NAME = "only letters and underscore allowed in file name (#5.6.0)";
const char *ERR_NO_MARK = "missing start-of-text or end-of-text mark (#5.6.0)";
const char *ERR_EDSIZE = "Maximum edit file size exceeded (#5.6.0)";
const char *ERR_BAD_CHAR = "NUL or other illegal character in input (#5.6.0)";
const char *ERR_EXTRA_SUB = "Processed SENDER check addition request for: ";
const char *ERR_EXTRA_UNSUB = "Processed SENDER check removal request for: ";


/* ezmlm-moderation functions unique */
const char *ERR_MOD_TIMEOUT = "I'm sorry, I no longer have this message";
const char *ERR_MOD_ACCEPTED = "I'm sorry, I've already accepted this message";
const char *ERR_MOD_REJECTED = "I'm sorry, I've already rejected this message";
const char *ERR_MOD_COOKIE = "Illegal or outdated moderator request (#5.7.1)";
const char *ERR_FORK = "unable to fork: ";
const char *ERR_EXECUTE = "unable to execute ";
const char *ERR_CHILD_CRASHED = "child crashed";
const char *ERR_CHILD_FATAL = "fatal error from child";
const char *ERR_CHILD_TEMP = "temporary error from child";
const char *ERR_CHILD_UNKNOWN = "unknown error from child";
const char *ERR_UNIQUE = "unable to create unique message file name";
const char *ERR_NO_POST = "I'm sorry, you are not allowed to post messages to this list (#5.7.2)";

/* ezmlm-make unique */
const char *ERR_VERSION = "ezmlmrc version mismatch. Behavior may not match docs.";
const char *ERR_ENDTAG = "tag lacks /> end marker: ";
const char *ERR_LINKDIR = "linktag lacks /dir: ";
const char *ERR_FILENAME = "continuation tag without defined file name: ";
const char *ERR_PERIOD = "periods not allowed in tags: ";
const char *ERR_SLASH = "dir and dot must start with slash";
const char *ERR_NEWLINE = "newlines not allowed in dir";
const char *ERR_QUOTE = "quotes not allowed in dir";
const char *ERR_SYNTAX = " syntax error: ";
const char *ERR_MKTAB = "creating subscriber tables failed: ";

/* ezmlm-limit unique */
const char *ERR_EXCESS_MOD = "excess traffic: moderating";
const char *ERR_EXCESS_DEFER = "excess traffic: deferring";

/* ezmlm-cron unique */
const char *ERR_SAME_HOST = "list and digest must be on same host";
const char *ERR_DOW = "single comma-separated digits only for day-of-week";
const char *ERR_NOT_CLEAN = "Bad character in address components";
const char *ERR_SUID = "Sorry, I won't run as root";
const char *ERR_UID = "user id not found";
const char *ERR_EUID = "effective user id not found";
const char *ERR_BADUSER = "user not allowed";
const char *ERR_BADHOST = "list host not allowed";
const char *ERR_BADLOCAL = "list local not allowed";
const char *ERR_LISTNO = "max number of list entries exceeded";
const char *ERR_NO_MATCH = "no matching entry found";
const char *ERR_SETUID = "unable to set uid: ";
const char *ERR_CFHOST = "bounce-host required on first line of ";
const char *ERR_EXCLUSIVE = "action-controlling switches are mutually exclusive";
const char *ERR_CRONTAB = "crontab update failed. Contact you sysadmin with the above error information";

/* ezmlm-gate */
const char *ERR_REJECT = "Sorry, I've been told to reject this message (#5.7.0)";

/* issub/subscribe ... */
const char *ERR_ADDR_AT = "address does not contain @";
const char *ERR_ADDR_LONG = "address is too long";
const char *ERR_ADDR_NL = "address contains newline";

/* sql */
const char *ERR_COOKIE = "message does not have valid authentication token";
const char *ERR_NOT_ACTIVE = "this sublist is not active";
const char *ERR_PARSE = "unable to parse ";
const char *ERR_DONE = "message already successfully processed by this list";
const char *ERR_MAX_BOUNCE = "max bounces exceeded: bounce will not be saved";
const char *ERR_NO_PLUGIN = "no plugin specified in database connect data";
const char *ERR_NO_ABSOLUTE = "absolute directory names outside of the list directory are no longer supported";
const char *ERR_NO_LEVELS = "subscriber table names may not contain slashes";
