#include "msgtxt.h"
#include "constmap.h"
#include "die.h"
#include "str.h"
#include "strerr.h"

/* Error messages. If you translate these, I would urge you to keep the */
/* English version as well. I'm happy to include any bilingual versions */
/* of this file with future versions of ezmlm-idx.                      */

static const char msgtxts[] =
"ERR_NOMEM:out of memory\0"
"ERR_NOCMD:command not available\0"
"ERR_CLOSE:unable to close \0"
"ERR_WRITE:unable to write \0"
"ERR_READ:unable to read \0"
"ERR_READ_KEY:unable to read key\0"
"ERR_FLUSH:unable to flush \0"
"ERR_SEEK:unable to seek \0"
"ERR_SYNC:unable to sync \0"
"ERR_CHMOD:unable to chmod \0"
"ERR_STAT:unable to stat \0"
"ERR_DELETE:unable to delete \0"
"ERR_READ_INPUT:unable to read input: \0"
"ERR_SEEK_INPUT:unable to seek input: \0"
"ERR_CREATE:unable to create \0"
"ERR_MOVE:unable to move \0"
"ERR_OPEN:unable to open \0"
"ERR_OBTAIN:unable to obtain \0"
"ERR_OPEN_LOCK:unable to open lock: \0"
"ERR_OBTAIN_LOCK:unable to obtain lock: \0"
"ERR_NOLOCAL:LOCAL not set\0"
"ERR_NODEFAULT:DEFAULT not set\0"
"ERR_NODTLINE:DTLINE not set\0"
"ERR_NOSENDER:SENDER not set\0"
"ERR_NOHOST:HOST not set\0"
"ERR_NOEXIST: does not exist\0"
"ERR_NOEXIST_KEY: key does not exist\0"
"ERR_SWITCH:unable to switch to \0"
"ERR_BOUNCE:I do not reply to bounce messages (#5.7.2)\0"
"ERR_ANONYMOUS:I do not reply to senders without host names (#5.7.2)\0"
"ERR_NOT_PUBLIC:Sorry, I've been told to reject all requests (#5.7.2)\0"
"ERR_NOT_ARCHIVED:Sorry, this list is not archived (#5.1.1)\0"
"ERR_NOT_INDEXED:Sorry, this list is not indexed (#5.1.1)\0"
"ERR_NOT_AVAILABLE:Command not available (#5.1.1)\0"
"ERR_NOT_ALLOWED:Command allowed only to moderators (#5.7.1)\0"
"ERR_BAD_ADDRESS:I don't accept messages at this address (local doesn't match) (#5.1.1)\0"
"ERR_BAD_RETURN_ADDRESS:Invalid bounce or receipt address format (#5.1.1)\0"
"ERR_BAD_REQUEST:Illegal request format (#5.7.1)\0"
"ERR_QMAIL_QUEUE:unable to run qmail-queue: \0"
"ERR_TMP_QMAIL_QUEUE:temporary qmail-queue error: \0"
"ERR_NOT_PARENT:this message is not from my parent list (#5.7.2)\0"
"ERR_SUBLIST:sublist messages must have a Mailing-List header (#5.7.2)\0"
"ERR_MAILING_LIST:message already has a Mailing-List header (maybe I should be a sublist) (#5.7.2)\0"
"ERR_LOOPING:this message is looping: it already has my Delivered-To line (#5.4.6)\0"
"ERR_SUBSCRIBER_CAN:only subscribers can \0"
"ERR_571: (#5.7.1)\0"
"ERR_EMPTY_DIGEST:nothing to digest\0"
"ERR_EMPTY_LIST:no messages in archive\0"
"ERR_NOINDEX:Sorry, I can't find the index for this message\0"
"ERR_BAD_INDEX:Old format or corrupted index. Run ezmlm-idx! (#5.3.0)\0"
"ERR_BAD_DIGCODE:incorrect digest code (#5.7.1)\0"
"ERR_UNEXPECTED:program logic error (#5.3.0)\0"
"ERR_BAD_ALL:Sorry, after removing unacceptable MIME parts from your message I was left with nothing (#5.7.0)\0"
"ERR_MIME_QUOTE:MIME boundary lacks end quote\0"
"ERR_SUBST_UNSAFE:Sorry, substitution of target addresses into headers with <#A#> or <#t#> is unsafe and not permitted.\0"

/* ezmlm-request unique */
"ERR_REQ_LISTNAME:This command requires a mailing list name (#5.1.1)\0"
"ERR_REQ_LOCAL:the local part of the command string does not match this list (#5.1.1)\0"

/* ezmlm-reject unique */
"ERR_MAX_SIZE:Sorry, I don't accept messages larger than \0"
"ERR_MIN_SIZE:Sorry, I don't accept messages shorter than \0"
"ERR_SIZE_CODE: (#5.2.3)\0"
"ERR_NO_ADDRESS:List address must be in To: or Cc: (#5.7.0)\0"
"ERR_NO_SUBJECT:Sorry, I don't accept message with empty Subject (#5.7.0)\0"
"ERR_SUBCOMMAND:Sorry, I don't accept commands in the subject line. Please send a message to the -help address shown in the the ``Mailing-List:'' header for command info (#5.7.0)\0"
"ERR_BODYCOMMAND:Sorry, as the message starts with ``[un]subscribe'' it looks like an adminstrative request. Please send a message to the -help address shown in the ``Mailing-List:'' header for [un]subscribe info (#5.7.0)\0"
"ERR_BAD_TYPE:Sorry, I don't accept messages of MIME Content-Type '\0"
"ERR_BAD_PART:Sorry, a message part has an unacceptable MIME Content-Type: \0"
"ERR_JUNK:Precedence: junk - message ignored\0"

