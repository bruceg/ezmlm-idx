#include "msgtxt.h"
#include "constmap.h"
#include "die.h"
#include "str.h"
#include "strerr.h"

/* Error messages. If you translate these, I would urge you to keep the */
/* English version as well. I'm happy to include any bilingual versions */
/* of this file with future versions of ezmlm-idx.                      */

static const char errtxts[] =
"NOMEM:out of memory\0"
"NOCMD:command not available\0"
"CLOSE:unable to close \0"
"WRITE:unable to write \0"
"READ:unable to read \0"
"READ_KEY:unable to read key\0"
"FLUSH:unable to flush \0"
"SEEK:unable to seek \0"
"SYNC:unable to sync \0"
"CHMOD:unable to chmod \0"
"STAT:unable to stat \0"
"DELETE:unable to delete \0"
"READ_INPUT:unable to read input: \0"
"SEEK_INPUT:unable to seek input: \0"
"CREATE:unable to create \0"
"MOVE:unable to move \0"
"OPEN:unable to open \0"
"OBTAIN:unable to obtain \0"
"OPEN_LOCK:unable to open lock: \0"
"OBTAIN_LOCK:unable to obtain lock: \0"
"NOLOCAL:LOCAL not set\0"
"NODEFAULT:DEFAULT not set\0"
"NODTLINE:DTLINE not set\0"
"NOSENDER:SENDER not set\0"
"NOHOST:HOST not set\0"
"NOEXIST: does not exist\0"
"NOEXIST_KEY: key does not exist\0"
"SWITCH:unable to switch to \0"
"BOUNCE:I do not reply to bounce messages (#5.7.2)\0"
"ANONYMOUS:I do not reply to senders without host names (#5.7.2)\0"
"NOT_PUBLIC:Sorry, I've been told to reject all requests (#5.7.2)\0"
"NOT_ARCHIVED:Sorry, this list is not archived (#5.1.1)\0"
"NOT_INDEXED:Sorry, this list is not indexed (#5.1.1)\0"
"NOT_AVAILABLE:Command not available (#5.1.1)\0"
"NOT_ALLOWED:Command allowed only to moderators (#5.7.1)\0"
"BAD_ADDRESS:I don't accept messages at this address (local doesn't match) (#5.1.1)\0"
"BAD_RETURN_ADDRESS:Invalid bounce or receipt address format (#5.1.1)\0"
"BAD_REQUEST:Illegal request format (#5.7.1)\0"
"QMAIL_QUEUE:unable to run qmail-queue: \0"
"TMP_QMAIL_QUEUE:temporary qmail-queue error: \0"
"NOT_PARENT:this message is not from my parent list (#5.7.2)\0"
"SUBLIST:sublist messages must have a Mailing-List header (#5.7.2)\0"
"MAILING_LIST:message already has a Mailing-List header (maybe I should be a sublist) (#5.7.2)\0"
"LOOPING:this message is looping: it already has my Delivered-To line (#5.4.6)\0"
"SUBSCRIBER_CAN:only subscribers can \0"
"571: (#5.7.1)\0"
"EMPTY_DIGEST:nothing to digest\0"
"EMPTY_LIST:no messages in archive\0"
"NOINDEX:Sorry, I can't find the index for this message\0"
"BAD_INDEX:Old format or corrupted index. Run ezmlm-idx! (#5.3.0)\0"
"BAD_DIGCODE:incorrect digest code (#5.7.1)\0"
"UNEXPECTED:program logic error (#5.3.0)\0"
"BAD_ALL:Sorry, after removing unacceptable MIME parts from your message I was left with nothing (#5.7.0)\0"
"MIME_QUOTE:MIME boundary lacks end quote\0"
"SUBST_UNSAFE:Sorry, substitution of target addresses into headers with <#A#> or <#t#> is unsafe and not permitted.\0"

/* ezmlm-request unique */
"REQ_LISTNAME:This command requires a mailing list name (#5.1.1)\0"
"REQ_LOCAL:the local part of the command string does not match this list (#5.1.1)\0"

/* ezmlm-reject unique */
"MAX_SIZE:Sorry, I don't accept messages larger than \0"
"MIN_SIZE:Sorry, I don't accept messages shorter than \0"
"SIZE_CODE: (#5.2.3)\0"
"NO_ADDRESS:List address must be in To: or Cc: (#5.7.0)\0"
"NO_SUBJECT:Sorry, I don't accept message with empty Subject (#5.7.0)\0"
"SUBCOMMAND:Sorry, I don't accept commands in the subject line. Please send a message to the -help address shown in the the ``Mailing-List:'' header for command info (#5.7.0)\0"
"BODYCOMMAND:Sorry, as the message starts with ``[un]subscribe'' it looks like an adminstrative request. Please send a message to the -help address shown in the ``Mailing-List:'' header for [un]subscribe info (#5.7.0)\0"
"BAD_TYPE:Sorry, I don't accept messages of MIME Content-Type '\0"
"BAD_PART:Sorry, a message part has an unacceptable MIME Content-Type: \0"
"JUNK:Precedence: junk - message ignored\0"

