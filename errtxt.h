/*$Id$*/

#ifndef ERRTXT_H
#define ERRTXT_H

/* Error messages. If you translate these, I would urge you to keep the */
/* English version as well. I'm happy to include any bilingual versions */
/* of this file with future versions of ezmlm-idx.                      */

#define ERR_NOMEM "out of memory"
#define ERR_NOCMD "command not available"
#define ERR_CLOSE "unable to close "
#define ERR_WRITE "unable to write "
#define ERR_READ "unable to read "
#define ERR_READ_KEY "unable to read key"
#define ERR_FLUSH "unable to flush "
#define ERR_SEEK "unalble to seek "
#define ERR_SYNC "unable to sync "
#define ERR_CHMOD "unable to chmod "
#define ERR_STAT "unable to stat "
#define ERR_DELETE "unable to delete "
#define ERR_READ_INPUT "unable to read input: "
#define ERR_SEEK_INPUT "unable to seek input: "
#define ERR_CREATE "unable to create "
#define ERR_MOVE "unable to move "
#define ERR_OPEN "unable to open "
#define ERR_OBTAIN "unable to obtain "
#define ERR_OPEN_LOCK "unable to open lock: "
#define ERR_OBTAIN_LOCK "unable to obtain lock: "
#define ERR_NOLOCAL "LOCAL not set"
#define ERR_NODTLINE "DTLINE not set"
#define ERR_NOSENDER "SENDER not set"
#define ERR_NOHOST "HOST not set"
#define ERR_NOEXIST " does not exist"
#define ERR_NOEXIST_KEY " key does not exist"
#define ERR_SWITCH "unable to switch to "
#define ERR_BOUNCE "I do not reply to bounce messages (#5.7.2)"
#define ERR_ANONYMOUS "I do not reply to senders without host names (#5.7.2)"
#define ERR_NOT_PUBLIC "Sorry, I've been told to reject all requests (#5.7.2)"
#define ERR_NOT_ARCHIVED "Sorry, this list is not archived (#5.1.1)"
#define ERR_NOT_INDEXED "Sorry, this list is not indexed (#5.1.1)"
#define ERR_NOT_AVAILABLE "Command not available (#5.1.1)"
#define ERR_NOT_ALLOWED "Command allowed only to moderators (#5.7.1)"
#define ERR_BAD_ADDRESS "I don't accept messages at this address (inlocal and/or inhost don't match) (#5.1.1)"
#define ERR_BAD_RETURN_ADDRESS "Invalid bounce or receipt address format (#5.1.1)"
#define ERR_BAD_REQUEST "Illegal request format (#5.7.1)"
#define ERR_QMAIL_QUEUE "unable to run qmail-queue: "
#define ERR_TMP_QMAIL_QUEUE "temporary qmail-queue error: "
#define ERR_NOT_PARENT "this message is not from my parent list (#5.7.2)"
#define ERR_SUBLIST "sublist messages must have a Mailing-List header (#5.7.2)"
#define ERR_MAILING_LIST "message already has a Mailing-List header (maybe I should be a sublist) (#5.7.2)"
#define ERR_LOOPING "this message is looping: it already has my Delivered-To line (#5.4.6)"
#define ERR_SUBSCRIBER_CAN "only subscribers can "
#define ERR_571 " (#5.7.1)"
#define ERR_EMPTY_DIGEST "nothing to digest"
#define ERR_EMPTY_LIST "no messages in archive"
#define ERR_NOINDEX "Sorry, I can't find the index for this message"
#define ERR_BAD_INDEX "Old format or corrupted index. Run ezmlm-idx! (#5.3.0)"
#define ERR_BAD_DIGCODE "incorrect digest code (#5.7.1)"
#define ERR_UNEXPECTED "program logic error (#5.3.0)"
#define ERR_BAD_ALL "Sorry, after removing unacceptable MIME parts from your message I was left with nothing (#5.7.0)"
#define ERR_MIME_QUOTE "MIME boundary lacks end quote"
#define ERR_SUBST_UNSAFE "Sorry, substitution of target addresses into headers with <#A#> or <#T#> is unsafe and not permitted."

/* ezmlm-request unique */
#define ERR_REQ_LISTNAME "This command requires a mailing list name (#5.1.1)"
#define ERR_REQ_LOCAL "the local part of the command string does not match this list (#5.1.1)"