/* ezmlm-manage unique */
"ERR_SUB_NOP:target is already a subscriber\0"
"ERR_UNSUB_NOP:target is not a subscriber\0"
"ERR_BAD_NAME:only letters and underscore allowed in file name (#5.6.0)\0"
"ERR_NO_MARK:missing start-of-text or end-of-text mark (#5.6.0)\0"
"ERR_EDSIZE:Maximum edit file size exceeded (#5.6.0)\0"
"ERR_BAD_CHAR:NUL or other illegal character in input (#5.6.0)\0"
"ERR_EXTRA_SUB:Processed SENDER check addition request for: \0"
"ERR_EXTRA_UNSUB:Processed SENDER check removal request for: \0"


/* ezmlm-moderation functions unique */
"ERR_MOD_TIMEOUT:I'm sorry, I no longer have this message\0"
"ERR_MOD_ACCEPTED:I'm sorry, I've already accepted this message\0"
"ERR_MOD_REJECTED:I'm sorry, I've already rejected this message\0"
"ERR_MOD_COOKIE:Illegal or outdated moderator request (#5.7.1)\0"
"ERR_FORK:unable to fork: \0"
"ERR_EXECUTE:unable to execute \0"
"ERR_CHILD_CRASHED:child crashed\0"
"ERR_CHILD_FATAL:fatal error from child\0"
"ERR_CHILD_TEMP:temporary error from child\0"
"ERR_CHILD_UNKNOWN:unknown error from child\0"
"ERR_UNIQUE:unable to create unique message file name\0"
"ERR_NO_POST:I'm sorry, you are not allowed to post messages to this list (#5.7.2)\0"

/* ezmlm-make unique */
"ERR_VERSION:ezmlmrc version mismatch. Behavior may not match docs.\0"
"ERR_ENDTAG:tag lacks /> end marker: \0"
"ERR_LINKDIR:linktag lacks /dir: \0"
"ERR_FILENAME:continuation tag without defined file name: \0"
"ERR_PERIOD:periods not allowed in tags: \0"
"ERR_SLASH:dir and dot must start with slash\0"
"ERR_NEWLINE:newlines not allowed in dir\0"
"ERR_QUOTE:quotes not allowed in dir\0"
"ERR_SYNTAX: syntax error: \0"
"ERR_MKTAB:creating subscriber tables failed: \0"

/* ezmlm-limit unique */
"ERR_EXCESS_MOD:excess traffic: moderating\0"
"ERR_EXCESS_DEFER:excess traffic: deferring\0"

/* ezmlm-cron unique */
"ERR_SAME_HOST:list and digest must be on same host\0"
"ERR_DOW:single comma-separated digits only for day-of-week\0"
"ERR_NOT_CLEAN:Bad character in address components\0"
"ERR_SUID:Sorry, I won't run as root\0"
"ERR_UID:user id not found\0"
"ERR_EUID:effective user id not found\0"
"ERR_BADUSER:user not allowed\0"
"ERR_BADHOST:list host not allowed\0"
"ERR_BADLOCAL:list local not allowed\0"
"ERR_LISTNO:max number of list entries exceeded\0"
"ERR_NO_MATCH:no matching entry found\0"
"ERR_SETUID:unable to set uid: \0"
"ERR_CFHOST:bounce-host required on first line of \0"
"ERR_EXCLUSIVE:action-controlling switches are mutually exclusive\0"
"ERR_CRONTAB:crontab update failed. Contact you sysadmin with the above error information\0"

/* ezmlm-gate */
"ERR_REJECT:Sorry, I've been told to reject this message (#5.7.0)\0"

/* issub/subscribe ... */
"ERR_ADDR_AT:address does not contain @\0"
"ERR_ADDR_LONG:address is too long\0"
"ERR_ADDR_NL:address contains newline\0"

/* sql */
"ERR_COOKIE:message does not have valid authentication token\0"
"ERR_NOT_ACTIVE:this sublist is not active\0"
"ERR_PARSE:unable to parse \0"
"ERR_DONE:message already successfully processed by this list\0"
"ERR_MAX_BOUNCE:max bounces exceeded: bounce will not be saved\0"
"ERR_NO_PLUGIN:no plugin specified in database connect data\0"
"ERR_NO_ABSOLUTE:absolute directory names outside of the list directory are no longer supported\0"
"ERR_NO_LEVELS:subscriber table names may not contain slashes\0"
;

static struct constmap msgmap = {0};

void msgtxt_init(void)
{
  if (!constmap_init(&msgmap,msgtxts,sizeof msgtxts,1))
    strerr_die2x(100,FATAL,"out of memory");
}

const char *MSG(const char *name)
{
  const char *c;
  return ((c = constmap(&msgmap,name,str_len(name))) != 0)
    ? c
    : "unknown error";
}
