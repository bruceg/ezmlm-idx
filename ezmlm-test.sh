# Above should have been added a QMPATH definition
# DEBUG=1
# This program is meant to test ezmlm-idx.
# Several criteria have to be met for this to work:
# 0. you need to have qmail installed allowing mail delivery to
#    "eztest@host" and subaddresses.
# 2. you need to have a user "eztest" or edit the script to change to a
#	user that exists on your system.
# 3. you need to be that user when you invoke this script and the script
#	needs to reside in the same dir as the binaries you want to test.
# 4. this user needs to have execute permission for the build dir
#
# The script will use ~eztest/__TSTDIR and ~eztest/__TSTDIR__err and destroy
# any files therein. Both user name and directory names etc can be configured
# below, but this should only very rarely be necessary.
#
# This program is experimental and not yet properly documented. Please
# send comments to lindberg@id.wustl.edu. I'm attempting to make this a
# rigorous test for future ezmlm+idx installations so that full function
# can be verified when upgrading to a new version.

# must be absolute
EZBIN=`pwd`

# the user that should run the scripts (test list is created in this
# users home directory).
EZTEST='eztest'

# HOST is normally set from /var/qmail/control/me. Set it here to override
HOST=''

# Version of ezmlm-idx for which this test script is designed
OLD_VER='ezmlm-idx-031'		# ezmlm-idx-0.31x
VER='ezmlm-idx-0.40'
EZVER='40'			# default version

#does the current version have qmail<1.02 support?
OLD_QMAIL='n'

# basedir for test list. It and all files therein are destroyed by
# the script
DIR="$HOME/__TSTDIR"

# part that follows user name of local name of the list
LIST='__tstlist'

# file not within listdir where some error output is stored. If something
# fails, this file may have more info.
ERR="${DIR}__err"

# file that can hold crated test msg to avoid sigpipe
TMP="${DIR}/__tmp"

# defaults for SQL version - overridden by command line switches
TABLE='list'
DB='ezmlm'
SQLHOST=''
SQLUSR="$EZTEST"	# -u overrides, -l overrides that

# sets umask so that qmail paternalism doesn't complain. With most
# installations, there is a umask in the user's profile. 022 should be ok,
# but 002 is safer.

UMASK='umask 002'

#programs:
# to report disk usage summary
DU='du -s'
# Need full path in case qmail doesn't have these in the path
ECHO=`which echo` 2>/dev/null || ECHO='echo'
GREP=`which grep` 2>/dev/null || GREP='grep'
# std programs
HEAD='head'
MKDIR='mkdir'
MV='mv'
# a ps command that would list qmail if running. This works for RedHat Linux
PS='ps auxw'
RM='rm'
SED='sed'
STRINGS='strings'
TAIL='tail'
UNSET='unset'
WC='wc'
# if you don't have this, you can put 'echo "user"' where user is the current
# login user name.
WHOAMI='whoami'

###################### END CONFIRGRABLE ITEMS #########################
if ${ECHO} -n | grep n > /dev/null 2>&1; then
	prompt() {
		${ECHO} "$*\c"
	}
else
	prompt() {
		${ECHO} -n "$*"
	}
fi
SQLUSER=''	# must be empty
ARR='-------------------->'
ALLOW='allow'
DENY='deny'
DASH='-'
# switch for ezmlm-return
DLC='-d'
DUC='-D'
# file for ezmlm-request testing
REQ="${DIR}/__req"
# Set if we've found bug from old version
BUG=''
# Use RDBMS, set if -p was specified even if PW empty, e.g. Postgres
USESQL=''
# process arguments

SECT="1"
while [ ! -z "$1" ]; do		# not everyone has getopt :-(
	case "$1" in
		-/)
			DASH='-/'; shift;;
		-d)
			DB="$2"; shift; shift;;
		-h)
			SQLHOST="$2"; shift; shift;;
		-l)
			SQLUSER="$2"; shift; shift;;
		-n)
			QMVER=n; shift;;
		-o)
			QMVER=o; shift;;
		-p)
			PW="$2"; USESQL=1; shift; shift;;
		-u)
			EZTEST="$2"; SQLUSR="$2"; shift; shift;;
		-s)
			SECT="$2"; shift; shift;;
		-t)
			TABLE="$2"; shift; shift;;
		--)
			shift; break;;
		*)
			${ECHO} "$i"
			${ECHO} "Usage: ezmlm-test [-/] [-on] [-u user]" \
				"[-p sqlpassword [-l sqluser] [-d sqldb]" \
				"[-t sqltable] [-h sqlhost]] [-s section]"
			${ECHO}
			${ECHO} "args have to be separated from switches!"
			${ECHO}
			${ECHO} "defaults: -d ezmlm"
			${ECHO} "          -h [empty => RDBMS default]"
			${ECHO} "          -l eztest or -u arg if specified"
			${ECHO} "          -n/o [autodetected]"
			${ECHO} "          -p [empty - don't use SQL support]"
			${ECHO} "          -s 1 [run test from beginning]"
			${ECHO} "          -t list"
			${ECHO} "          -u eztest"
			${ECHO}
			exit 99;;
	esac
done

if [ -z "$SQLUSER" ]; then
	SQLUSER="$SQLUSR"
fi

USER=`${WHOAMI}` >/dev/null 2>&1 || \
	{ ${ECHO} "whoami doesn't work. If you're not \"${EZTEST}\" the";
	  ${ECHO} "will fail."; USER="${EZTEST}"; }

if [ "$USER" != "${EZTEST}" ]; then 
  ${ECHO} "Must be user ${EZTEST} to execute"; exit 99
fi
LOC="$EZTEST-$LIST"
# calculate position in LOCAL where [normally] default starts
LOCLEN=`${ECHO} "$LOC-" | ${WC} -c | ${SED} 's/ //g'`
REJLEN=`${ECHO} "$LOC-reject-" | ${WC} -c | ${SED} 's/ //g'`
ACCLEN=`${ECHO} "$LOC-accept-" | ${WC} -c | ${SED} 's/ //g'`

${UMASK} >/dev/null || \
      {
	${ECHO} "Umask failed. Usually, this is OK. Fix this if ezmlm-test"
	${ECHO} "fails and messages remain queued with the qmail error"
	${ECHO} "'Uh-oh: .qmail file is writable'."
	${ECHO}
      }

DOT="$HOME/.qmail$DASH$LIST"

if [ "$SECT" != "1" ]; then
	${ECHO} "Starting with section $SECT ..."
fi
 
# test addresses. These are all within the list dir and all addresses
# are subaddresses of the list address. You can change this, but it is
# usually pointless.
SINK='sink'
SINKDIR="${DIR}/${SINK}"
SND="${LOC}-${SINK}"

# moddir
MODACC='modacc'
MODDIR="${DIR}/${MODACC}"
MOD="$LOC-${MODACC}"

# digdir
DIGGG='dig'
DIGDIR="${DIR}/${DIGGG}"
DIG="$LOC-${DIGGG}"

# mandir
MANAG="man"
MANDIR="${DIR}/${MANAG}"
MAN="$LOC-${MANAG}"

# bouncedir
BOUNCE='bnc'
BNC="$LOC-bnc"

if [ -z "$HOST" ]; then
  HOST=`${HEAD} -1 ${QMPATH}/control/me` || \
	{ ${ECHO} "unable to get host name. Set HOST in script" ; exit 99; }
fi

if [ ! -x "${EZBIN}/ezmlm-make" ]; then
	${ECHO} "can't execute ${EZBIN}/ezmlm-make. Most likely, user ``$USER''"
	${ECHO} "doesn't have execute permission to files in directory"
	${ECHO} "``${EZBIN}''. Adjust permissions or edit the script to"
	${ECHO} "use a different test user."
	exit 99
fi

if [ ! -x "${QMPATH}/bin/qmail-local" ]; then
	${ECHO} "can't find qmail-local. Please correct the path in the script"
	exit 99
fi
if [ ! -x "${QMPATH}/bin/qmail-inject" ]; then
	${ECHO} "can't find qmail-inject. Please correct the path in the script"
	exit 99
fi
# Check if qmail is running. Don't fail if not, as it's most likely a
# ps issue and not a lack of qmail.
${PS} | ${GREP} qmai\l-send >/dev/null 2>&1 || \
	{
	 ${ECHO} "qmail isn't running or ps doesn't work as expected. If"
	 ${ECHO} "qmail is not running, this script will fail. Will continue..."
	}

###############################
# message generating function #
###############################
make_body()
{
  ${ECHO} "This is a simple message body"
  ${ECHO} "--bound123ary"
  ${ECHO} "Content-type: Text/pLAIn"
  ${ECHO}
  ${ECHO} "plain text"
  ${ECHO} "--bound123ary"
  ${ECHO} "Content-type: texT/Html"
  ${ECHO}
  ${ECHO} "html text"
  ${ECHO} "--bound123ary--"
  ${ECHO}
  ${ECHO} "junk after boundary"
  return 0
}

make_message()
{
  ${ECHO} "ReCEIved: #LAST#"
  ${ECHO} "ReCeIved: #PENULTIMATE#"
  ${ECHO} "retuRN-RECeipt-to: nobody"
  ${ECHO} "To: $TO"
  ${ECHO} "CC: "
  ${ECHO} " $CC"
  ${ECHO} "FROM: $FROM"
  if [ ! -z "$CONTENT" ]; then
	${ECHO} "MIME-Version: 1.0"
	${ECHO} "Content-type: $CONTENT;"
	${ECHO} " boundary=bound123ary${AFTERBOUND}"
  fi
  if [ ! -z "$SUBJECT" ]; then
	${ECHO} "Subject: $SUBJECT"
  fi
  ${ECHO}
  make_body
  return 0
}

############################
# function to test testmsg #
############################
send_test()
{
  {
	${ECHO} "X-num: #TSTMSG$1#"
	${ECHO} "To: ${SND}@$HOST"
  } | ${QMPATH}/bin/qmail-inject
  return 0
}

############################
# sleeping 5 secs function #
############################
sleep_5()
{
  sleep 1; prompt "."; sleep 1; prompt "."
  sleep 1; prompt "."; sleep 1; prompt "."
  sleep 1; prompt "${1}"
  return 0
}

################################
# waiting for delivery fuction #
################################
wait_test()
{
prompt "max 35s for delivery: "
sleep_5 5s
TSTMSG=`${GREP} -l "#TSTMSG$1" $SINKDIR/new/* 2>/dev/null`
if [ -z "$TSTMSG" ]; then
  sleep_5 10s
  TSTMSG=`${GREP} -l "#TSTMSG$1" $SINKDIR/new/* 2>/dev/null`
  if [ -z "$TSTMSG" ]; then
    sleep_5 15s
    TSTMSG=`${GREP} -l "#TSTMSG$1" $SINKDIR/new/* 2>/dev/null`
    if [ -z "$TSTMSG" ]; then
      sleep_5 20s
      TSTMSG=`${GREP} -l "#TSTMSG$1" $SINKDIR/new/* 2>/dev/null`
      if [ -z "$TSTMSG" ]; then
        sleep_5 25s
        TSTMSG=`${GREP} -l "#TSTMSG$1" $SINKDIR/new/* 2>/dev/null`
        if [ -z "$TSTMSG" ]; then
          sleep_5 30s
          TSTMSG=`${GREP} -l "#TSTMSG$1" $SINKDIR/new/* 2>/dev/null`
          if [ -z "$TSTMSG" ]; then
  		${ECHO}
		${ECHO} "Delivery of test message failed. Fix qmail or wait"
		${ECHO} "longer and continue with argument \"-s $1\""
 		exit 100
          fi
        fi
      fi
    fi
  fi
fi
sleep_5 OK
${ECHO}
${RM} -f "$TSTMSG" > "${ERR}" 2>&1 || \
	{ ${ECHO} "failed to remove test message for section $1"; exit 100; }
TSTMSG=''
return 0
}

########################
# remove old test list #
########################
if [ "$SECT" = "1" ]; then
  if [ $USESQL ]; then
	${EZBIN}/ezmlm-unsub "${DIR}/digest" "${MAN}@$HOST" "${DIG}@$HOST" \
		>"${ERR}" 2>&1
	${EZBIN}/ezmlm-unsub "${DIR}/mod" "${MOD}@$HOST" \
		>"${ERR}" 2>&1
	${EZBIN}/ezmlm-unsub "${DIR}/${ALLOW}" "aaa@bbb" "ccc@ddd" "eee@fff" \
		>"${ERR}" 2>&1
  fi
  ${RM} -rf "${DIR}" ${DOT}* "${ERR}" >/dev/null 2>&1
fi
${ECHO}

#################
# check version #
#################

# assume ezmlm-idx and ezmlm-test are different versions
SAME_VER='n'

TMP_VER=`${EZBIN}/ezmlm-make -V 2>&1` || \
	{
		${ECHO} "This program only works with ezmlm-idx"
		exit 100
	}
# ezmlm-idx-0313 does not support DEFAULT, so no sense testing it
THIS_VER=`${ECHO} "$TMP_VER" | cut -d'+' -f2`
${ECHO} "$THIS_VER" | ${GREP} "ezmlm-idx" > /dev/null 2>&1 ||
  THIS_VER=`${ECHO} "$TMP_VER" | cut -d' ' -f4`

${ECHO} "testing ezmlm-idx:    $THIS_VER"
${ECHO} "Using FQDN host name: $HOST"