/* ezmlm-reject unique */
#define ERR_MAX_SIZE "Sorry, I don't accept messages larger than "
#define ERR_MIN_SIZE "Sorry, I don't accept messages shorter than "
#define ERR_SIZE_CODE " (#5.2.3)"
#define ERR_NO_ADDRESS "List address must be in To: or Cc: (#5.7.0)"
#define ERR_NO_SUBJECT "Sorry, I don't accept message with empty Subject (#5.7.0)"
#define ERR_SUBCOMMAND "Sorry, I don't accept commands in the subject line. Please send a message to the -help address shown in the the ``Mailing-List:'' header for command info (#5.7.0)"
#define ERR_BODYCOMMAND "Sorry, as the message starts with ``[un]subscribe'' it looks like an adminstrative request. Please send a message to the -help address shown in the ``Mailing-List:'' header for [un]subscribe info (#5.7.0)"
#define ERR_BAD_TYPE "Sorry, I don't accept messages of MIME Content-Type '"
#define ERR_BAD_PART "Sorry, a message part has an unacceptable MIME Content-Type: "
#define ERR_JUNK "Precedence: junk - message ignored"

/* ezmlm-manage unique */
#define ERR_SUB_NOP "target is already a subscriber"
#define ERR_UNSUB_NOP "target is not a subscriber"
#define ERR_BAD_NAME "only letters and underscore allowed in file name (#5.6.0)"
#define ERR_NO_MARK "missing start-of-text or end-of-text mark (#5.6.0)"
#define ERR_EDSIZE "Maximum edit file size exceeded (#5.6.0)"
#define ERR_BAD_CHAR "NUL or other illegal character in input (#5.6.0)"
#define ERR_EXTRA_SUB "Processed SENDER check addition request for: "
#define ERR_EXTRA_UNSUB "Processed SENDER check removal request for: "


/* ezmlm-moderation functions unique */
#define ERR_MOD_TIMEOUT "I'm sorry, I no longer have this message"
#define ERR_MOD_ACCEPTED "I'm sorry, I've already accepted this message"
#define ERR_MOD_REJECTED "I'm sorry, I've already rejected this message"
#define ERR_MOD_COOKIE "Illegal or outdated moderator request (#5.7.1)"
#define ERR_FORK "unable to fork: "
#define ERR_EXECUTE "unable to execute "
#define ERR_CHILD_CRASHED "child crashed"
#define ERR_CHILD_FATAL "fatal error from child"
#define ERR_CHILD_TEMP "temporary error from child"
#define ERR_CHILD_UNKNOWN "unknown error from child"
#define ERR_UNIQUE "unable to create unique message file name"
#define ERR_NO_POST "I'm sorry, you are not allowed to post messages to this list (#5.7.2)"

/* ezmlm-make unique */
#define ERR_VERSION "ezmlmrc version mismatch. Behavior may not match docs."
#define ERR_ENDTAG "tag lacks /> end marker: "
#define ERR_LINKDIR "linktag lacks /dir: "
#define ERR_FILENAME "continuation tag without defined file name: "
#define ERR_PERIOD "periods not allowed in tags: "
#define ERR_SLASH "dir and dot must start with slash"
#define ERR_NEWLINE "newlines not allowed in dir"
#define ERR_QUOTE "quotes not allowed in dir"
#define ERR_SYNTAX " syntax error: "

/* ezmlm-limit unique */
#define ERR_EXCESS_MOD "excess traffic: moderating"
#define ERR_EXCESS_DEFER "excess traffic: deferring"

/* ezmlm-cron unique */
#define ERR_SAME_HOST "list and digest must be on same host"
#define ERR_DOW "single comma-separated digits only for day-of-week"
#define ERR_NOT_CLEAN "Bad character in address components"
#define ERR_SUID "Sorry, I won't run as root"
#define ERR_UID "user id not found"
#define ERR_EUID "effective user id not found"
#define ERR_BADUSER "user not allowed"
#define ERR_BADHOST "list host not allowed"
#define ERR_BADLOCAL "list local not allowed"
#define ERR_LISTNO "max number of list entries exceeded"
#define ERR_NO_MATCH "no matching entry found"
#define ERR_SETUID "unable to set uid: "
#define ERR_CFHOST "bounce-host required on first line of "
#define ERR_EXCLUSIVE "action-controlling switches are mutually exclusive"
#define ERR_CRONTAB "crontab update failed. Contact you sysadmin with the above error information"

/* ezmlm-gate */
#define ERR_REJECT "Sorry, I've been told to reject this message (#5.7.0)"

/* issub/subscribe ... */
#define ERR_ADDR_AT "address does not contain @"
#define ERR_ADDR_LONG "address is too long"
#define ERR_ADDR_NL "address contains newline"

/* sql */
#define ERR_COOKIE "message does not have valid authentication token"
#define ERR_NOT_ACTIVE "this sublist is not active"
#define ERR_PARSE "unable to parse "
#define ERR_DONE "message already successfully processed by this list"
#define ERR_MAX_BOUNCE "max bounces exceeded: bounce will not be saved"
#define ERR_NO_TABLE "no table specified in database connect data"

#endif

