
# [should have a bin/sh line and EZPATH/QMPATH added above by make]
#
# script to diagnose ezmlm lists
# call as is for environment only, or as ezmlm-check 'DIR' for more info

listsubscribers=yes
if [ "$1" = '-S' ] ; then
  listsubscribers=no
  shift
else
    if [ "$1" = '-s' ] ; then
      shift
    fi
fi

QINJECT="${QMPATH}/bin/qmail-inject"
EZLIST="${EZPATH}/ezmlm-list"
MYNAME='ezmlm-check'
MYDTLINE="Delivered-To: ${MYNAME}"
FATAL="${MYNAME}: fatal:"
EZERR="??? ERROR:"
EZWARN="!!! Warning:"
# This should be a ``grep'' that does regexps. Needs to recognize ^ and $.
GREP='grep'
CAT='cat'
CUT='cut'
ECHO='echo'
# Needed to isolate some lines in the DIR/editor ... files
HEAD='head'
TAIL='tail'
# needed to count characters in $USER
WC='wc'
# should mark executables with '*' and list one file per line
LS='ls -F1'
# should list links as: "... symlink -> file"
LLS='ls -l'
if [ ! -x "$QINJECT" ]; then
  ${ECHO} "$FATAL edit script to for path to qmail-inject"
  exit 100
fi

if [ -z "$SENDER" ] ; then
	RCP="${CAT}"
else
	RCP="$QINJECT"
	if ${CAT} - | ${GREP} "${MYDTLINE}" >/dev/null 2>&1
	  then
	    ${ECHO} "FATAL this message is looping: it already has my Delivered-To line (#5.4.6)"
	    exit 100
	fi
fi

if [ ! -z "$1" -a ! -d "$1" ]; then
  ${ECHO} "$FATAL $1 is not a directory"; exit 100
fi

# reset variables
GET=''; DIGEST=''; FLAGARCH=''; FLAGIND=''; INLOCAL=''; INLOCALOK='' 
MANAGE=''; OUTLOCAL=''; OUTHOST=''; SPEC=''; SRESTRICT=''; STORE=''; REMOTE=''
MODSUB=''; MODPOST=''; SPEC=''