/* ezmlm-manage unique */
"SUB_NOP:target is already a subscriber\0"
"UNSUB_NOP:target is not a subscriber\0"
"BAD_NAME:only letters and underscore allowed in file name (#5.6.0)\0"
"NO_MARK:missing start-of-text or end-of-text mark (#5.6.0)\0"
"EDSIZE:Maximum edit file size exceeded (#5.6.0)\0"
"BAD_CHAR:NUL or other illegal character in input (#5.6.0)\0"
"EXTRA_SUB:Processed SENDER check addition request for: \0"
"EXTRA_UNSUB:Processed SENDER check removal request for: \0"


/* ezmlm-moderation functions unique */
"MOD_TIMEOUT:I'm sorry, I no longer have this message\0"
"MOD_ACCEPTED:I'm sorry, I've already accepted this message\0"
"MOD_REJECTED:I'm sorry, I've already rejected this message\0"
"MOD_COOKIE:Illegal or outdated moderator request (#5.7.1)\0"
"FORK:unable to fork: \0"
"EXECUTE:unable to execute \0"
"CHILD_CRASHED:child crashed\0"
"CHILD_FATAL:fatal error from child\0"
"CHILD_TEMP:temporary error from child\0"
"CHILD_UNKNOWN:unknown error from child\0"
"UNIQUE:unable to create unique message file name\0"
"NO_POST:I'm sorry, you are not allowed to post messages to this list (#5.7.2)\0"

/* ezmlm-make unique */
"VERSION:ezmlmrc version mismatch. Behavior may not match docs.\0"
"ENDTAG:tag lacks /> end marker: \0"
"LINKDIR:linktag lacks /dir: \0"
"FILENAME:continuation tag without defined file name: \0"
"PERIOD:periods not allowed in tags: \0"
"SLASH:dir and dot must start with slash\0"
"NEWLINE:newlines not allowed in dir\0"
"QUOTE:quotes not allowed in dir\0"
"SYNTAX: syntax error: \0"
"MKTAB:creating subscriber tables failed: \0"

/* ezmlm-limit unique */
"EXCESS_MOD:excess traffic: moderating\0"
"EXCESS_DEFER:excess traffic: deferring\0"

/* ezmlm-cron unique */
"SAME_HOST:list and digest must be on same host\0"
"DOW:single comma-separated digits only for day-of-week\0"
"NOT_CLEAN:Bad character in address components\0"
"SUID:Sorry, I won't run as root\0"
"UID:user id not found\0"
"EUID:effective user id not found\0"
"BADUSER:user not allowed\0"
"BADHOST:list host not allowed\0"
"BADLOCAL:list local not allowed\0"
"LISTNO:max number of list entries exceeded\0"
"NO_MATCH:no matching entry found\0"
"SETUID:unable to set uid: \0"
"CFHOST:bounce-host required on first line of \0"
"EXCLUSIVE:action-controlling switches are mutually exclusive\0"
"CRONTAB:crontab update failed. Contact you sysadmin with the above error information\0"

/* ezmlm-gate */
"REJECT:Sorry, I've been told to reject this message (#5.7.0)\0"

/* issub/subscribe ... */
"ADDR_AT:address does not contain @\0"
"ADDR_LONG:address is too long\0"
"ADDR_NL:address contains newline\0"

/* sql */
"COOKIE:message does not have valid authentication token\0"
"NOT_ACTIVE:this sublist is not active\0"
"PARSE:unable to parse \0"
"DONE:message already successfully processed by this list\0"
"MAX_BOUNCE:max bounces exceeded: bounce will not be saved\0"
"NO_PLUGIN:no plugin specified in database connect data\0"
"NO_ABSOLUTE:absolute directory names outside of the list directory are no longer supported\0"
"NO_LEVELS:subscriber table names may not contain slashes\0"
;

static struct constmap errmap = {0};

void errtxt_init(void)
{
  if (!constmap_init(&errmap,errtxts,sizeof errtxts,1))
    strerr_die2x(100,FATAL,"out of memory");
}

const char *ERR(const char *name)
{
  const char *c;
  return ((c = constmap(&errmap,name,str_len(name))) != 0)
    ? c
    : "unknown error";
}