${ECHO} "$THIS_VER" | ${GREP} "$OLD_VER" >/dev/null 2>&1
if [ "$?" = "0"	]; then
	DLC=''
	DUC=''
	EZVER='31'
	QMVER='o'
	ALLOW='extra'		# support old nomenclature
	DENY='blacklist'
else
	${ECHO} "$THIS_VER" | ${GREP} "$VER" >/dev/null 2>&1
	if [ "$?" != "0" ]; then
		${ECHO} "Warning: ezmlm-make version is not $VER"
		${ECHO} "         test info may not be reliable"
		${ECHO}
	fi
########## should add testing of From line logging for non-0.31x
	SW_FROM="-f"
fi

# Now see if we support old qmail

  ${ECHO} "$THIS_VER" | ${GREP} "ezmlm-idx-0.32" >/dev/null 2>&1 && \
	EZVER='32'; 

if [ "$EZVER" != '31' -a "$EZVER" != '32' -a "$QMVER" = 'o' ]; then
	${ECHO} "Sorry, this version of ezmlm-idx requires qmail>=1.02"
	exit 100;
fi

if [ "$SECT" = "1" ]; then
##############
# ezmlm-make #
##############
  prompt "ezmlm-make (1/2):     "

# edit non-existant list
  ${EZBIN}/ezmlm-make -e -C${EZBIN}/ezmlmrc "${DIR}" "${DOT}" \
	"$LOC" "$HOST" > /dev/null 2>&1 && \
	{ ${ECHO} "ezmlm-make failed reject edit of non-existing list:"
	  exit 100; }

# make simple test list
  ${EZBIN}/ezmlm-make -C${EZBIN}/ezmlmrc "${DIR}" "${DOT}" \
	"$LOC" "$HOST" || \
	{ ${ECHO} "ezmlm-make failed to create test list"; exit 100; }

# remake simple test list which should fail
  ${EZBIN}/ezmlm-make -C${EZBIN}/ezmlmrc "${DIR}" "${DOT}" \
	"$LOC" "$HOST" >/dev/null 2>&1 && \
	{ ${ECHO} "failed to reject creation of existing list"; exit 100; }

# edit the list (add digest)
  ${EZBIN}/ezmlm-make -+d -C${EZBIN}/ezmlmrc "${DIR}" || \
	{ ${ECHO} "ezmlm-make failed to edit test list"; exit 100; }

# edit the list (add digest)
  ${MV} "${DIR}/config" "${DIR}/config~"
  ${EZBIN}/ezmlm-make -ed -C${EZBIN}/ezmlmrc "${DIR}" "$DOT" "$LOC" "$HOST" \
	>/dev/null 2>&1 || \
	{ ${ECHO} "failed without DIR/config: 0.313 bug, fixed in 0.314."
	  prompt "ezmlm-make ......     "
	  BUG="${BUG} config"
	}
  ${MV} "${DIR}/config~" "${DIR}/config"

  ${GREP} "ezmlm-weed" "${DIR}/bouncer" >/dev/null 2>&1 || \
	{ ${ECHO} "no ezmlm-weed in bouncer"; exit 100; }
  ${GREP} "ezmlm-return" "${DIR}/bouncer" >/dev/null 2>&1 || \
	{ ${ECHO} "no ezmlm-return in bouncer: 0.32 bug, fixed in 0.321."
	  prompt "ezmlm-make ......     "
	  BUG="${BUG} return"
	}
# digest/bouncer only for >=0.32
  if [  "$EZVER" != '31' ]; then
    if [ ! -f "${DIR}/digest/bouncer" ]; then
	${ECHO} "failed to create digest/bouncer"; exit 100;
    fi
    ${GREP} "ezmlm-weed" "${DIR}/digest/bouncer" >/dev/null 2>&1 || \
	{ ${ECHO} "no ezmlm-weed in bouncer"; exit 100; }
    ${GREP} "ezmlm-return" "${DIR}/digest/bouncer" >/dev/null 2>&1 || \
	{ ${ECHO} "no ezmlm-return in digest/bouncer: 0.32 bug, OK in 0.321."
	  prompt "ezmlm-make ......     "
	  BUG="${BUG} return"
	}
  fi
  ${ECHO} "OK"

# Add sql files for sql testing
RDBMS='STD'
prompt "Using RDBMS support:  "
if [ $USESQL ]; then
  ${EZBIN}/ezmlm-make -+6 "$SQLHOST::$SQLUSER:$PW:$DB:$TABLE" \
	-C${EZBIN}/ezmlmrc "${DIR}"|| \
	{ ${ECHO} "ezmlm-make failed to add SQL config info"; exit 100; }

  ${STRINGS} ${EZBIN}/ezmlm-sub | ${GREP} -i 'MySQL' >/dev/null 2>&1 && \
	RDBMS='MySQL'
  ${STRINGS} ${EZBIN}/ezmlm-sub | ${GREP} -i 'libpq.' >/dev/null 2>&1 && \
	RDBMS='PostgreSQL'
  if [ "$RDBMS" = 'STD' ]; then
	${ECHO} "No recognized support. If none, will default to std dbs."
  else
	${ECHO} "$RDBMS. Hope empty tables exist."
  fi

else
	${ECHO} "No."
fi

###############################################################
# set up subscriber/moderator/sender/digest recipient account #
###############################################################
  ${MKDIR} "$SINKDIR" "$SINKDIR/new" "$SINKDIR/cur" "$SINKDIR/tmp" || \
	{ ${ECHO} "mkdir for sinkdir failed"; exit 100; }
  ${ECHO} "${SINKDIR}/" > "$DOT-$SINK"
# link for qmail version testing 
  ${ECHO} '|${ECHO} $DEFAULT >' "${DIR}/default" > "$DOT-$SINK-default"
  ${ECHO} "${SINKDIR}/" >> "$DOT-$SINK-default"

  ${MKDIR} "$MODDIR" "$MODDIR/new" "$MODDIR/cur" "$MODDIR/tmp" || \
	{ ${ECHO} "mkdir for moddir failed"; exit 100; }
  ${ECHO} "${MODDIR}/" > "$DOT-$MODACC"

  ${MKDIR} "$MANDIR" "$MANDIR/new" "$MANDIR/cur" "$MANDIR/tmp" || \
	{ ${ECHO} "mkdir for mandir failed"; exit 100; }
  ${ECHO} "${MANDIR}/" > "$DOT-$MANAG"

  ${MKDIR} "$DIGDIR" "$DIGDIR/new" "$DIGDIR/cur" "$DIGDIR/tmp" || \
	{ ${ECHO} "mkdir for digdir failed"; exit 100; }
  ${ECHO} "${DIGDIR}/" > "$DOT-$DIGGG"

fi

###########################
# determine qmail version #
###########################

if [ "$SECT" != "9" ]; then

${ECHO} "Subject: zzz-test" > "${DIR}/__tmp"
${QMPATH}/bin/qmail-local "$EZTEST" "$HOME" "$SND-zzz" "$DASH" \
		"$LIST-$SINK-zzz" "$HOST" \
		"<>" '' < "${DIR}/__tmp" >"${ERR}" 2>&1 || \
	{ ${ECHO} "-failed to deliver message with qmail-local"; exit 100; }

if [ ! -r "${DIR}/default" ]; then
	${ECHO} "qmail-local failed to deliver the message. Can't determine"
	${ECHO} "qmail version"
	exit 99
fi

if [ `cat "${DIR}/default"` = "zzz" ]; then
	if [ -z "$QMVER" ]; then
		QMVER="n"
	fi
else
	if [ -z "$QMVER" ]; then
		QMVER="o"
	fi
fi

prompt "testing for qmail:    "
if [ "$QMVER" = "n" ]; then
	${ECHO} ">=1.02"
else
	${ECHO} "[any]"
fi

# if testing for old version, make sure DEFAULT is not defined. If printenv
# is not available, we hope for the best and continue. unset should work ...
# Set BADUNSET if unset doesn't do the job

A='a'
export A
${UNSET} A
[ -z "$A" ] || BADUNSET='y'

${UNSET} DEFAULT

if [ "$QMVER" = "o" ]; then
  printenv PATH >/dev/null 2>&1 && \
    printenv DEFAULT > /dev/null 2>&1 && \
      { ${ECHO} "Can't test for old version of qmail if DEFAULT is defined. ";
      ${ECHO} "Please undefine it."; exit 99; }
fi

# correct bouncer for our binaries:
###################################
# NOTE: This is duplicated (and should be) after next ezmlm-make block.
  ${ECHO} "|/${EZBIN}/ezmlm-weed" > "${DIR}/bouncer"
  ${ECHO} "|/${EZBIN}/ezmlm-weed" > "${DIR}/digest/bouncer"
  if [ "$EZVER" = "31" ]; then	# autodetecting bouncer for 0.31x
    ${ECHO} "|/${EZBIN}/ezmlm-return '${DIR}'" >> "${DIR}/bouncer"
    ${ECHO} "|/${EZBIN}/ezmlm-return '${DIR}'" >> "${DIR}/digest/bouncer"
  else				# split bouncer with args for later versions
	# edited for ezmlm-new
    ${ECHO} "|/${EZBIN}/ezmlm-return '${DIR}'" >> "${DIR}/bouncer"
    ${ECHO} "|/${EZBIN}/ezmlm-return '${DIR}'" >> "${DIR}/digest/bouncer"
  fi

# if testing qmail>=1.02, remove inlocal/inhost - shouldn't be used
  if [ "$QMVER" = "n" ]; then
	${RM} -f "${DIR}/inlocal" "${DIR}/inhost" > /dev/null || \
	  { ${ECHO} "failed to remove inlocal/inhost for testlist"; exit 100; }
  fi

###########################
# set up bouncing account #
###########################
  ${ECHO} "|${GREP} 'MAILER-DAEMON' >/dev/null && exit 99" > "$DOT-$BOUNCE"
  ${ECHO} "|exit 100" > "$DOT-$BOUNCE"

fi

###################################################
# account to receive digests and archive excerpts #
###################################################

if [ "$SECT" = "1" ]; then

#####################
# test ezmlm-reject #
#####################
  prompt "ezmlm-reject:         "
  FROM="$EZTEST"
  TO="$EZTEST-__tstlist@$HOST"
  SUBJECT="test"
  CONTENT="multipart/mixed"
  CC="<>"
  BODY=''

# with directory
  make_message | ${EZBIN}/ezmlm-reject "${DIR}" || \
	{ ${ECHO} "failed to accept good message with dir"; \
	exit 100; }
# without directory

  make_message | ${EZBIN}/ezmlm-reject || \
	{ ${ECHO} "failed to accept good message without dir: $?"; \
	exit 100; }

#too small
  ${ECHO} "5000:1000" > "${DIR}/msgsize"
  OUT=`make_message | ${EZBIN}/ezmlm-reject "${DIR}" 2>&1` && \
	{ ${ECHO} "ezmlm-reject failed to reject too small message"; \
		exit 100; }

# just right
  ${ECHO} "500:5" > "${DIR}/msgsize"
  make_message | ${EZBIN}/ezmlm-reject "${DIR}" || \
	{ ${ECHO} "failed to accept message of ok size"; \
	exit 100; }

#too large
  ${ECHO} "20:10" > "${DIR}/msgsize"
  OUT=`make_message | ${EZBIN}/ezmlm-reject "${DIR}" 2>&1` && \
	{ ${ECHO} "ezmlm-reject failed to reject too large message"; \
		exit 100; }

# restore
  ${RM} -f "${DIR}/msgsize"

# without subject
  SUBJECT=''
  OUT=`make_message | ${EZBIN}/ezmlm-reject "${DIR}" 2>&1` && \
	{ ${ECHO} "ezmlm-reject failed to reject message without subject"; \
		exit 100; }
  OUT=`make_message | ${EZBIN}/ezmlm-reject 2>&1` && \
	{ ${ECHO} "ezmlm-reject failed to reject message without subject"; \
		exit 100; }

# with empty subject
  SUBJECT='(NUll)'
  OUT=`make_message | ${EZBIN}/ezmlm-reject "${DIR}" 2>&1` && \
	{ ${ECHO} "ezmlm-reject failed to reject null subject"; \
		exit 100; }
  OUT=`make_message | ${EZBIN}/ezmlm-reject 2>&1` && \
	{ ${ECHO} "ezmlm-reject failed to reject null subject"; \
		exit 100; }

# testing -S
  OUT=`make_message | ${EZBIN}/ezmlm-reject -S "${DIR}"` || \
	{ ${ECHO} "-S switch failed with dir"; exit 100; }
  OUT=`make_message | ${EZBIN}/ezmlm-reject -S ` || \
	{ ${ECHO} "-S switch failed without dir"; exit 100; }

# with command subject
  SUBJECT='REmOVE'
  OUT=`make_message | ${EZBIN}/ezmlm-reject "${DIR}" 2>&1` && \
	{ ${ECHO} "failed to reject command subject with dir"; \
		exit 100; }
  OUT=`make_message | ${EZBIN}/ezmlm-reject 2>&1` && \
	{ ${ECHO} "failed to reject command subject without dir"; \
		exit 100; }

# testing -C
  OUT=`make_message | ${EZBIN}/ezmlm-reject -C "${DIR}"` || \
	{ ${ECHO} "-C switch failed with dir"; exit 100; }
  OUT=`make_message | ${EZBIN}/ezmlm-reject -C ` || \
	{ ${ECHO} "-C switch failed without dir"; exit 100; }

  SUBJECT='test'