(
	${ECHO} "Delivered-To: ezmlm-check"
	${ECHO} "To: $SENDER"
	${ECHO} "Subject: ${MYNAME} results"
	${ECHO}
	${ECHO} "Important environment variables:"
	${ECHO}
	${ECHO} "SENDER ='$SENDER'"
	${ECHO} "NEWSENDER ='$NEWSENDER'"
	${ECHO} "RECIPIENT ='$RECIPIENT'"
	${ECHO} "USER ='$USER'"
	${ECHO} "DEFAULT ='$DEFAULT'"
	${ECHO} "LOCAL ='$LOCAL'"
	${ECHO} "HOST ='$HOST'"
	${ECHO} "---------------------------------------------------"
	if [ ! -z "$1" ]; then
	  ${ECHO} "Checking basic list setup:"
	  ${ECHO}
	  if [ ! -r "$1/mailinglist" ]; then
	    ${ECHO} "$EZERR $1/mailinglist does not exist"
	  fi
	  if [ -e "$1/num" ]; then
	    if [ ! -w "$1/num" ]; then
	      ${ECHO} "$EZERR $1/num is not writable to $USER"
	    else
	      NUM=`${CAT} "$1/num" | ${HEAD} -1`
	      ${ECHO} "... latest message was message # $NUM"
            fi
	  else
	    ${ECHO} "... no num. This must be a new list ..."
	  fi
	  ${ECHO}
	  if [ -e "$1/lock" -a ! -w "$1/lock" ]; then
	    ${ECHO} "$EZERR User $USER does not have write premission to $1/lock"
	  fi
	  if [ -e "$1/lockbounce" -a ! -w "$1/lockbounce" ]; then
	    ${ECHO} "$EZERR User $USER does not have write premission to $1/lockbounce"
	  fi
	  if [ ! -r "$1/inlocal" ]; then
	    ${ECHO} "$EZERR $1/inlocal does not exist"
	  else
	    INLOCAL=`${CAT} "$1/inlocal"| ${HEAD} -1`
	    if [ -z "$HOST" ]; then
	      ${ECHO} "$EZERR HOST is empty. Likely running from the command"
	      ${ECHO} "    line. Run from $1/editor to check if LOCAL matches"
	      ${ECHO} "    $1/inlocal."
	      ${ECHO} "    Mismatches here are the most common setup error."
	      ${ECHO}
	    else
	      if ${ECHO} "$LOCAL" | ${GREP} -G "^$INLOCAL" >/dev/null 2>&1
              then
	        ${ECHO} "... $1/inlocal OK"
	      else
	        ${ECHO} \
		 "$EZERR LOCAL does not begin with contents of $1/inlocal"
	      fi
	    fi
	  fi
	  if [ ! -r "$1/outlocal" ]; then
	    ${ECHO} "$EZERR $1/outlocal does not exist"
	  else
	    OUTLOCAL=`${CAT} "$1/outlocal"| ${HEAD} -1`
	  fi
	  if [ ! -r "$1/outhost" ]; then
	      ${ECHO} "$EZERR $1/outhost does not exist"
          else
	      OUTHOST=`${CAT} "$1/outhost"| ${HEAD} -1`
	  fi
	  ${ECHO} "... The list is named ${OUTLOCAL}@${OUTHOST}"
	  if ${ECHO} "$INLOCAL" | ${GREP} "^${USER}" >/dev/null ; then
	    USERSTART='1'; INLOCALOK='1'
	  fi
	  if [ -z "$USERSTART" ]; then
	    ${ECHO}
	    ${ECHO} "??? $1/inlocal does not start with the user name."
	    ${ECHO} "    This is an error, unless $INLOCAL starts with"
	    ${ECHO} "    an alias of \"$USER\"."
	  fi
	  if [ "$INLOCAL" = "$OUTLOCAL" ]; then
	    ${ECHO} "... $1/inlocal matches $1/outlocal"
	    ${ECHO} "    suggesting that this is a regular user-owned list."
	    CHARS=`${ECHO} " $USER" | ${WC} -c`
	    LIST=`${ECHO} "$OUTLOCAL" | cut -c$CHARS-`
	  else
	    if  ${ECHO} "$INLOCAL" | ${GREP} "$OUTLOCAL$" >/dev/null ; then
	      if [ ! -z "$USERSTART" ]; then
		${ECHO} "... It appears that $OUTHOST is a virtual domain"
		${ECHO} "    controlled by $USER."
		LIST="$OUTLOCAL"
	        if [ ! -z "$HOSTMATCH" ]; then
		  ${ECHO} "   This part of the setup appears correct."
	        fi
	      else
	   	${ECHO}
		${ECHO} "$EZWARN $1/inlocal ends with the contents"
		${ECHO} "    of $1/outlocal, but does not start with"
		${ECHO} "    $USER. If this message persists when you"
		${ECHO} "    run this program from $1/editor,"
		${ECHO} "    there is a setup error."
	      fi
	    else
	      ${ECHO}
	      ${ECHO} "$EZWARN $1/inlocal does not end with the contents"
	      ${ECHO} "    of $1/outlocal. This is almost always wrong."
	    fi
          fi
	  if [ ! -r "$1/editor" ]; then
	      ${ECHO} "$EZERR $1/editor does not exist"
	  else
	    ${ECHO}
	    ${ECHO} "$1/editor:"
	    ${ECHO} "============================"
	    ${CAT} "$1/editor"
	    ${ECHO} "============================"
	    ${ECHO}
	  fi
	  if [ ! -r "$1/manager" ]; then
	      ${ECHO} "$EZERR $1/manager does not exist"
	  else
	    ${ECHO} "$1/manager:"
	    ${ECHO} "============================"
	    ${CAT} "$1/manager"
	    ${ECHO} "============================"
	    ${ECHO}
	  fi
	  if [ ! -r "$1/bouncer" ]; then
	      ${ECHO} "$EZERR $1/bouncer does not exist"
	  else
	    ${ECHO} "$1/bouncer:"
	    ${ECHO} "============================"
	    ${CAT} "$1/bouncer"
	    ${ECHO} "============================"
	    ${ECHO}
	  fi
	  if [ ! -r "$1/owner" ]; then
	      ${ECHO} "$EZERR $1/owner does not exist"
	  else
	    ${ECHO} "$1/owner:"
	    ${ECHO} "============================"
	    ${CAT} "$1/owner"
	    ${ECHO} "============================"
	    ${ECHO}
	    OWNER=`${GREP} "@" < $1/owner`
	  fi
	  if [ ! -r "$1/headeradd" ]; then
	      ${ECHO} "$EZERR $1/headeradd does not exist"
	  else
	    ${ECHO} "$1/headeradd:"
	    ${ECHO} "============================"
	    ${CAT} "$1/headeradd"
	    ${ECHO} "============================"
	    ${ECHO}
	  fi
	  if [ ! -r "$1/headerremove" ]; then
	      ${ECHO} "$EZERR $1/headerremove does not exist"
	  else
	    ${ECHO} "$1/headerremove:"
	    ${ECHO} "============================"
	    ${CAT} "$1/headerremove"
	    ${ECHO} "============================"
	    ${ECHO}
	  fi
	  ${ECHO} "---------------------------------------------------"
	  ${ECHO} "Checking standard options:"
	  ${ECHO}
	  if [ -r "$1/public" ]; then
	    ${ECHO} "... public"
	  else
	    ${ECHO} "... not public"
	  fi
	  if [ -r "$1/archived" ]; then
	    FLAGARCH='1'
	    ${ECHO} "... archived"
	  else
	    ${ECHO} "... not archived"
	  fi
	  if [ -r "$1/indexed" ]; then
	    FLAGARCH='1'
	    FLAGIND='1'
	    ${ECHO} "... indexed"
	  else
	    ${ECHO} "... not indexed"
	  fi
	  if [ ! -z "$FLAGARCH" ]; then
            if [ ! -d "$1/archive" ]; then
	      ${ECHO} "$EZERR $1/archive is not a directory"
	    else
	      if [ ! -z "$NUM" -a ! -r "$1/archive/0/index" \
			-a ! -z "$FLAGIND" ]; then
	        ${ECHO} "$EZWARN list is archived, but there is no index."
	        ${ECHO} "    Please run ezmlm-idx!"
	      fi
            fi
          fi
          if [ ! -d "$1/bounce" ]; then
	    ${ECHO} "$EZERR $1/bounce is not a directory"
	  fi
	  if [ -r "$1/prefix" ]; then
	    PREFIX=`${HEAD} -1 "$1/prefix"`
	    ${ECHO} "... using $1/prefix as subject prefix: $PREFIX"
	    ${ECHO}
	  fi
	  if [ -r "$1/sublist" ]; then
	    ${ECHO} "... this is a sublist for:"
	    ${HEAD} -1 < "$1/sublist"
	  else
	    ${ECHO} "... not a sublist"
	  fi
          if [ ! -d "$1/text" ]; then
	    ${ECHO} "$EZERR $1/text is not a directory"
	  fi
	  ${ECHO} "... Contents of $1/text not checked"
	  ${ECHO}
	  if [ ! -z "$OWNER" ]; then
	    ${ECHO} "... Mail to owner goes to: $OWNER"
	  else
	    ${ECHO} "$EZWARN Mail to owner seems not to be forwarded."
	    ${ECHO} "    Remember to check the mailbox once in a while!"
	  fi
	  ${ECHO}
	  ${ECHO} "--------------------------------------------------"
	  ${ECHO}
	  ${ECHO} "... Links should be:"
	  ${ECHO} "    ~/.qmail-{list} -> $1/editor"
	  ${ECHO} "    ~/.qmail-{list}-default -> $1/manager"
	  ${ECHO} "    ~/.qmail-{list}-owner -> $1/owner"
	  ${ECHO} "    ~/.qmail-{list}-return-default -> $1/bouncer"
	  if [ ! -z "$LIST" -a ! -z "$INLOCALOK" ]; then
	    ${ECHO}
	    ${ECHO} "    As far as I can see, '{list}' should be '$LIST'."
	    ${ECHO} "    If so and if .qmail files should be in $HOME ..."
	    BN="$HOME/.qmail-$LIST"
	    FN="$BN"
	    if ${LLS} "$FN" 2>/dev/null | ${GREP} "$1/editor$" >/dev/null ; then
	      ${ECHO} "      $FN is OK"
	    else
	      ${ECHO} "???    $FN is BAD"
	    fi
	    FN="$BN-default"
	    if ${LLS} "$FN" | ${GREP} "$1/manager$" >/dev/null ; then
	      ${ECHO} "      $FN OK"
	    else
	      ${ECHO} "???    $FN is BAD"
	    fi
	    FN="$BN-owner"
	    if ${LLS} "$FN" | ${GREP} "$1/owner$" >/dev/null ; then
	      ${ECHO} "      $FN is OK"
	    else
	      ${ECHO} "???    $FN is BAD"
	    fi
	    FN="$BN-return-default"
	    if ${LLS} "$FN" | ${GREP} "$1/bouncer$" >/dev/null ; then
	      ${ECHO} "      $FN is OK"
	    else
	      ${ECHO} "???    $FN is BAD"
	    fi
	  fi
	  ${ECHO} "--------------------------------------------------"
	  ${ECHO} "Checking subscribers:"
	  ${ECHO}
          if [ ! -d "$1/subscribers" ]; then
	    ${ECHO} "$EZERR $1/subscribers is not a directory"
	  else
            if [ ! -x "$EZLIST" ]; then
	      ${ECHO} "$EZLIST is not available for listing"
	    else
	      if [ "$listsubscribers" = "yes" ] ; then
		${ECHO} "... Subscribers are:"
                if ${EZLIST} "$1" | ${GREP} '@' ; then
	  	  :
	        else
	          ${ECHO} "$EZWARN no subscribers!"
                fi
	      else
		if ${EZLIST} "$1" | ${GREP} '@' >/dev/null 2>&1 ; then
		  ${ECHO} "... There are subscribers."
		else
		  ${ECHO} "$EZWARN no subscribers!"
		fi
	      fi
            fi
	  fi
	  ${ECHO}
	  ${ECHO} "--------------------------------------------------"
	  ${ECHO} "Checking for digest:"
	  ${ECHO}
	  if ${GREP} 'ezmlm-tstdig' < $1/editor >/dev/null 2>&1 ; then
            if ${GREP} -1 'ezmlm-tstdig' < $1/editor \
		| ${TAIL} -1 | ${GREP} 'ezmlm-get' >/dev/null; then
	      ${ECHO} "... integrated digest via $1/editor"
	      DIGEST='1'
	      ${ECHO}
	    fi
	  fi
	  if [ -z "$DIGEST" ]; then
	    ${ECHO} "... no digest via $1/editor"
	  else
	    ${ECHO} "... links should be:"
	    ${ECHO} "    ~/.qmail-{list}-digest-return-default -> $1/bouncer"
	    ${ECHO} "    ~/.qmail-{list}-digest-owner -> $1/owner"
	    if [ ! -z "$LIST" -a ! -z "$INLOCALOK" ]; then
	      ${ECHO}
	      ${ECHO} "    As far as I can see, '{list}' should be '$LIST'."
	      ${ECHO} "    If so and if .qmail files should be in $HOME ..."
	      BN="$HOME/.qmail-$LIST"
	      FN="$BN-digest-return-default"
	      if ${LLS} "$FN" 2>/dev/null | \
			${GREP} "$1/bouncer$" >/dev/null ; then
		${ECHO} "      $FN is OK"
	      else
		${ECHO} "???    $FN is BAD"
	      fi
	      FN="$BN-digest-owner"
	      if ${LLS} "$FN" 2>/dev/null | \
			${GREP} "$1/owner$" >/dev/null ; then
		${ECHO} "      $FN is OK"
	      else
		${ECHO} "???    $FN is BAD"
	      fi
	    fi
	  fi
	  if [ -d "$1/digest" ]; then
	    if [ ! -d "$1/digest/subscribers" ]; then
	      ${ECHO} "$EZERR $1/digest exists, but $1/digest/subscribers"
	      ${ECHO} "$EZERR is not a directory"
	      ${ECHO}
	    else
	      if [ ! -x "$EZLIST" ]; then
  	        ${ECHO} "$EZLIST is not available for listing"
	      else
	        if [ "$listsubscribers" = "yes" ] ; then
		  ${ECHO}
		  ${ECHO} "... Digest subscribers are:"
                  if ${EZLIST} "$1" | ${GREP} '@' ; then
	  	    :
	          else
	            ${ECHO} "$EZWARN no subscribers!"
                  fi
	        else
		  if ${EZLIST} "$1" | ${GREP} '@' >/dev/null 2>&1 ; then
		    ${ECHO} "... There are digest subscribers."
		  else
		    ${ECHO} "$EZWARN no subscribers!"
		  fi
	        fi
              fi
	      ${ECHO}
            fi
	  fi
	  ${ECHO} "---------------------------------------------------"
	  ${ECHO} "Checking for subscription moderation/remote admin:"
	  ${ECHO}
	  if [ -r "$1/remote" ]; then
	    FLAGMOD='1'
	    ${ECHO} "... set up for remote administration"
	    REMOTE=`${CAT} "$1/remote"| ${HEAD} -1`
            if ${ECHO} "$REMOTE" | ${GREP} -G "^/" >/dev/null 2>&1
              then
	        MODDIR="$REMOTE"
	      else
	        MODDIR="$1/mod"
	    fi
	    REMOTE='1'
	  else
	    ${ECHO} "... no remote admin"
          fi
	  if [ -r "$1/modsub" ]; then
	    FLAGMOD='1'
	    ${ECHO} "... subscription moderated"
	    MODSUB=`${CAT} "$1/modsub"| ${HEAD} -1`
            if ${ECHO} "$MODSUB" | ${GREP} -G "^/" >/dev/null 2>&1
	    then
              MODDIR="$MODSUB"
	    elif [ -z "$MODDIR" ]; then
	      MODDIR="$1/mod"
	    fi
	  else
	    ${ECHO} "... no subscription moderation"
	  fi
	  if [ "$FLAGMOD" = '1' ]; then
	    ${ECHO}
	    ${ECHO} "Mods/remote admins stored based in $MODDIR:"
	    ${ECHO}
	    if [ ! -d "$MODDIR" ]; then
	      ${ECHO} "$EZERR moderator dir $MODDIR doesn't exist!"
	    elif [ -e "$MODDIR/lock" -a ! -w "$MODDIR/lock" ]; then
	      ${ECHO} "$EZERR $MODDIR/lock is not writable to user $USER"
	    elif [ ! -x "$EZLIST" ]; then
	      ${ECHO} "${EZLIST} not available for listing"
	    else
	      if ${EZLIST} "$MODDIR" | ${GREP} '@' ; then
		  :
	      else
	          ${ECHO} "$EZERR no subscription moderators/remote admins!"
	      fi
	    fi
	    ${ECHO}
	  fi
	  ${ECHO} "---------------------------------------------------"
	  ${ECHO} "Checking for message moderation:"
	  ${ECHO}
	  if ${GREP} 'ezmlm-gate' < "$1/editor" > /dev/null 2>&1; then
            GATE='1'
	  fi
	  if ${GREP} 'ezmlm-store' < "$1/editor" > /dev/null 2>&1; then
            STORE='1'
	  fi
	  FLAGMOD=''
	  if [ -r "$1/modpost" ]; then
	    FLAGMOD='1'
	    MODPOST=`${CAT} "$1/modpost" | ${HEAD} -1`
            if ${ECHO} "$MODPOST" | ${GREP} -G "^/" >/dev/null 2>&1
              then
	      MODDIR="$MODPOST"
	    else
	      MODDIR="$1/mod"
	    fi
	  fi
	  if [ "$STORE" = '1' -a -z "$FLAGMOD" ]; then
	      ${ECHO} "??? it looks from $1/editor like the list is set up"
	      ${ECHO} "    for message moderation. However, since $1/modpost"
              ${ECHO} "    doesn't exist, ezmlm-store posts them directly. If"
              ${ECHO} "    this is not intended, please create $1/modpost."
	      ${ECHO}
	      FLAGMOD='1'
	      MODDIR="$1/mod"
	  elif [ -z "$STORE" -a -z "$GATE" -a "$FLAGMOD" = '1' ]; then 
	      ${ECHO} "??? $1/modpost exists, leading me to think you'd like"
	      ${ECHO} "    message moderation, but I can't find any call to"
	      ${ECHO} "    ezmlm-store in $1/editor."
	      ${ECHO}
	  elif [ -z "$STORE" -a "$GATE" = '1' ]; then
	      if [ -z "$FLAGMOD" ]; then
		${ECHO} "??? The list is set up with ezmlm-gate in $1/editor."
		${ECHO} "    However, since $1/modpost does not exist all"
		${ECHO} "    messages will be accepted!"
	        FLAGMOD='1'
	        MODDIR="$1/mod"
	      else
		${ECHO} "... The list is set up with ezmlm-gate in $1/editor."
		${ECHO} "    Since $1/modpost exists, subscriber messages"
		${ECHO} "    will be accepted and others will be send for"
		${ECHO} "    moderation."
	      fi
	  fi
	  if [ "$FLAGMOD" = '1' ]; then
	    ${ECHO} "... message moderated"
	    ${ECHO}
	    ${ECHO} "Message moderators based in $MODDIR:"
	    ${ECHO}
	    if [ ! -d "$MODDIR" ]; then
	      ${ECHO} "$EZERR moderator dir $MODDIR doesn't exist!"
	    elif [ -e "$MODDIR/lock" -a ! -w "$MODDIR/lock" ]; then
	      ${ECHO} "$EZERR $MODDIR/lock is not writable to user $USER"
	    elif [ ! -x "$EZLIST" ]; then
	      ${ECHO} "${EZLIST} not available for listing"
	    else
	      if ${EZLIST} "$MODDIR" | ${GREP} '@' ; then
		  :
	      else
	          ${ECHO} "$EZERR no message moderators!"
	      fi
	    fi
	    ${ECHO}
	    MT="120"
	    if [ -r "$1/modtime" ]; then
	      MODTIME=`${CAT} "$1/modtime" | ${HEAD} -1`
	      if [ "$MODTIME" -eq 0 ]; then
		MT="120"
	      elif [ "$MODTIME" -lt 24 ]; then
		MT="24"
	      elif [ "$MODTIME" -gt 240 ]; then
		MT="240"
	      else
		MT="${MODTIME}"
	      fi
	    fi
	    ${ECHO} "... Messages awaiting moderation time out after $MT hours"
	    if [ ! -d "$1/mod/pending" ]; then
	      ${ECHO} "$EZERR $MODDIR/pending is not a directory"
	    else
	      MODNUM=`${LS} "$1/mod/pending" | ${GREP} -c '*'`
	      ${ECHO} "... there are $MODNUM messages awaiting moderator action"
	    fi
	    if [ ! -d "$1/mod/accepted" ]; then
	      ${ECHO} "$EZERR $MODDIR/accepted is not a directory"
	    fi
	    if [ ! -d "$1/mod/rejected" ]; then
	      ${ECHO} "$EZERR $MODDIR/rejected is not a directory"
	    fi
	    if [ ! -r "$1/moderator" ]; then
	      ${ECHO} "$EZERR $1/moderator is not readable to user $USER"
	    else
	      if ${GREP} 'ezmlm-moderate' < "$1/moderator" >/dev/null 2>&1
	      then
	        :
	      else
	        ${ECHO} "$EZERR $1/moderator lacks ezmlm-moderate entry"
	      fi
	      ${ECHO}
	      ${ECHO} "$1/moderator:"
	      ${ECHO} "============================"
	      ${CAT} "$1/moderator"
	      ${ECHO} "============================"
	      ${ECHO} 
	    fi
	    ${ECHO}
	    ${ECHO} "... Links should be:"
	    ${ECHO} "    ~/.qmail-{list}-accept-default -> $1/moderator"
	    ${ECHO} "    ~/.qmail-{list}-reject-default -> $1/moderator"
	    ${ECHO}
	    if [ ! -z "$LIST" -a ! -z "$INLOCALOK" ]; then
	      ${ECHO}
	      ${ECHO} "    As far as I can see, '{list}' should be '$LIST'."
	      ${ECHO} "    If so and if .qmail files should be in $HOME ..."
	      BN="$HOME/.qmail-$LIST"
	      FN="$BN-accept-default"
	      if ${LLS} "$FN" 2>/dev/null | \
			${GREP} "$1/moderator$" >/dev/null ; then
		${ECHO} "      $FN is OK"
	      else
		${ECHO} "???    $FN is BAD"
	      fi
	      FN="$BN-reject-default"
	      if ${LLS} "$FN" 2>/dev/null | \
			${GREP} "$1/moderator$" >/dev/null ; then
		${ECHO} "      $FN is OK"
	      else
		${ECHO} "???    $FN is BAD"
	      fi
	    fi
          else
	    ${ECHO} "... no message moderation"
          fi   
	  ${ECHO}
	  ${ECHO} "---------------------------------------------------"
	  ${ECHO} "Checking for SENDER checks:"
	  ${ECHO}
	  if ${GREP} 'ezmlm-issubn -n' < "$1/editor" >/dev/null 2>&1 ; then
	    ${ECHO} "... Some type of blacklisting in use"
	    SRESTRICT='1'
 	  fi
	  if ${GREP} 'ezmlm-issubn' < "$1/editor" |\
	    ${GREP} -v -- '-n' >/dev/null 2>&1 ; then
	    ${ECHO} "... Some type of SENDER check in use for posts"
	    SRESTRICT='1'
	  fi
	  if [ -z "$SRESTRICT" ]; then
	    ${ECHO} "... no SENDER restrictions found for posts"
	  fi
	  ${ECHO}
	  GET=` ${GREP} 'ezmlm-get' < "$1/manager" | \
		${CUT} -d' ' -f2- | ${CUT} -d\' -f1`
	  if ${ECHO} "$GET" | ${GREP} 's' >/dev/null ; then
	    ${ECHO} "... Only subscribers may access the archive"
	  else
	    ${ECHO} "... no SENDER restrictions for archive access"
	  fi
	  ${ECHO}
	  ${ECHO} "---------------------------------------------------"
	  ${ECHO} "Checking for special options:"
	  ${ECHO}
	  MANAGE=` ${GREP} 'ezmlm-manage' < "$1/manager" | \
		${CUT} -d' ' -f2- | ${CUT} -d\' -f1`
	  if ${ECHO} "$MANAGE" | ${GREP} 'e' >/dev/null ; then
	    ${ECHO} "... remote editing of $1/text/ files enabled"
	    SPEC='1'
	  fi
	  if ${ECHO} "$MANAGE" | ${GREP} 'l' >/dev/null ; then
	    ${ECHO} "... remote listing of subscribers enabled"
	    SPEC='1'
	  fi
	  if [ "$SPEC" = '1' -a -z "$REMOTE" ] ; then
	    ${ECHO} \
	      "$EZERR but remote admin is not enabled, so this will not work!"
	  fi
	  ${ECHO} "---------------------------------------------------"
	fi
	${ECHO}
	${ECHO} "EXT ='$EXT'"
	${ECHO} "EXT1 ='$EXT1'"
	${ECHO} "EXT2 ='$EXT2'"
	${ECHO} "EXT3 ='$EXT3'"
	${ECHO} "EXT4 ='$EXT4'"
	${ECHO} "DTLINE = $DTLINE"
	${ECHO} "RPLINE = $RPLINE"
	${ECHO} "UFLINE = $UFLINE"
	${ECHO} "---------------------------------------------------"
	${ECHO}
	${ECHO} "Hope that helps!"
) | "$RCP" || exit 100

exit 99

