# above should be a definition of EZPATH and the /bin/sh line
# automatically added at build time.
#########################################################################
# Script to build ezmlm-request config file for the global interface
#	Lists are presented in alphabetical order and digest lists are
#	directly after the main list. Digest list info is only that
#	they are digest of the main list. If the -n switch is used,
#	the current number of subscribers is added to the info text.
#
# Usage: ezmlm-glconf [-n] dotdir [dotdir1 ...] > config
#	where ``dotdir'' is the directory where the users .qmail files
#	reside (usually ``~''), and ``config'' is the config file for
#	ezmlm-request servicing the global interface.
#
#	Note: This config file will provide info on all lists available
#	and allow ``which'' for all lists. You may want to edit the
#	output before using it.
##########################################################################

# Change if you change the name of the command
NAME='ezmlm-glconf'

# Info text used for list-digest
DIGTXT='Digest of'
# ``subscribers'' for stating subscriber number
SUBTXT='subscribers'
# Info text if dir/text/info doesn't exist/is not readable
INFOTXT='No information available'

# Set full path if you use the script as root or if the commands
# are not in your path.
HEAD='head'
TAIL='tail'
CAT='cat'
CUT='cut'
GREP='grep'
ECHO='echo'
LS='ls'
SORT='sort'
SED='sed'
WC='wc'

# Nothing more to configure
##########################################################################

SUB=''
EZLIST="${EZPATH}/ezmlm-list"
FATAL="${NAME}: fatal: "
WARN="${NAME}: warning: "
USAGE="${NAME} [-n] dotdir [dotdir1 ...] > config"

if [ "$1" = "-n" ]; then
	shift
	if [ ! -x "${EZLIST}" ]; then
		echo "${WARN} ezmlm-list not found. Edit ezmlm binary path."
	else
		SUB='yes'
	fi
fi

if [ -z "$1" ]; then
	${ECHO} "$USAGE"
	exit 100
fi


(
while [ -n "$1" ]; do
  for i in `${LS} $1/.qmail-*-return-default` ; do
	INFO=''
					# get list directory
	if [ -r "$i" ] ; then
		DIR=`${TAIL} -1 "$i" | ${CUT} -d\' -f2`
					# only dir that exists
		if [ -d "$DIR" ] ; then
			INFOFN="${DIR}/text/info"
			if [ -r "${INFOFN}" ]; then
				INFO=`${HEAD} -1 ${INFOFN}`
			else
				INFO="$INFOTXT"
				${ECHO} "$WARN No info for list in $DIR" 1>&2
			fi

			OUTLOCAL=`${CAT} $DIR/outlocal`
			OUTHOST=`${CAT} $DIR/outhost`
			if [ -z "$SUB" ]; then
				SUBN=''
			else
				NO=`${EZLIST} ${DIR} | ${WC} -l |${SED} 's/ //g'`
				SUBN=" (${NO} ${SUBTXT})"
			fi
			${ECHO} "$i" | ${GREP} 'digest-return' >/dev/null && \
				{ INFO="${DIGTXT} $OUTLOCAL@$OUTHOST."; \
				OUTLOCAL="${OUTLOCAL}~digest" ; }
			${ECHO} "$OUTLOCAL@$OUTHOST:$DIR:${INFO}${SUBN}"
		else
			${ECHO} "$WARN $DIR not readable - list ignored" 1>&2
		fi
	else
		${ECHO} "$WARN $i ignored: doesn't point to readable file" 1>&2
	fi
  done ;
  shift
done;
) | ${SORT} | ${SED} 's/~digest@/-digest@/'
					# list-digest after list
					# "~"-cludge needed to get order right