# Test with list name in Cc:
  CC="$TO"
  TO="nobody@$HOST"
  OUT=`make_message | ${EZBIN}/ezmlm-reject "${DIR}"` || \
	{ ${ECHO} "failed to accept good Cc: with dir"; \
		exit 100; }
  OUT=`make_message | ${EZBIN}/ezmlm-reject` || \
	{ ${ECHO} "failed to accept good Cc: without dir"; \
		exit 100; }

# Bad To/Cc
  CC="$TO"
  OUT=`make_message "$MESSAGE" | ${EZBIN}/ezmlm-reject "${DIR}" 2>&1` && \
		{ ${ECHO} "failed to reject bad To/Cc with dir"; \
		exit 100; }
  if [ "$?" != "100" ]; then
	${ECHO} "failed to exit 100 on error"; exit 100
  fi
  OUT=`make_message "$MESSAGE" | ${EZBIN}/ezmlm-reject -q "${DIR}" 2>&1` && \
		{ ${ECHO} "failed to reject bad To/Cc with dir"; \
		exit 100; }
  if [ "$?" -ne "99" ]; then
	${ECHO} "-q failed"; exit 100
  fi

# for backwards-compatibility and since we don't know inlocal@inhost without
# dir, ezmlm-reject doesn't check To/Cc when there is no dir
  OUT=`make_message "$MESSAGE" | ${EZBIN}/ezmlm-reject` || \
		{ ${ECHO} "failed to accept bad To/Cc without dir"; \
		exit 100; }

# testing -T
  OUT=`make_message | ${EZBIN}/ezmlm-reject -T "${DIR}"` || \
	{ ${ECHO} "-T switch failed with dir"; exit 100; }
OUT=`make_message | ${EZBIN}/ezmlm-reject -T ` || \
	{ ${ECHO} "-T switch failed without dir"; exit 100; }

# restore good TO
  TO="$EZTEST-__tstlist@$HOST"

# if part is mimereject message should be rejected
  touch "${DIR}"/mimeremove
  ${ECHO} "text/html" > "${DIR}"/mimereject
  OUT=`make_message | ${EZBIN}/ezmlm-reject "${DIR}" 2>&1` && \
	{ ${ECHO} "mimereject failed with dir"; exit 100; }
  OUT=`make_message | ${EZBIN}/ezmlm-reject` || \
	{ ${ECHO} "mimereject without dir"; exit 100; }

# if part is removed ezmlm-reject should not reject
  ${ECHO} "tExt/htMl" > "${DIR}"/mimeremove
  ${ECHO} "" > "${DIR}"/mimereject 
  OUT=`make_message | ${EZBIN}/ezmlm-reject "${DIR}"` || \
	{ ${ECHO} "mimeremove failed with dir"; exit 100; }
  OUT=`make_message | ${EZBIN}/ezmlm-reject` || \
	{ ${ECHO} "mimeremove without dir"; exit 100; }

# test content-type with something after boundary=xxx
  AFTERBOUND=';micalg=pgp-md5'
  ${ECHO} "text/html" > "${DIR}"/mimereject
  OUT=`make_message | ${EZBIN}/ezmlm-reject "${DIR}" 2>&1` && \
	{ ${ECHO} "err with text after boundary: 0.30 bug fixed in 0.322"
	  prompt "ezmlm-reject.......   "
	  BUG="${BUG} reject_bound"
	}

# restore
  ${RM} "${DIR}"/mimereject
  AFTERBOUND=''

# if entire message is mimeremove type is should be rejected
  ${ECHO} "multipart/mixed" > "${DIR}"/mimeremove
  OUT=`make_message | ${EZBIN}/ezmlm-reject "${DIR}" 2>&1` && \
	{ ${ECHO} "mimereject failed with dir"; exit 100; }
  OUT=`make_message | ${EZBIN}/ezmlm-reject` || \
	{ ${ECHO} "mimereject without dir"; exit 100; }

# restore
  ${RM} "${DIR}"/mimeremove

# test headerreject
  ${ECHO} "Content-TYPE" > "${DIR}"/headerreject
  OUT=`make_message | ${EZBIN}/ezmlm-reject -H "${DIR}"` || \
	{ ${ECHO} "headerreject -H failed with dir"; exit 100; }
  OUT=`make_message | ${EZBIN}/ezmlm-reject -h "${DIR}" 2>&1` && \
	{ ${ECHO} "headerreject failed with dir"; exit 100; }
  OUT=`make_message | ${EZBIN}/ezmlm-reject` || \
	{ ${ECHO} "headerreject failed without dir"; exit 100; }
  OUT=`make_message | ${EZBIN}/ezmlm-reject -h 2>&1` && \
	{ ${ECHO} "-h was accepted without dir"; exit 100; }

# Suppress content-type header
  CONTENT=''
  OUT=`make_message | ${EZBIN}/ezmlm-reject "${DIR}"` || \
	{ ${ECHO} "headerreject rejected even though header isn't there"; \
	exit 100; }

  CONTENT='multIpart/mIXed'

  ${ECHO} "OK"
###############################
# ezmlm-sub/unsub/list/issubn #
###############################

  prompt "ezmlm-[un|is]sub[n]:  "

  SENDER="XYZZY@HOst"; export SENDER

# With mysql testing, there may be junk left from earlier testing that
# gives false positives in testing. Make sure it's detected
  ${EZBIN}/ezmlm-list "${DIR}" >/dev/null || \
	{ ${ECHO} "ezmlm-list: failed"; exit 100; }

  ${EZBIN}/ezmlm-list "${DIR}" | ${GREP} '@' >/dev/null 2>&1 && \
	{ ${ECHO} "already addresses in table - please remove and start again";
		exit 100; }

  ${EZBIN}/ezmlm-list "${DIR}/digest" | ${GREP} '@' >/dev/null 2>&1 && \
	{ ${ECHO} "already addresses in table - please remove and start again";
		exit 100; }

  ${EZBIN}/ezmlm-list "${DIR}/${ALLOW}" | ${GREP} '@' >/dev/null 2>&1 && \
	{ ${ECHO} "already addresses in table - please remove and start again";
		exit 100; }

# not subscriber. Test default
  ${EZBIN}/ezmlm-issubn "${DIR}" "${DIR}/${ALLOW}" && \
	{ ${ECHO} "ezmlm-issubn: failed: exit 0 on non-subscriber"; exit 100; }

# not subscriber. Test -n
  ${EZBIN}/ezmlm-issubn -n "${DIR}" "${DIR}/${ALLOW}" || \
	{ ${ECHO} "ezmlm-issubn: -n failed for non-subscriber"; exit 100; }

# add subscriber
  ${EZBIN}/ezmlm-sub "${DIR}" "xyZZy@hoSt" || \
	{ ${ECHO} "ezmlm-sub: failed to add subscriber"; exit 100; }

# is subscriber. Test default
  ${EZBIN}/ezmlm-issubn "${DIR}" "${DIR}/${ALLOW}" || \
	{ ${ECHO} "ezmlm-issubn: failed: exit false for subscriber"; exit 100; }

# is subscriber. Test -n
  ${EZBIN}/ezmlm-issubn -n "${DIR}" "${DIR}/${ALLOW}" && \
	{ ${ECHO} "ezmlm-issubn: -n failed for subscriber"; exit 100; }

# add to allow
  ${EZBIN}/ezmlm-sub "${DIR}/${ALLOW}" "ZZtop@hoSt" || \
	{ ${ECHO} "ezmlm-sub: failed to add address to ${DIR}/${ALLOW}"; exit 100; }

# list subscribers
  ${EZBIN}/ezmlm-list "${DIR}" | ${GREP} "xyZZy" >"${ERR}" 2>&1 || \
	{ ${ECHO} "ezmlm-list: failed to list subscribers"; exit 100; }

# remove subscriber
  ${EZBIN}/ezmlm-unsub "${DIR}" "xYzZy@hOst" || \
	{ ${ECHO} "ezmlm-sub: failed to add subscriber"; exit 100; }

# see that it was removed
  ${EZBIN}/ezmlm-list "${DIR}" | ${GREP} "xyZZy" >"${ERR}" 2>&1 && \
	{ ${ECHO} "ezmlm-unsub: failed to remove subscriber"; exit 100; }

  SENDER="zztop@HOst"; export SENDER

# check for address in allow
  ${EZBIN}/ezmlm-issubn "${DIR}" "${DIR}/${ALLOW}" || \
	{ ${ECHO} "ezmlm-sub/issubn: failed to add/look in 2nd db"; exit 100; }

# remove (multiple) (non)existing addresses from allow
  ${EZBIN}/ezmlm-unsub "${DIR}/${ALLOW}" "xYzZy@hOst" "zZToP@HOSt" || \
	{ ${ECHO} "ezmlm-unsub: failed to remove subscriber"; exit 100; }

# verify removal
  ${EZBIN}/ezmlm-issubn "${DIR}" "${DIR}/${ALLOW}" && \
	{ ${ECHO} "ezmlm-unsub/issubn: failed to remove address"; exit 100; }

# clean up
  LOCAL=''; export LOCAL

  ${ECHO} "OK"
##############
# ezmlm-send #
##############
  prompt "ezmlm-send (1/2):     "

  SENDER="${SND}@$HOST"; export SENDER
  ${EZBIN}/ezmlm-sub "${DIR}" "$SENDER"
# set up prefix
  ${ECHO} "[PFX]" > "${DIR}/prefix"
# set up trailer
  { ${ECHO} "--- TRAILER ---"; ${ECHO}; } > "${DIR}/text/trailer"
# test
  { ${ECHO} "X-num: msg1"; make_message; } | \
	${EZBIN}/ezmlm-send "${DIR}" >"${ERR}" 2>&1 || \
	{ ${ECHO} "failed to accept normal message"; exit 100; }
  if [ `cat "${DIR}/num"` != "1:1" ]; then
	${ECHO} "failed to create num for normal message 1"; exit 100; 
  fi
  if [ ! -x "${DIR}/archive/0/01" ]; then
	{ ${ECHO} "failed to archive normal message"; exit 100; }
  fi
  ${GREP} "1:" "${DIR}/archive/0/index" >/dev/null 2>&1 || \
	{ ${ECHO} "failed to index archive"; exit 100; }

  ${RM} -f "${DIR}/indexed"
  ${RM} -f "${DIR}/archived"

# test to see that trailer is added to nom-mime messages
  CONTENT=''
  { ${ECHO} "X-num: msg5"; make_message; } | \
	${EZBIN}/ezmlm-send "${DIR}" >"${ERR}" 2>&1  || \
	{ ${ECHO} "failed to accept non-mime message"; exit 100; }

# test to see that trailer is suppressed for multipart/signed
  CONTENT='multipart/signed'
  { ${ECHO} "X-num: msg6"; make_message; } | \
	${EZBIN}/ezmlm-send "${DIR}" >"${ERR}" 2>&1  || \
	{ ${ECHO} "failed to accept multipart/signed message"; exit 100; }

# restore
  CONTENT='multipart/mixed'

# test content-type with something after boundary=xxx
  AFTERBOUND=';micalg=pgp-md5'
  ${ECHO} "text/html" > "${DIR}"/mimeremove
  make_message | ${EZBIN}/ezmlm-send "${DIR}" >"${ERR}" 2>&1  || \
	{ ${ECHO} "err with text after boundary: 0.30 bug fixed in 0.322"
	  prompt "ezmlm-send.........   "
	  BUG="${BUG} send_bound"
	}
# restore
  AFTERBOUND=''
  ${ECHO} "1:1" > "${DIR}/num"
  ${RM} "${DIR}"/mimeremove

# -r => don't trim received headers
  { ${ECHO} "X-num: msg2"; make_message; } | \
	${EZBIN}/ezmlm-send -r "${DIR}" >"${ERR}" 2>&1 || \
	{ ${ECHO} "failed to accept normal message 2"; exit 100; }

  ${GREP} "2:" "${DIR}/archive/0/index" >/dev/null 2>&1 && \
	{ ${ECHO} "indexed message with DIR/indexed missing"; exit 100; }
  ${GREP} "msg2" ${DIR}/archive/0/* >/dev/null 2>&1 && \
	{ ${ECHO} "archived message with DIR/archived missing"; exit 100; }

# -C eliminate SENDER from addressees
  { ${ECHO} "X-num: msg3"; make_message; } | \
	${EZBIN}/ezmlm-send -C "${DIR}" >"${ERR}" 2>&1 || \
	{ ${ECHO} "failed to accept normal message 3"; exit 100; }
  ${EZBIN}/ezmlm-unsub "${DIR}" "$SENDER"

# make sure headerremove was done
  ${GREP} -i 'return-receipt-to' < "${DIR}/archive/0/01" >/dev/null &&
	{ ${ECHO} "failed to remove headerremove"; exit 100; }
# test mimeremove
  touch "${DIR}/archived" "${DIR}/indexed"
  ${ECHO} "teXT/hTml" > "${DIR}/mimeremove"
  { ${ECHO} "X-num: msg4"; make_message; } | \
	${EZBIN}/ezmlm-send "${DIR}" >"${ERR}" 2>&1 || \
	{ ${ECHO} "failed to accept mimeremove message"; exit 100; }
  ${GREP} -i 'text/html' < "${DIR}/archive/0/04" >/dev/null &&
	{ ${ECHO} "failed to remove mimeremove part"; exit 100; }

  ${ECHO} "OK"
################
# ezmlm-tstdig #
################
  prompt "ezmlm-tstdig:         "

  ${EZBIN}/ezmlm-tstdig -k2 -m5 -t1 "${DIR}" || \
	{ ${ECHO} "-t1 failed"; exit 100; }
  ${EZBIN}/ezmlm-tstdig -k2 -m5 -t0 "${DIR}" && \
	{ ${ECHO} "-t0 failed"; exit 100; }

  LOCAL="$LOC-xx"; export LOCAL
  if [ "$QMVER" = "n" ]; then
	DEFAULT='xx'; export DEFAULT
  fi
  ${EZBIN}/ezmlm-tstdig -k2 -m5 -t0 "${DIR}" || \
	{ ${ECHO} "problem with -xx in manager position"; exit 100; }
  LOCAL="$LOC-dig."; export LOCAL
  if [ "$QMVER" = "n" ]; then
	DEFAULT='dig.'; export DEFAULT
  fi
  ${EZBIN}/ezmlm-tstdig -k2 -m5 -t0 "${DIR}" && \
	{ ${ECHO} "problem with -dig in manager position"; exit 100; }
  LOCAL="$LOC-digest-"; export LOCAL
  if [ "$QMVER" = "n" ]; then
	DEFAULT='digest-'; export DEFAULT
  fi
  ${EZBIN}/ezmlm-tstdig -k2 -m5 -t0 "${DIR}" || \
	{ ${ECHO} "err with -digest- in mgr pos: 0.31 bug fixed in 0.321"
	  prompt "ezmlm-tstdig.......   "
	  BUG="${BUG} digest"
	}
  LOCAL=''; export LOCAL
  if [ "$QMVER" = "n" ]; then
	unset DEFAULT
  fi
  ${EZBIN}/ezmlm-tstdig -k2 -m4 -t1 "${DIR}" || \
	{ ${ECHO} "-m failed"; exit 100; }
  ${EZBIN}/ezmlm-tstdig -k1 -m5 -t0 "${DIR}" || \
	{ ${ECHO} "-k failed"; exit 100; }
  LOCAL="$LOC"; export LOCAL
  ${EZBIN}/ezmlm-tstdig -k1 -m5 -t0 "${DIR}" > "${ERR}" 2>&1 || \
	{
	 ${ECHO} "problem with DEFAULT unset: 0.32 bug, OK in 0.321."
	 prompt "ezmlm-tstdig.......   "
	  BUG="${BUG} tstdig"
	}
  ${ECHO} "OK"

##############
# ezmlm-weed #
##############

  prompt "ezmlm-weed:           "

  ${ECHO} "Subject: test" | ${EZBIN}/ezmlm-weed || \
	{ ${ECHO} "failed to accept good message"; exit 100; }
  ${ECHO} "Subject: success notice" | ${EZBIN}/ezmlm-weed >/dev/null 2>&1 && \
	{ ${ECHO} "failed to reject bad message"; exit 100; }

  ${ECHO} "OK"

##############
# ezmlm-make #
##############
  prompt "ezmlm-make (2/2):     "

# make sure a few ezmlm-make switches work
  ${EZBIN}/ezmlm-make -+qkgu -C${EZBIN}/ezmlmrc "${DIR}" || \
	{ ${ECHO} "failed to edit test list to +qkgu"; exit 100; }
  ${GREP} "${DENY}" "${DIR}/editor" >/dev/null 2>&1 || \
	{ ${ECHO} "failed to implement -k for list"; exit 100; }
  ${GREP} "ezmlm-request" "${DIR}/manager" >/dev/null 2>&1 || \
	{ ${ECHO} "failed to implement -q for list"; exit 100; }
  ${GREP} "ezmlm-get -s" "${DIR}/manager" >/dev/null 2>&1 || \
	{ ${ECHO} "failed to implement -g for list"; exit 100; }
  ${GREP} "${ALLOW}" "${DIR}/editor" >/dev/null 2>&1 || \
	{ ${ECHO} "failed to implement -u for list"; exit 100; }

  ${EZBIN}/ezmlm-make -+QKGU -C${EZBIN}/ezmlmrc "${DIR}" || \
	{ ${ECHO} "failed to edit test list to +QKGU"; exit 100; }
  ${GREP} "${DENY}" "${DIR}/editor" >/dev/null 2>&1 && \
	{ ${ECHO} "failed to implement -K for list"; exit 100; }
  ${GREP} "ezmlm-request" "${DIR}/manager" >/dev/null 2>&1 && \
	{ ${ECHO} "failed to implement -Q for list"; exit 100; }
  ${GREP} "ezmlm-get -s" "${DIR}/manager" >/dev/null 2>&1 && \
	{ ${ECHO} "failed to implement -G for list"; exit 100; }
  ${GREP} "${ALLOW}" "${DIR}/editor" >/dev/null 2>&1 && \
	{ ${ECHO} "failed to implement -U for list"; exit 100; }

# edit the list (add moderation and remove admin)
  ${EZBIN}/ezmlm-make -+rsm -C${EZBIN}/ezmlmrc "${DIR}" || \
	{ ${ECHO} "failed to edit test list to +rsm"; exit 100; }
# edit the list (add text file editing and list/log)
${EZBIN}/ezmlm-make -+ln -C${EZBIN}/ezmlmrc "${DIR}" || \
	{ ${ECHO} "failed to edit test list to +ln"; exit 100; }

# Now to create our own manager for later tests:

${ECHO} "|${GREP} 'req1' >/dev/null 2>&1 && { ${ECHO} \"\$LOCAL\" >> '${REQ}'; exit 99; }; exit 0" > "${DIR}/manager"
${ECHO} "|${EZBIN}/ezmlm-manage -le ${SW_FROM} '${DIR}'" >> "${DIR}/manager"
${ECHO} "OK"

# correct bouncer for our binaries:
###################################
  ${ECHO} "|/${EZBIN}/ezmlm-weed" > "${DIR}/bouncer"
  ${ECHO} "|/${EZBIN}/ezmlm-weed" > "${DIR}/digest/bouncer"
  if [ "$EZVER" = "31" ]; then	# autodetecting bouncer for 0.31x
    ${ECHO} "|/${EZBIN}/ezmlm-return '${DIR}'" >> "${DIR}/bouncer"
    ${ECHO} "|/${EZBIN}/ezmlm-return '${DIR}'" >> "${DIR}/digest/bouncer"
  else				# split bouncer with args for later versions
    ${ECHO} "|/${EZBIN}/ezmlm-return -D '${DIR}'" >> "${DIR}/bouncer"
    ${ECHO} "|/${EZBIN}/ezmlm-return -d '${DIR}'" >> "${DIR}/digest/bouncer"
  fi

# if testing qmail>=1.02, remove inlocal/inhost - shouldn't be used
  if [ "$QMVER" = "n" ]; then
	${RM} -f "${DIR}/inlocal" "${DIR}/inhost" > /dev/null || \
	  { ${ECHO} "failed to remove inlocal/inhost for testlist"; exit 100; }
  fi


###############
# ezmlm-clean #
###############

  prompt "ezmlm-clean (1/2):    "

# clean1 should be silently removed (no -x).
# clean2 should result in a message
# clean3 should not since it's time hasn't come
# clean4 should be removed, but not result in a message since we use -R

  ${ECHO} "Return-Path: <${DIG}@$HOST>" > "${DIR}/mod/pending/1"
  ${ECHO} "X-num: clean1" >> "${DIR}/mod/pending/1"
  ${ECHO} "Return-Path: <${DIG}@${HOST}>" > "${DIR}/mod/pending/2"
  ${ECHO} "X-num: clean2" >> "${DIR}/mod/pending/2"
  ${ECHO} "Return-Path: <${DIG}@$HOST>" > "${DIR}/mod/pending/999999999"
  ${ECHO} "X-num: clean3" >> "${DIR}/mod/pending/999999999"

  chmod +x "${DIR}/mod/pending/2" "${DIR}/mod/pending/999999999"

  ${EZBIN}/ezmlm-clean "${DIR}" >"${ERR}" 2>&1 ||
	{ ${ECHO} "failed first invocation"; exit 100; }
  if [ -r "${DIR}/mod/pending/1" ]; then
	${ECHO} "failed to remove non-x moderation queue entry 1"
	exit 100
  fi
  if [ -r "${DIR}/mod/pending/2" ]; then
	${ECHO} "failed to remove moderation queue entry 2"
	exit 100
  fi
  if [ ! -r "${DIR}/mod/pending/999999999" ]; then
	${ECHO} "removed mod queue entry 3 that wasn't due"
	exit 100
  fi

${ECHO} <<EOF > "${DIR}/mod/pending/4"
Return-Path: <${DIG}@$HOST>
X-num: clean4
EOF
  chmod +x "${DIR}/mod/pending/4"
  ${EZBIN}/ezmlm-clean -R "${DIR}" >"${ERR}" 2>&1 ||
	{ ${ECHO} "-R failed"; exit 100; }
  if [ -r  "${DIR}/mod/pending/4" ]; then
	${ECHO} "failed to remove moderation queue entry 4"; exit 100
  fi

  ${ECHO} "OK"

###############
# ezmlm-store #
###############

  prompt "ezmlm-store (1/2):    "

  SENDER="${SND}@$HOST"; export SENDER
  ${EZBIN}/ezmlm-sub "${DIR}/mod" "$SENDER"

# message from mod, normal use -> should queue
  { ${ECHO} "X-num: mod1"; make_message; } > ${TMP};
	${EZBIN}/ezmlm-store "${DIR}" >"${ERR}" 2>&1 < ${TMP} || \
	{ ${ECHO} "failed to process message 1"; exit 100; }

  cat ${DIR}/mod/pending/* | ${GREP} "mod1" > /dev/null || \
	{ ${ECHO} "failed to queue message 1"; exit 100; }

  ${RM} -f "${DIR}/modpost" 

# no modpost - should go directly to list
  { ${ECHO} "X-num: mod2"; make_message; } > ${TMP};
	${EZBIN}/ezmlm-store "${DIR}" >"${ERR}" 2>&1 < ${TMP} || \
	{
	  ${GREP} -v "child" "${ERR}" > /dev/null 2>&1
	  if [ "$?" != "0" ]; then
	    ${ECHO} "Failed to process message mod2"; exit 100
	  else
	    EZFORK='no'
	  fi
	}

  cat ${DIR}/mod/pending/* | ${GREP} "mod2" > /dev/null && \
	{ ${ECHO} "queued message 2 despite non-modpost"; exit 100; }

  if [ -z "$EZFORK" ]; then
	cat ${DIR}/archive/0/* | ${GREP} "mod2" > /dev/null || \
		{ ${ECHO} "failed to archive message 2 (non-modpost)"; exit 100; }
  fi

  touch "${DIR}/modpost"

# from moderator. Should be queued, even with -P
  { ${ECHO} "X-num: mod3"; make_message; } > ${TMP};
	${EZBIN}/ezmlm-store -P "${DIR}" >"${ERR}" 2>&1 < ${TMP} || \
	{ ${ECHO} "-P failed to accept mods post 3"; exit 100; }

  cat ${DIR}/mod/pending/* | ${GREP} "mod3" > /dev/null || \
	{ ${ECHO} "failed to queue message 3"; exit 100; }

  ${EZBIN}/ezmlm-unsub "${DIR}/mod" "$SENDER"

# not from moderator, should be rejected directly with -P
  { ${ECHO} "X-num: mod4"; make_message; } > ${TMP};
	${EZBIN}/ezmlm-store -P "${DIR}" >"${ERR}" 2>&1 < ${TMP} && \
	{ ${ECHO} "-P failed to reject non-mod message 4"; exit 100; }

  ${ECHO} "OK"

################
# ezmlm-return #
################
  prompt "ezmlm-return:         "

  SENDER="${BNC}@$HOST"; export SENDER
  HOST="$HOST"; export HOST
  LOCAL="$LOC-return-1-$BNC=$HOST"; export LOCAL
  if [ "$QMVER" = "n" ]; then
	DEFAULT="1-$BNC=$HOST"; export DEFAULT
  fi
# we use 'du' because bounce handling is different in 0.31x and >=0.32
  BSIZE1=`${DU} "${DIR}/bounce"` || \
	{ ${ECHO} "du doesn't work"; exit 99; } 
  make_message | ${EZBIN}/ezmlm-return "${DIR}" || \
    [ "$?" -eq "99" ] || \
	{ ${ECHO} "failed to process normal bounce from non-sub" ; exit 100; }
  BSIZE2=`${DU} "${DIR}/bounce"`
  if [ "$BSIZE1" != "$BSIZE2" ]; then
	${ECHO} "failed to ignore non-subscriber bounce" ; exit 100
  fi
  ${EZBIN}/ezmlm-sub "${DIR}" "${BNC}@$HOST"
  make_message | ${EZBIN}/ezmlm-return "${DIR}" || \
    [ "$?" -eq "99" ] || \
	{ ${ECHO} "failed to process normal bounce from sub" ; exit 100; }
  BSIZE1=`${DU} "${DIR}/bounce"`
  if [ "$BSIZE1" = "$BSIZE2" ]; then
	${ECHO} "failed to note subscriber bounce" ; exit 100
  fi
  LOCAL="$LOC-digest-return-1-$BNC=$HOST"; export LOCAL
  if [ "$QMVER" = "n" ]; then
	DEFAULT="1-$BNC=$HOST"; export DEFAULT
  fi
  BSIZE1=`${DU} "${DIR}/digest/bounce"`
  make_message | ${EZBIN}/ezmlm-return $DLC "${DIR}" || \
    [ "$?" -eq "99" ] || \
	{ ${ECHO} "failed to process normal digest non-sub bounce" ; exit 100; }
  BSIZE2=`${DU} "${DIR}/digest/bounce"`
  if [ "$BSIZE1" != "$BSIZE2" ]; then
	${ECHO} "failed to ignore non-digest-subscriber bounce" ; exit 100
  fi
  ${EZBIN}/ezmlm-unsub "${DIR}" "${BNC}@$HOST"
  ${EZBIN}/ezmlm-sub "${DIR}/digest" "${BNC}@$HOST"
  make_message | ${EZBIN}/ezmlm-return $DLC "${DIR}" || \
    [ "$?" -eq "99" ] || \
	{ ${ECHO} "failed to proc. nl digest-subscriber bounce" ; exit 100; }
  BSIZE1=`${DU} "${DIR}/digest/bounce"`
  if [ "$BSIZE1" = "$BSIZE2" ]; then
	${ECHO} "failed to note digest-subscriber bounce" ; exit 100
  fi
  ${EZBIN}/ezmlm-sub "${DIR}" "${BNC}@$HOST"

  ${ECHO} "OK"

# as we exit, the bounce address is subscribed to both list and digest-list
# and is the SENDER

##############
# ezmlm-warn #
##############
  prompt "ezmlm-warn (1/3):     "

# should send a warning
  ${EZBIN}/ezmlm-warn -t0 "${DIR}" >"${ERR}" 2>&1 || \
	{ ${ECHO} "failed with normal bounce for warning"; exit 100; }
  ${EZBIN}/ezmlm-issubn "${DIR}" || \
	{ ${ECHO} "script error: SENDER is not a subscriber"; exit 100; }

  ${EZBIN}/ezmlm-warn -d -t0 "${DIR}" >"${ERR}" 2>&1 || \
	{ ${ECHO} "failed with digest bounce for warning"; exit 100; }
  ${EZBIN}/ezmlm-issubn "${DIR}/digest" || \
	{ ${ECHO} "script error: SENDER is not a digest subscriber"; exit 100; }

  ${ECHO} "OK"

################
# ezmlm-manage #
################
  prompt "ezmlm-manage (1/4):   "

  LOCAL="$LOC-unsubscribe"; export LOCAL
  if [ "$QMVER" = "n" ]; then
	DEFAULT='unsubscribe'; export DEFAULT
  fi
  SENDER="${SND}@$HOST"; export SENDER

  ${EZBIN}/ezmlm-sub "${DIR}" "${SND}@$HOST"
  ${EZBIN}/ezmlm-manage -U "${DIR}" </dev/null >"${ERR}" 2>&1 || \
	{ ${ECHO} "failed with -U"; exit 100; }
  ${EZBIN}/ezmlm-issubn "${DIR}" && \
	{ ${ECHO} "unsubscribe with -U failed"; exit 100; }

  LOCAL="$LOC-digest-subscribe"; export LOCAL
  if [ "$QMVER" = "n" ]; then
	DEFAULT='digest-subscribe'; export DEFAULT
  fi
  ${EZBIN}/ezmlm-unsub "${DIR}/digest" "${SND}@$HOST"

# test that access to the deny db is restricted to remote admins
  LOCAL="$LOC-deny-subscribe"; export LOCAL
  if [ "$QMVER" = "n" ]; then
	DEFAULT='deny-subscribe'; export DEFAULT
  fi
  ${EZBIN}/ezmlm-manage "${DIR}" </dev/null >/dev/null 2>&1 && \
	{
	 ${ECHO} "Deny open to regular subscribers: 0.31 bug, OK in 0.321."
	 prompt "ezmlm-manage ...      "
	 BUG="${BUG} deny"
	}
  SENDER="${MOD}@$HOST"; export SENDER
  ${EZBIN}/ezmlm-sub "${DIR}/mod" "$SENDER" || exit 100
  ${EZBIN}/ezmlm-manage "${DIR}" </dev/null > "${ERR}" 2>&1 || \
	{ ${ECHO} "Deny access denied to remote admin!"; exit 100; }

# make non-moderated
  ${RM} -f "${DIR}/modsub" || \
	{ ${ECHO} "Failed to remove DIR/modsub"; exit 99; }

# make non-remote
  ${RM} -f "${DIR}/remote" || \
	{ ${ECHO} "Failed to remove DIR/remote"; exit 99; }
  ${EZBIN}/ezmlm-manage "${DIR}" </dev/null > "${ERR}" 2>&1 && \
	{
	 ${ECHO} "Deny even without remote/modsub: 0.31 bug, OK in 0.321."
	 prompt "ezmlm-manage ...      "
	 BUG="${BUG} deny"
	}

# restore remote/SENDER/mod/LOCAL/DEFAULT
  ${EZBIN}/ezmlm-unsub "${DIR}/mod" "$SENDER" || exit 100
  SENDER="${SND}@$HOST"; export SENDER	# restore order
  touch "${DIR}/remote" || \
	{ ${ECHO} "Failed to remove DIR/remote"; exit 99; }
  LOCAL="$LOC-digest-subscribe"; export LOCAL
  if [ "$QMVER" = "n" ]; then
	DEFAULT='digest-subscribe'; export DEFAULT
  fi

  ${EZBIN}/ezmlm-manage -S "${DIR}" </dev/null >"${ERR}" 2>&1 || \
	{ ${ECHO} "failed with -S"; exit 100; }
  ${EZBIN}/ezmlm-issubn "${DIR}/digest" || \
	{ ${ECHO} "digest-subscribe with -S failed"; exit 100; }
  ${EZBIN}/ezmlm-unsub "${DIR}/digest" "${SND}@$HOST"
  touch "${DIR}/modsub" || \
	{ ${ECHO} "Failed to restore DIR/modsub"; exit 99; }

  SENDER="${MAN}@$HOST"; export SENDER

  ${ECHO} "X-num: sub1" > "${DIR}/__tmp"
  ${ECHO} "From: Mr. $EZTEST requests <${MAN}@$HOST>" >> "${DIR}/__tmp"
  ${ECHO} >> "${DIR}/__tmp"
  ${EZBIN}/ezmlm-manage ${SW_FROM} "${DIR}" < "${DIR}/__tmp" \
		>"${ERR}" 2>&1 || \
	{ ${ECHO} "digest-subscribe with request failed"; exit 100; }

  ${EZBIN}/ezmlm-sub "${DIR}" "${MAN}@$HOST"
  LOCAL="$LOC-unsubscribe"; export LOCAL
  if [ "$QMVER" = "n" ]; then
	DEFAULT='unsubscribe'; export DEFAULT
  fi
  ${ECHO} "X-num: sub2" > "${DIR}/__tmp"
  ${EZBIN}/ezmlm-manage "${DIR}" < "${DIR}/__tmp" >"${ERR}" 2>&1 || \
	{ ${ECHO} "unsubscribe request failed"; exit 100; }

# -get function for backwards compatibility
  LOCAL="$LOC-get.1"; export LOCAL
  if [ "$QMVER" = "n" ]; then
	DEFAULT='get.1'; export DEFAULT
  fi
  ${ECHO} "X-num: manget1" > "${DIR}/__tmp"
  ${EZBIN}/ezmlm-manage "${DIR}" < "${DIR}/__tmp" >"${ERR}" 2>&1 || \
	{ ${ECHO} "get failed"; exit 100; }
# -C should disable it
  ${EZBIN}/ezmlm-manage -C "${DIR}" < "${DIR}/__tmp" >"${ERR}" 2>&1 && \
	{ ${ECHO} "-C failed to disable -get"; exit 100; }

  ${ECHO} "OK"

#################
# ezmlm-request #
#################
  prompt "ezmlm-request (1/2):  "

  SENDER="${SND}@$HOST"; export SENDER
  LOCAL="$LOC-request"; export LOCAL
  if [ "$QMVER" = "n" ]; then
	DEFAULT='request'; export DEFAULT
  fi

  ${ECHO} "X-num: req1" > "${DIR}/__tmp"
# use a non-existing command
  ${ECHO} "Subject: qqqq ${SND}@$HOST" >> "${DIR}/__tmp"
  ${EZBIN}/ezmlm-request "${DIR}" < "${DIR}/__tmp" > "${ERR}" 2>&1
  if [ "$?" != "99" ]; then
	${ECHO} "qqqq command in subject failed to exit 99"
	exit 100
  fi

  ${ECHO} "X-num: req1" > "${DIR}/__tmp"
# test full ezmlm cmd in subject and command substitution
  ${ECHO} "Subject: ${LOC}-remove-${SND}=${HOST}@${HOST}" >> "${DIR}/__tmp"
  ${EZBIN}/ezmlm-request "${DIR}" < "${DIR}/__tmp" > "${ERR}" 2>&1
  if [ "$?" != "99" ]; then
	${ECHO} "full ezmlm command in subject failed to exit 99"
	exit 100
  fi



  ${ECHO} "OK"

###############
# ezmlm-split #
###############
if [ "$QMVER" = "n" ]; then
  prompt "ezmlm-split (1/2):    "
# set up split file
  ${ECHO} "edu:1:26:l1@h1" > "${DIR}/split"
  ${ECHO} "edu:27:52:l2@h2" >> "${DIR}/split"
  ${ECHO} "com:::l3@h3" >> "${DIR}/split"
# most testing with -D
  ${ECHO} "lindberg@ezmlm.org" | ${EZBIN}/ezmlm-split -D "${DIR}" | \
	${GREP} "$LIST@$HOST" >/dev/null || \
	{ ${ECHO} "failed to split correctly on domain"; exit 100; }
  ${ECHO} "lindberg@id.com" | ${EZBIN}/ezmlm-split -D "${DIR}" | \
	${GREP} 'l3' >/dev/null || \
	{ ${ECHO} "failed to split correctly on domain"; exit 100; }
  ${ECHO} "lindberg@id.wustl.edu" | ${EZBIN}/ezmlm-split -D "${DIR}" | \
	${GREP} 'l1' >/dev/null || \
	{ ${ECHO} "failed to split correctly on hash + domain"; exit 100; }
  ${ECHO} "cfl@id.wustl.edu" | ${EZBIN}/ezmlm-split -D "${DIR}" | \
	${GREP} 'l2' >/dev/null || \
	{ ${ECHO} "failed to split correctly on hash + domain"; exit 100; }
# one test with delivery - redirect to local manager
# should exit 99 after redirecting
  ${ECHO} ":::${LOC}@$HOST" > "${DIR}/split"
  SENDER="${MOD}@$HOST"; export SENDER
  DTLINE="Delivered-To: ezmlm-split@$HOST"; export DTLINE
  LOCAL="$LOC-subscribe-${SND}=$HOST"; export LOCAL
  if [ "$QMVER" = "n" ]; then
	DEFAULT="subscribe-${SND}=$HOST"; export DEFAULT
  fi
  ${ECHO} "X-num: spl1" | ${EZBIN}/ezmlm-split "${DIR}" >"${ERR}" 2>&1

  EC="$?"
  if [ "$EC" -eq "0" ]; then
	${ECHO} "exited 0 after forwarding, rather than 99"; exit 100
  elif [ "$EC" != "99" ]; then
	${ECHO} "failed to process message for forwarding"; exit 100
  fi
# if no match, should exit 0
  ${ECHO} "___:::${LOC}@$HOST" > "${DIR}/split"
  ${ECHO} "X-num: spl1" | ${EZBIN}/ezmlm-split "${DIR}" >"${ERR}" 2>&1 || \
	{ ${ECHO} "failed to exit 0 after failing to match"; exit 100; }

  ${ECHO} "OK"
fi

########################
# waiting for delivery #
########################
  send_test 2
fi		# end of sect 1

####################################### start of section 2

if [ "$SECT" -le "2" ]; then
  wait_test 2

#############
# ezmlm-idx #
#############
  prompt "ezmlm-idx:            "
  ${RM} -f "${DIR}/archive/0/index" "${DIR}/indexed"
  ${EZBIN}/ezmlm-idx "${DIR}" >"${ERR}" 2>&1 || \
	{ ${ECHO} "failed to run"; exit 100; }
  if [ ! -r "${DIR}/indexed" ]; then
	${ECHO} "failed to create DIR/indexed"; exit 100
  fi
  if [ ! -r "${DIR}/archive/0/index" ]; then
	${ECHO} "failed to create index"; exit 100
  fi 
  ${ECHO} "OK"

#############
# ezmlm-get #
#############
prompt "ezmlm-get (1/2):      "

# blast digest recipient account with all these excerpts.
${EZBIN}/ezmlm-sub "${DIR}/digest" "${DIG}@$HOST"

# first ezmlm-get in the manager position:

# index1/get1/thread1 should bounce and will not be looked for
# index2 ... should be in DIG@HOST's inbox
# get3 - r format to DIG@HST
# get4 - n
# get5 - v
# get6 - x

SENDER="${BNC}@$HOST"; export SENDER
LOCAL="$LOC-xxxx"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT='xxxx'; export DEFAULT
fi
${ECHO} "X-num: index1" > "${DIR}/__tmp"
${EZBIN}/ezmlm-get "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 || \
	{ ${ECHO} " failed to exit 0 for non-recognized commands"; exit 100; }

# This should not give a digest
LOCAL="$LOC-"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT=''; export DEFAULT
fi
${EZBIN}/ezmlm-get "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 || \
	{ ${ECHO} " failed to exit 0 for list-@host"; exit 100; }

LOCAL="$LOC-index"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT='index'; export DEFAULT
fi
${EZBIN}/ezmlm-get -s "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 && \
	{ ${ECHO} "-s failed to reject -index from non-sub"; exit 100; }
${EZBIN}/ezmlm-get "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1
if [ "$?" -ne "99" ]; then
	${ECHO} "failed to exit 99 after -index"
	exit 100
fi

${ECHO} "X-num: index2" > "${DIR}/__tmp"
SENDER="${DIG}@$HOST"; export SENDER
${EZBIN}/ezmlm-get -s "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1
if [ "$?" -ne "99" ]; then
	${ECHO} "-s failed to exit 99 after -index"
	exit 100
fi

SENDER="${BNC}@$HOST"; export SENDER
${ECHO} "X-num: get1" > "${DIR}/__tmp"
LOCAL="$LOC-get.2_4"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT='get.2_4'; export DEFAULT
fi
${EZBIN}/ezmlm-get -s "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 && \
	{ ${ECHO} "-s failed to reject -get from non-sub"; exit 100; }
${EZBIN}/ezmlm-get "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1
if [ "$?" != "99" ]; then
	${ECHO} "failed to exit 99 after -get"
	exit 100
fi
${ECHO} "X-num: get2" > "${DIR}/__tmp"
SENDER="${DIG}@$HOST"; export SENDER
${EZBIN}/ezmlm-get -s "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1
if [ "$?" != "99" ]; then
	${ECHO} "-s failed to exit 99 after -get"
	exit 100
fi

# test formats for -get
${ECHO} "X-num: get3" > "${DIR}/__tmp"
LOCAL="$LOC-getr.2_4"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT='getr.2_4'; export DEFAULT
fi
${EZBIN}/ezmlm-get "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1
if [ "$?" != "99" ]; then
	${ECHO} "failed to exit 99 after -getr"
	exit 100
fi
${ECHO} "X-num: get4" > "${DIR}/__tmp"
LOCAL="$LOC-getn.2_4"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT='getn.2_4'; export DEFAULT
fi
${EZBIN}/ezmlm-get "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1
if [ "$?" != "99" ]; then
	${ECHO} "failed to exit 99 after -getn"
	exit 100
fi

${ECHO} "X-num: get5" > "${DIR}/__tmp"
LOCAL="$LOC-getv.2_4"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT='getv.2_4'; export DEFAULT
fi
${EZBIN}/ezmlm-get "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1
if [ "$?" != "99" ]; then
	${ECHO} "failed to exit 99 after -getv"
	exit 100
fi

${ECHO} "X-num: get6" > "${DIR}/__tmp"
LOCAL="$LOC-getx.2_4"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT='getx.2_4'; export DEFAULT
fi
${EZBIN}/ezmlm-get "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1
if [ "$?" != "99" ]; then
	${ECHO} "failed to exit 99 after -getx"
	exit 100
fi

SENDER="${BNC}@$HOST"; export SENDER
LOCAL="$LOC-index"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT='index'; export DEFAULT
fi
${ECHO} "X-num: thread1" > "${DIR}/__tmp"
LOCAL="$LOC-thread.1"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT='thread.1'; export DEFAULT
fi
${EZBIN}/ezmlm-get -s "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 && \
	{ ${ECHO} "-s failed to reject -thread from non-sub"; exit 100; }
${EZBIN}/ezmlm-get "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1
if [ "$?" != "99" ]; then
	${ECHO} "failed to exit 99 after -thread"
	exit 100
fi
${ECHO} "X-num: thread2" > "${DIR}/__tmp"
SENDER="${DIG}@$HOST"; export SENDER
${EZBIN}/ezmlm-get -s "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1
if [ "$?" != "99" ]; then
	${ECHO} "-s failed to exit 99 after -thread"
	exit 100
fi

######### digests
# we use headeradd to label them since trigger headers aren't propagated
${ECHO} "X-num: not_propagated" > "${DIR}/__tmp"

# dig1 from manager will go to DIG@HOST
# dig2 from editor
# dig3 from command line
# dig4 -fr format check from command line. We check only that they get there.
# dig5 -fn
# dig6 -fx
# dig7 -fv
# we check that dignum is created and digissue is updated 

# now -dig in the manager position:
mv -f "${DIR}/headeradd" "${DIR}/headeradd.bak"
${ECHO} "X-num: dig1" > "${DIR}/headeradd"
SENDER="${BNC}@$HOST"; export SENDER
LOCAL="$LOC-dig.code"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT='dig.code'; export DEFAULT
fi
${EZBIN}/ezmlm-get "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 && \
	{ ${ECHO} "failed to reject -dig when no digest code was on cmd-line"
	  exit 100
	}
if [ -r "${DIR}/dignum" ]; then
	${ECHO} "script error: dignum exists"; exit 100
fi
${EZBIN}/ezmlm-get "${DIR}" 'code' < "${DIR}/__tmp" >"${ERR}" 2>&1
if [ "$?" != "99" ]; then
	${ECHO} "failed to exit 99 after digest in manager position"
	exit 100
fi
if [ ! -r "${DIR}/dignum" ]; then
	${ECHO} "failed to generate dignum"; exit 100
fi
if [ ! -r "${DIR}/digissue" ]; then
	${ECHO} "failed to generate digissue"; exit 100
fi
${EZBIN}/ezmlm-get "${DIR}" 'code' < "${DIR}/__tmp" >/dev/null 2>&1
if [ "$?" != "99" ]; then
	${ECHO} "failed to exit 99 when nothing to digest in manager position"
	exit 100
fi

${EZBIN}/ezmlm-get "${DIR}" 'coden' < "${DIR}/__tmp" >/dev/null 2>&1 && \
	{ ${ECHO} "failed to reject -dig with bad digest code 'coden'"; exit 100; }
${EZBIN}/ezmlm-get "${DIR}" 'cod' < "${DIR}/__tmp" >/dev/null 2>&1 && \
	{ ${ECHO} "failed to reject -dig with bad digest code 'cod'"; exit 100; }

# now in the editor position:
${RM} -f "${DIR}/dignum"
LOCAL="$LOC"; export LOCAL
if [ "$QMVER" = "n" ]; then
	${UNSET} DEFAULT
fi
${ECHO} "X-num: dig2" > "${DIR}/headeradd"
${EZBIN}/ezmlm-get "${DIR}" < "${DIR}/__tmp" >"${ERR}" 2>&1 || \
	{ ${ECHO} "failed to exit 0 after digest in editor"; exit 100; }

# This causes an error on systems where 'unset' doesn't work
# For these, we skip this test.
  if [ -z "$BADUNSET" ]; then
    if [ ! -r "${DIR}/dignum" ]; then
	${ECHO} "failed to generate dignum after digest in editor"; exit 100
    fi

    ${GREP} "2:" "${DIR}/digissue" >/dev/null 2>&1 || \
	{ ${ECHO} "failed to update digissue after digest in editor";
	  exit 100; }
    ${EZBIN}/ezmlm-get "${DIR}" < "${DIR}/__tmp" >"${ERR}" 2>&1 || \
	{ ${ECHO} "failed to exit 0 when nothing to digest in editor";
	  exit 100; }
  fi

# now from the command line with formats ...
${RM} -f "${DIR}/dignum"
LOCAL=''; export LOCAL
${ECHO} "X-num: dig3" > "${DIR}/headeradd"
${EZBIN}/ezmlm-get "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 || \
	{ ${ECHO} "failed to exit 0 after cmd line digest"; exit 100; }
${GREP} "3:" "${DIR}/digissue" >/dev/null 2>&1 || \
	{ ${ECHO} "failed to update digissue after cmd line digest"; exit 100; }
${EZBIN}/ezmlm-get "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 || \
	{ ${ECHO} "failed to exit 0 when nothing to digest from cmd line"
	exit 100; }
${RM} -f "${DIR}/dignum"
${ECHO} "X-num: dig4" > "${DIR}/headeradd"
${EZBIN}/ezmlm-get -fr "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 || \
	{ ${ECHO} "-fr failed for digest"; exit 100; }
${RM} -f "${DIR}/dignum"
${ECHO} "X-num: dig5" > "${DIR}/headeradd"
${EZBIN}/ezmlm-get -fn "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 || \
	{ ${ECHO} "-fn failed for digest"; exit 100; }
${RM} -f "${DIR}/dignum"
${ECHO} "X-num: dig6" > "${DIR}/headeradd"
${EZBIN}/ezmlm-get -fv "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 || \
	{ ${ECHO} "-fv failed for digest"; exit 100; }
${RM} -f "${DIR}/dignum"
${ECHO} "X-num: dig7" > "${DIR}/headeradd"
${EZBIN}/ezmlm-get -fx "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 || \
	{ ${ECHO} "-fx failed for digest"; exit 100; }

# restore headeradd
mv -f "${DIR}/headeradd.bak" "${DIR}/headeradd"

${ECHO} "OK"

##############
# ezmlm-send #
##############
prompt "ezmlm-send (2/2):     "
MSG1=`${GREP} -l "msg1" $SINKDIR/new/*` || \
	{ ${ECHO} "failed to deliver message 1 to subscriber"; \
	exit 100; }
# make sure headeradd was done
  ${GREP} -i 'precedence: bulk' < "$MSG1" >/dev/null 2>&1 ||
	{ ${ECHO} "failed to add headeradd"; exit 100; }
# check on received: header handling
${GREP} '#PENULTIMATE#' "$MSG1" >/dev/null && \
	{ ${ECHO} "-r failed to remove received header"; \
	exit 100; }
${GREP} '#LAST#' "$MSG1" >/dev/null || \
	{ ${ECHO} "-r failed to leave last received header"; \
	exit 100; }
${GREP} 'Subject:' "$MSG1" | ${GREP} 'PFX' >/dev/null 2>&1 || \
	{ ${ECHO} "failed to add subject prefix"; exit 100; }
	# the trailer should be a MIME part, so not at the very end
${TAIL} -6 "$MSG1" | ${HEAD} -2 | ${GREP} 'TRAILER' >/dev/null 2>&1 || \
	{ ${ECHO} "failed to add trailer"; exit 100; }

MSG2=`${GREP} -l "msg2" $SINKDIR/new/*` || \
	{ ${ECHO} "failed to deliver message 2 to subscriber"; \
	exit 100; }
${GREP}  '#PENULTIMATE#' "$MSG2" >/dev/null || \
	{ ${ECHO} "-R failed to leave received header"; \
	exit 100; }

${GREP} "msg3" $SINKDIR/new/* >/dev/null 2>&1 && \
	{ ${ECHO} "-C failed to exclude sender (no longer supported)"; \
	  BUG="${BUG}_noself"; \
	  prompt "ezmlm-send:           "; }

MSG5=`${GREP} -l "msg5" $SINKDIR/new/*` || \
	{ ${ECHO} "failed to deliver message 5 to subscriber"; \
	exit 100; }
${GREP} 'TRAILER' "$MSG5" >/dev/null 2>&1 || \
	{ ${ECHO} "failed to add trailer to non-mime message"; \
	exit 100; }

MSG6=`${GREP} -l "msg6" $SINKDIR/new/*` || \
	{ ${ECHO} "failed to deliver message 6 to subscriber"; \
	exit 100; }

${GREP} 'TRAILER' "$MSG6" >/dev/null 2>&1 && \
	{ ${ECHO} "failed to suppress trailer for multipart/signed message"; \
	  ${ECHO} "                      0.31 bug fixed in 0.316/0.323";
	  BUG="${BUG}_signed"; \
	  prompt "ezmlm-send ......:    "; }

${GREP} "msg3" $SINKDIR/new/* >/dev/null 2>&1 && \
	{ 
	  ${ECHO} "${BUG}" | ${GREP} 'noself' >/dev/null 2>&1 || \
	  {
	    ${ECHO} "-C failed to exclude sender (no longer supported)"
	    BUG="${BUG}_noself"
	    prompt "ezmlm-send ......:   ${BUG} "
	  }
	}

${ECHO} "OK"
###############
# ezmlm-clean #
###############

prompt "ezmlm-clean (2/2):    "

${GREP} "clean1" ${DIGDIR}/new/* >/dev/null 2>&1 && \
	{ ${ECHO} "removal of non-x mod queue entry 1 wasn't silent"; exit 100; }
${GREP} "clean2" ${DIGDIR}/new/* >/dev/null 2>&1 || \
	{ ${ECHO} "failed to notify sender of mod queue entry 2 time out"
	  exit 100
	}
${GREP} "clean3" ${DIGDIR}/new/* >/dev/null 2>&1 && \
	{ ${ECHO} "notified sender about entry 3 even though it wasn't rejected"
	  exit 100
	}
${GREP} "clean4" ${DIGDIR}/new/* >/dev/null 2>&1 && \
	{ ${ECHO} "-R failed: notified sender about entry 3 rejection"; exit 100; }


# clean1 should be silently removed (no -x).
# clean2 should result in a message
# clean3 should not since it's time hasn't come
# clean4 should be removed, but not result in a message since we use -R

${ECHO} "OK"

###############
# ezmlm-store #
###############
prompt "ezmlm-store (2/2):    "

MOD1=`${GREP} -l "mod1" $SINKDIR/new/* 2>/dev/null`
if [ -z "$MOD1" ]; then
	${ECHO} "ezmlm-store: failed to deliver mod request to moderator"
	exit 100
fi
${GREP} "mod2" $SINKDIR/new/* >/dev/null && \
	{ ${ECHO} "ezmlm-store: didn't post directly in absence of DIR/modpost"; \
	exit 100; }
MOD3=`${GREP} -l "mod3" $SINKDIR/new/* 2>/dev/null`
if [ -z "$MOD3" ]; then
	${ECHO} "ezmlm-store: -P failed to deliver mod request to moderator"
	exit 100
fi
${GREP} "mod4" $SINKDIR/new/* >/dev/null && \
	{ ${ECHO} "ezmlm-store: -P failed to reject message from non-mod"; \
	exit 100; }

${ECHO} "OK"

################
# ezmlm-manage #
################
prompt "ezmlm-manage (2/4):   "

# check digest-subscribe and list-unsubscribe replies
SUB1=`${GREP} -l 'sub1' $MANDIR/new/*` || \
	{ ${ECHO} "failed getting digest-subscribe confirm request"; exit 100; }

SUB2=`${GREP} -l 'sub2' $MANDIR/new/*` || \
	{ ${ECHO} "failed getting -unsubscribe confirm request"; exit 100; }

# Check -get.1 reply
MANGET1=`${GREP} -l 'manget1' $MANDIR/new/*` || \
	{ ${ECHO} "failed getting -get.1 reply"; exit 100; }

${GREP} 'msg1' "$MANGET1" >/dev/null || \
	{ ${ECHO} "get.1 failed to return archived message"; exit 100; }

# Add moderator
${EZBIN}/ezmlm-sub "${DIR}/mod" "${MOD}@$HOST"

LOCAL=`${GREP} "Reply-To:" "$SUB1" | cut -d' ' -f2 | cut -d'@' -f1` || \
	{ ${ECHO} "failed to find confirm address in -subscribe reply"; exit 100; }
export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT=`${ECHO} "$LOCAL" | cut -c"$LOCLEN"-`; export DEFAULT
fi
${ECHO} "X-num: sub3" > "${DIR}/__tmp"
${ECHO} "From: Mr. $EZTEST confirms <$SENDER>" >> "${DIR}/__tmp"
${ECHO} >> "${DIR}/__tmp"
${EZBIN}/ezmlm-manage ${SW_FROM} "${DIR}" < "${DIR}/__tmp" \
		>"${ERR}" 2>&1 || \
	{ ${ECHO} "failed to send user conf for sub1"; exit 100; }

LOCAL=`${GREP} "Reply-To:" "$SUB2" | cut -d' ' -f2 | cut -d'@' -f1` || \
	{ ${ECHO} "failed to find confirm address in -unsubscribe reply"
	  exit 100; }
export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT=`${ECHO} "$LOCAL" | cut -c"$LOCLEN"-`; export DEFAULT
fi
${ECHO} "X-num: sub4" > "${DIR}/__tmp"
${EZBIN}/ezmlm-manage "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 || \
	{ ${ECHO} "failed to send conf for sub2"; exit 100; }

# now test remote admin functions
# add a few addresses to allow
${EZBIN}/ezmlm-sub "${DIR}/${ALLOW}" "aaa@bbb" "ccc@ddd" "eee@fff"

# test -edit
${ECHO} "#TEST_TEXT#" > "${DIR}/text/test"
LOCAL="$LOC-edit.test-$MAN=$HOST"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT="edit.test-$MAN=$HOST"; export DEFAULT
fi
${ECHO} "X-num: edit1" > "${DIR}/__tmp"
${EZBIN}/ezmlm-manage -e "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 && \
	{ ${ECHO} "failed to reject edit request from non-mod"; exit 100; }
LOCAL="$LOC-edit.test-$MOD=$HOST"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT="edit.test-$MOD=$HOST"; export DEFAULT
fi
${ECHO} "X-num: edit2" > "${DIR}/__tmp"
${EZBIN}/ezmlm-manage "${DIR}" < "${DIR}/__tmp" >"${ERR}" 2>&1 && \
	{ ${ECHO} "-E failed for edit2"; exit 100; }
${ECHO} "X-num: edit3" > "${DIR}/__tmp"
${EZBIN}/ezmlm-manage -e "${DIR}" < "${DIR}/__tmp" >"${ERR}" 2>&1 || \
	{ ${ECHO} "-e failed for remote admin for edit3"; exit 100; }

# test list/log
LOCAL="$LOC-allow-list-$MAN=$HOST"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT="allow-list-$MAN=$HOST"; export DEFAULT
fi
${ECHO} "X-num: list1" > "${DIR}/__tmp"
${EZBIN}/ezmlm-manage -l "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 && \
	{ ${ECHO} "failed to reject list request from non-mod"; exit 100; }

LOCAL="$LOC-allow-log-$MAN=$HOST"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT="allow-log-$MAN=$HOST"; export DEFAULT
fi
${ECHO} "X-num: log1" > "${DIR}/__tmp"
${EZBIN}/ezmlm-manage -l "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 && \
	{ ${ECHO} "failed to reject log request from non-mod"; exit 100; }

LOCAL="$LOC-allow-list-$MOD=$HOST"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT="allow-list-$MOD=$HOST"; export DEFAULT
fi
${ECHO} "X-num: list2" > "${DIR}/__tmp"
${EZBIN}/ezmlm-manage "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 && \
	{ ${ECHO} "-L failed to reject list request"; exit 100; }

${ECHO} "X-num: list3" > "${DIR}/__tmp"
${EZBIN}/ezmlm-manage -l "${DIR}" < "${DIR}/__tmp" >"${ERR}" 2>&1 || \
	{ ${ECHO} "-l failed for remote admin for list3"; exit 100; }

LOCAL="$LOC-allow-log-$MOD=$HOST"; export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT="allow-log-$MOD=$HOST"; export DEFAULT
fi
${ECHO} "X-num: log2" > "${DIR}/__tmp"
${EZBIN}/ezmlm-manage "${DIR}" < "${DIR}/__tmp" >"${ERR}" 2>&1 && \
	{ ${ECHO} "-L failed to reject log request"; exit 100; }

${ECHO} "X-num: log3" > "${DIR}/__tmp"
${EZBIN}/ezmlm-manage -l "${DIR}" < "${DIR}/__tmp" >"${ERR}" 2>&1 || \
	{ ${ECHO} "-l failed for remote admin for log3"; exit 100; }


${ECHO} "OK"

##################
# ezmlm-moderate #
##################

prompt "ezmlm-moderate (1/2): "

# MOD1 and MOD3 are defined from ezmlm-store testing

REJ=`${GREP} "From: $LOC-reject" "$MOD1"| cut -d' ' -f2`
if [ -z "$REJ" ]; then
	${ECHO} "No From: ...-reject header in mod request for mod1"
	exit 100
fi

ACC=`${GREP} "Reply-To: $LOC-accept" "$MOD3"| cut -d' ' -f2`
if [ -z "$ACC" ]; then
	${ECHO} "No From: ...-accept header in mod request for mod3"
	exit 100
fi

# remove moderation request from sinkdir
${RM} -f "$MOD1" 2>/dev/null || \
	{ ${ECHO} "failed to remove mod request for mod1"; exit 100; }
${RM} -f "$MOD3" 2>/dev/null || \
	{ ${ECHO} "failed to remove mod request for mod3"; exit 100; }

# make sure we get the (mis)accepted message(s)
${EZBIN}/ezmlm-sub "${DIR}" "${SND}@$HOST"

LOCAL=`${ECHO} "$REJ" | cut -d@ -f1`
export LOCAL

if [ "$QMVER" = "n" ]; then
	DEFAULT=`${ECHO} "$LOCAL" | cut -c"$REJLEN"-`; export DEFAULT
fi
${EZBIN}/ezmlm-moderate "${DIR}" "${EZBIN}/ezmlm-send ${DIR}" \
	</dev/null >"${ERR}" 2>&1 || \
		{ ${ECHO} "failed on rejection"; exit 100; }

LOCAL=`${ECHO} "$ACC" | cut -d@ -f1`
export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT=`${ECHO} "$LOCAL" | cut -c"$ACCLEN"-`; export DEFAULT
fi
${EZBIN}/ezmlm-moderate "${DIR}" "${EZBIN}/ezmlm-send ${DIR}" \
	 </dev/null >"${ERR}" 2>&1 || \
		{ ${ECHO} "failed on acceptance"; exit 100; }

ls -l "${DIR}/mod/rejected/" | ${GREP} '[0-9]' >/dev/null 2>&1 || \
	{ ${ECHO} "failed to write reject stub"; exit 100; }
ls -l "${DIR}/mod/accepted/" | ${GREP} '[0-9]' >/dev/null 2>&1 || \
	{ ${ECHO} "failed to write accept stub"; exit 100; }

REJ1=`${ECHO} "$REJ" | sed s/reject/accept/`
LOCAL=`${ECHO} "$REJ1" | cut -d@ -f1`
export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT=`${ECHO} "$LOCAL" | cut -c"$REJLEN"-`; export DEFAULT
fi
${EZBIN}/ezmlm-moderate "${DIR}" "${EZBIN}/ezmlm-send ${DIR}" \
	</dev/null >/dev/null 2>&1 && \
	{ ${ECHO} "failed to bounce accept of rejected message"; exit 100; }
LOCAL=`${ECHO} "$REJ" | cut -d@ -f1`
export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT=`${ECHO} "$LOCAL" | cut -c"$REJLEN"-`; export DEFAULT
fi
${EZBIN}/ezmlm-moderate "${DIR}" "${EZBIN}/ezmlm-send ${DIR}" \
	</dev/null >/dev/null 2>&1 || \
	{ ${ECHO} "failed to silently ignore re-rejection"; exit 100; }

ACC1=`${ECHO} "$ACC" | sed s/accept/reject/`
LOCAL=`${ECHO} "$ACC1" | cut -d@ -f1`
export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT=`${ECHO} "$LOCAL" | cut -c"$REJLEN"-`; export DEFAULT
fi
${EZBIN}/ezmlm-moderate "${DIR}" "${EZBIN}/ezmlm-send ${DIR}" \
	</dev/null >/dev/null 2>&1 && \
	{ ${ECHO} "failed to bounce reject of accepted message"; exit 100; }
LOCAL=`${ECHO} "$ACC" | cut -d@ -f1`
export LOCAL
if [ "$QMVER" = "n" ]; then
	DEFAULT=`${ECHO} "$LOCAL" | cut -c"$ACCLEN"-`; export DEFAULT
fi
${EZBIN}/ezmlm-moderate "${DIR}" "${EZBIN}/ezmlm-send ${DIR}" \
	</dev/null >/dev/null 2>&1 || \
	{ ${ECHO} "failed to silently ignore re-acceptance"; exit 100; }

${ECHO} "OK"

# cleanup
${EZBIN}/ezmlm-unsub "${DIR}" "${SND}@$HOST"

##############
# ezmlm-warn #
##############
prompt "ezmlm-warn (2/3):     "

${EZBIN}/ezmlm-warn -t0 "${DIR}" >"${ERR}" 2>&1 || \
	{ ${ECHO} "failed with normal bounce for warning"; exit 100; }

${EZBIN}/ezmlm-warn -d -t0 "${DIR}" >"${ERR}" 2>&1 || \
	{ ${ECHO} "failed with digest bounce for warning"; exit 100; }

${ECHO} "OK"

#################
# ezmlm-request #
#################

  prompt "ezmlm-request (2/2):  "

  ${GREP} "$LOC-qqqq-$SND=$HOST" "${REQ}" >/dev/null || \
	{ ${ECHO} "'qqqq' subject query rewriting failed"; exit 100; }

  ${GREP} "$LOC-unsubscribe-$SND=$HOST" "${REQ}" >/dev/null || \
	{ ${ECHO} "ezmlm 'remove' subject query rewriting failed"; exit 100; }

  ${ECHO} "OK"

########################
# waiting for delivery #
########################
  send_test 3
fi		# end section 2

######################################### start of section 3
if [ "$SECT" -le "3" ]; then
  wait_test 3

###############
# ezmlm-split #
###############
if [ "$QMVER" = "n" ]; then
  prompt "ezmlm-split (2/2):    "

# we know that ezmlm-manage works. A bounce would go to MODDIR, so a
# message in SINKDIR means that the request was forwarded to ezmlm-manage,
# which replied with a confirmation request.
  ${GREP} 'X-num: spl1' $SINKDIR/new/* > /dev/null 2>&1 || \
	{ ${ECHO} "failed to receive sub conf req.";
	  ${ECHO} "this could be a failure of ezmlm-split, but usually,";
	  ${ECHO} "it happens because ezmlm binaries when run by qmail";
	  ${ECHO} "don't have access to shared libraries required for";
	  ${ECHO} "RDBMS access. This happens on systems where RDBMS";
	  ${ECHO} "shared libs are installed in the /usr/local hierarchy.";
	  ${ECHO} "fix: see ld.so man page on how to modify /etc/ld.so.conf";
	  ${ECHO} "or compile statically by adding -static to conf-sqlld.";
	  ${ECHO}
	  exit 100; }

  ${ECHO} "OK"
fi

##################
# ezmlm-moderate #
##################

  prompt "ezmlm-moderate (2/2): "

  MOD1=`${GREP} -l "mod1" $SINKDIR/new/* | head -1` || \
	{ ${ECHO} "failed to send rejection notice for message mod1"; exit 100; }

# ${SND}@$HOST means it was rejected, not send through the list
  ${GREP} "To: ${SND}@$HOST" "$MOD1" > /dev/null 2>&1 || \
	{ ${ECHO} "failed to reject message mod1"; exit 100; }

  MOD3=`${GREP} -l "mod3" $SINKDIR/new/* | head -1`
  if [ -z "$MOD3" ]; then
    ${ECHO} "failed to post message mod3"
    exit 100
  fi

# ${LOC}@$HOST means it was not rejected, but sent through the list
  ${GREP} "To: ${LOC}@$HOST" "$MOD3" > /dev/null 2>&1 || \
	{ ${ECHO} "failed to reject message mod3"; exit 100; }

  ${ECHO} "OK"

################
# ezmlm-manage #
################
  prompt "ezmlm-manage (3/4):   "

  SENDER="${MOD}@$HOST"; export SENDER
  ${EZBIN}/ezmlm-issubn "${DIR}" && \
	{ ${ECHO} "unsub without mod for moderated list failed"; exit 100; }

  SUB3=`${GREP} -l 'sub3' $MODDIR/new/*` || \
	{ ${ECHO} "failed getting subscribe moderation confirm request"; \
	 exit 100; }

# confirm subscription request
  LOCAL=`${GREP} "Reply-To:" "$SUB3" | cut -d' ' -f2 | cut -d'@' -f1` || \
	{ ${ECHO} "no confirm address in sub3 mod confirm request"; exit 100; }
  export LOCAL
  if [ "$QMVER" = "n" ]; then
	DEFAULT=`${ECHO} "$LOCAL" | cut -c"$LOCLEN"-`; export DEFAULT
  fi
  ${ECHO} "X-num: modR1" > "${DIR}/__tmp"
  ${ECHO} "FROM: moderator agrees <$SENDER>" >> "${DIR}/__tmp"
  ${ECHO} >> "${DIR}/__tmp"
  ${EZBIN}/ezmlm-manage ${SW_FROM} "${DIR}" < "${DIR}/__tmp"\
		>/dev/null 2>&1 || \
	{ ${ECHO} "failed to send digest sub mod accept for sub3"; exit 100; }

# complete edit. SENDER can be any address
  SENDER="${MAN}@$HOST"; export SENDER
  EDIT3=`${GREP} -l 'edit3' $MODDIR/new/*` || \
	{ ${ECHO} "failed getting edit reply for edit3"; \
	 exit 100; }
  ${GREP} "#TEST_TEXT#" "$EDIT3" >/dev/null 2>&1 || \
	{ ${ECHO} "old text missing in edit3 edit reply"; exit 100; }
  LOCAL=`${GREP} "Reply-To:" "$EDIT3" | cut -d' ' -f2 | cut -d'@' -f1` || \
	{ ${ECHO} "no reply address in edit3 edit reply"; exit 100; }
  export LOCAL
  if [ "$QMVER" = "n" ]; then
	DEFAULT=`${ECHO} "$LOCAL" | cut -c"$LOCLEN"-`; export DEFAULT
  fi
  ${ECHO} "X-num: edit4" > "${DIR}/__tmp"
  ${ECHO} >> "${DIR}/__tmp"
  ${ECHO} "%%% START OF TEXT FILE" >> "${DIR}/__tmp"
  ${ECHO} "#NEW_TEXT#" >> "${DIR}/__tmp"
  ${ECHO} "%%% END OF TEXT FILE" >> "${DIR}/__tmp"
  ${EZBIN}/ezmlm-manage -e "${DIR}" < "${DIR}/__tmp" >/dev/null 2>&1 || \
	{ ${ECHO} "failed to send edit4 reply for edit3"; exit 100; }

# check results of log/list
  LOG3=`${GREP} -l 'log3' $MODDIR/new/*` || \
	{ ${ECHO} "failed getting -log reply to log3"; \
	 exit 100; }
  ${GREP} "aaa@bbb" "$LOG3" | ${GREP} "+m" > /dev/null 2>&1 || \
	{ ${ECHO} "failed to get log reply to log3"; exit 100; }

  LIST3=`${GREP} -l 'list3' $MODDIR/new/*` || \
	{ ${ECHO} "failed getting -list reply to list3"; \
	 exit 100; }
  ${GREP} "aaa@bbb" "$LIST3" > /dev/null 2>&1 || \
	{ ${ECHO} "failed to get list reply to list3"; exit 100; }

  ${ECHO} "OK"

#############
# ezmlm-get #
#############
  prompt "ezmlm-get (2/2):      "

# index1/get1/thread1 should bounce and will not be looked for
# index2 ... should be in DIG@HOST's inbox
# get3 - r format to DIG@HST
# get4 - n
# get5 - v
# get6 - x

# well - just a consistency check
  ${GREP} "index1" ${DIGDIR}/new/* >/dev/null 2>&1 && \
	{ ${ECHO} "index1 found in wrong mailbox"; exit 100; }

# now check that they've been delivered. We don't check the formats,
# as this would be quite involved.
  ${GREP} "index2" ${DIGDIR}/new/* >/dev/null 2>&1 || \
	{ ${ECHO} "index2 failed to return"; exit 100; }
  ${GREP} "get2" ${DIGDIR}/new/* >/dev/null 2>&1 || \
	{ ${ECHO} "get2 failed to return"; exit 100; }
  ${GREP} "get3" ${DIGDIR}/new/* >/dev/null 2>&1 || \
	{ ${ECHO} "get3 format 'r' failed to return"; exit 100; }
  ${GREP} "get4" ${DIGDIR}/new/* >/dev/null 2>&1 || \
	{ ${ECHO} "get3 format 'n' failed to return"; exit 100; }
  ${GREP} "get5" ${DIGDIR}/new/* >/dev/null 2>&1 || \
	{ ${ECHO} "get3 format 'v' failed to return"; exit 100; }
  ${GREP} "get6" ${DIGDIR}/new/* >/dev/null 2>&1 || \
	{ ${ECHO} "get3 format 'x' failed to return"; exit 100; }

  ${GREP} "dig1" ${DIGDIR}/new/* >/dev/null 2>&1 || \
	{ ${ECHO} "dig1 from manager wasn't delivered"; exit 100; }
  ${GREP} "dig2" ${DIGDIR}/new/* >/dev/null 2>&1 || \
	{ ${ECHO} "dig2 from editor wasn't delivered"; exit 100; }
  ${GREP} "dig3" ${DIGDIR}/new/* >/dev/null 2>&1 || \
	{ ${ECHO} "dig3 from command line wasn't delivered"; exit 100; }
  ${GREP} "dig4" ${DIGDIR}/new/* >/dev/null 2>&1 || \
	{ ${ECHO} "dig4 format 'r' wasn't delivered"; exit 100; }
  ${GREP} "dig5" ${DIGDIR}/new/* >/dev/null 2>&1 || \
	{ ${ECHO} "dig5 format 'n' wasn't delivered"; exit 100; }
  ${GREP} "dig6" ${DIGDIR}/new/* >/dev/null 2>&1 || \
	{ ${ECHO} "dig6 format 'x' wasn't delivered"; exit 100; }
  ${GREP} "dig6" ${DIGDIR}/new/* >/dev/null 2>&1 || \
	{ ${ECHO} "dig6 format 'v' wasn't delivered"; exit 100; }

  ${ECHO} "OK"


########################
# waiting for delivery #
########################
  send_test 4
fi			# end section 3

####################################### start of section 4
if [ "$SECT" -le "4" ]; then
  wait_test 4

##############
# ezmlm-warn #
##############
  prompt "ezmlm-warn (3/3):     "

  SENDER="${BNC}@${HOST}"
  export SENDER
  ${EZBIN}/ezmlm-issubn -n "${DIR}" || \
	{ ${ECHO} "failed to remove bouncing subscriber"; exit 100; }
  ${EZBIN}/ezmlm-issubn -n "${DIR}/digest" || \
	{ ${ECHO} "failed to remove bouncing digest subscriber"; exit 100; }

  ${ECHO} "OK"

################
# ezmlm-manage #
################
  prompt "ezmlm-manage (4/4):   "

  ${GREP} "#NEW_TEXT#" "${DIR}/text/test" >/dev/null 2>&1 || \
	{ ${ECHO} "edit4 failed to update text file"; exit 100; }

  ${ECHO} "OK"

fi			# end section 4

########################## start of section 9 (cleanup)
if [ "$SECT" -eq "9" -o -z "$DEBUG" ]; then

#####################
# remove test files #
#####################


# cleanup the mysql sub tables so we can repeat if necessary
# the Log test will pass due to old data once we access the mysql log,
# rather than the file, but what the ...
  if [ $USESQL ]; then
	${EZBIN}/ezmlm-unsub "${DIR}/digest" "${MAN}@$HOST" "${DIG}@$HOST" \
		>/dev/null 2>&1
	${EZBIN}/ezmlm-unsub "${DIR}/mod" "${MOD}@$HOST" \
		>/dev/null 2>&1
	${EZBIN}/ezmlm-unsub "${DIR}/${ALLOW}" "aaa@bbb" "ccc@ddd" "eee@fff" \
		>/dev/null 2>&1
  fi
  ${RM} -rf "${DIR}" ${DOT}* "${ERR}" >/dev/null 2>&1

fi
${ECHO}
if [ ! -z "${BUG}" ]; then
  ${ECHO} "${BUG}" | ${GREP} "config" >/dev/null 2>&1 && \
    {
	${ECHO}
	${ECHO} "The config bug prevents editing lists created with"
	${ECHO} "ezmlm-idx<0.31 or ezmlm-0.53. 'touch DIR/config' is a work-"
	${ECHO} "around, and upgrading to >=0.314 corrects it."
    }
  ${ECHO} "${BUG}" | ${GREP} "deny" >/dev/null 2>&1 && \
    {
	${ECHO}
    if [ "$EZVER" = '31' ]; then
	${ECHO} "The DENY bug allows users to remove themselves"
	${ECHO} "from DIR/blacklist which is not intended, but OTOH,"
	${ECHO} "DIR/blacklist is not intended for this and as a SENDER check"
	${ECHO} "inherently insecure anyway. If you need this feature and the"
	${ECHO} "bug is a problem, upgrade to >=0.321."
    else
	${ECHO} "DENY access means that subscribers can remove";
	${ECHO} "themselves from DIR/deny. This is a bug, but DENY"
	${ECHO} "is easy to circumvent and not intended to keep users from"
	${ECHO} "posting, anyway."
	${ECHO} "The bug is fixed in >=0.321."
    fi
    }
  ${ECHO} "${BUG}" | ${GREP} "return" >/dev/null 2>&1 && \
    {
	${ECHO}
	${ECHO} "The failure to add the ezmlm-return lines means"
	${ECHO} "that old lists will work correctly, but bounce handling"
	${ECHO} "won't work in lists created with this version."
	${ECHO} "The bug is fixed in >=0.321."
    }
  ${ECHO} "${BUG}" | ${GREP} "tstdig" >/dev/null 2>&1 && \
    {
	${ECHO}
	${ECHO} "The ezmlm-tstdig bug means that DIR/inlocal still needs to be"
	${ECHO} "adjusted for with digests within virtual domains."
	${ECHO} "The bug is fixed in >=0.321."
    }
  ${ECHO} "${BUG}" | ${GREP} "digest" >/dev/null 2>&1 && \
    {
	${ECHO}
	${ECHO} "The ezmlm-tstdig -digest- bug means that ezmlm-tstdig when"
	${ECHO} "in DIR/manager does not pass on digest subscribe request."
	${ECHO} "Upgrade to ezmlm-idx>=0.321 if you use ezmlm-tstdig in"
	${ECHO} "DIR/manager (this is NOT used except in custom or very"
	${ECHO} "old (ezlm-idx<0.30) digest setups)."
    }
  ${ECHO} "${BUG}" | ${GREP} "_bound" >/dev/null 2>&1 && \
    {
	${ECHO}
	${ECHO} "The ezmlm-send/reject mimeremove bug caused erroneous"
	${ECHO} "rejection of messages with text after the mime boundary in the"
	${ECHO} "Content-type header when DIR/mimeremove was used. This type"
	${ECHO} "of message is very rare (mainly Mutt with PGP MIME)."
    }
  ${ECHO} "${BUG}" | ${GREP} "_noself" >/dev/null 2>&1 && \
    {
	${ECHO}
	${ECHO} "The ezmlm-send -C switch 'not to sender' is no longer" 
	${ECHO} "supported. For backwards compatibility the switch is"
	${ECHO} "ignored. Instead, implement this feature in the recipients"
	${ECHO} "mailbox by rejecting messages from the list with the"
	${ECHO} "recipient's address in the From: header."
    }
  ${ECHO} "${BUG}" | ${GREP} "_signed" >/dev/null 2>&1 && \
    {
	${ECHO}
	${ECHO} "The trailer is added as a separate MIME part to multipart"
	${ECHO} "messages, but should be suppressed not only for multipart"
	${ECHO} "alternative, but also for many other multipart types,"
	${ECHO} "including multipart/signed."
    }
    ${ECHO}
fi

exit 0


