#!/bin/sh
# Simple script to generate input to mysql to generate tables for a list
# All tables are created, even though it is not advisable to put e.g. the
# moderator table in the SQL database nor is it very useful to put the
# blacklist/deny table there. The subscriber lists for the main and digest
# lists should be there, and it's reasonable to put the "extra" list there
# if used.
ECHO='echo'
CAT='cat'
CUT='cut'


CREATE='y'
DROP='n'
TROOT='list'
# size of std cookie
COOKIE='20'

					# not everyone has getopt :-(
while [ "`${ECHO} "$1" | ${CUT} -c1`" = "-" ]; do
	case "$1" in
		-c)	CREATE='y'; shift;;
		-C)	CREATE='n'; shift;;
		-d)	DROP='y'; shift;;
		-D)	DROP='n'; shift;;
		-cd|-dc)	CREATE='y'; DROP='y'; shift;;
		-cD|-Dc)	CREATE='y'; DROP='n'; shift;;
		-Cd|-dC)	CREATE='n'; DROP='y'; shift;;
		-CD|-DC)	CREATE='n'; DROP='n'; shift;;
		--)	shift; break;;
		*)	echo "usage: emzlm-mktab [-cCdD] table_toot"; exit 100;;
	esac
done

[ ! -z "$1" ] && TROOT="$1";


if [ "$DROP" = "y" ]; then
  cat <<EOF

/* drop old tables. This may fail unless you use mysql -f */
/* Usage: */
/* ezmlm-mktab [-d] troot | mysql -hhost -uuserid -ppw datab -f */

DROP TABLE ${TROOT};
DROP TABLE ${TROOT}_slog;
DROP TABLE ${TROOT}_digest;
DROP TABLE ${TROOT}_digest_slog;
DROP TABLE ${TROOT}_mod;
DROP TABLE ${TROOT}_mod_slog;
DROP TABLE ${TROOT}_allow;
DROP TABLE ${TROOT}_allow_slog;
DROP TABLE ${TROOT}_deny;
DROP TABLE ${TROOT}_deny_slog;
/* eliminated name table - no need */
DROP TABLE ${TROOT}_cookie;
DROP TABLE ${TROOT}_mlog;
DROP TABLE ${TROOT}_digest_cookie;
DROP TABLE ${TROOT}_digest_mlog;

EOF

fi

if [ $CREATE = 'y' ]; then
  cat << EOF

/* Main address table */
/* Need varchar. Domain = 3 chars => fixed length, as opposed to varchar */
/* Always select on domain and hash, so that one index should do         */
/* primary key(address) is very inefficient for MySQL. */
/* MySQL tables do not need a primary key. Other RDBMS require one. For  */
/* the log tables, just add an INT AUTO_INCREMENT. For the address table,*/
/* do that or use address as a primary key. */

create TABLE ${TROOT} (
	hash		TINYINT UNSIGNED NOT NULL,
	address		VARCHAR(255) NOT NULL,
	INDEX h (hash),
	INDEX a (address(12)));

/* Subscription log table. No addr idx to make insertion fast, since that is */
/* almost the only thing we do with this table */
create TABLE ${TROOT}_slog (
	tai		TIMESTAMP,
	address		VARCHAR(255) NOT NULL,
	fromline	VARCHAR(255) NOT NULL,
	edir		CHAR(1) NOT NULL,
	etype		CHAR(1) NOT NULL,	
	INDEX (tai));

/* digest list table */
create TABLE ${TROOT}_digest (
	hash		TINYINT UNSIGNED NOT NULL,
	address		VARCHAR(255) NOT NULL,
	INDEX h (hash),
	INDEX a (address(12)));

/* digest list subscription log */
create TABLE ${TROOT}_digest_slog (
	tai		TIMESTAMP,
	address		VARCHAR(255) NOT NULL,
	fromline	VARCHAR(255) NOT NULL,
	edir		CHAR(1) NOT NULL,
	etype		CHAR(1) NOT NULL,	
	INDEX (tai));

/* moderator addresses */
create TABLE ${TROOT}_mod (
	hash		TINYINT UNSIGNED NOT NULL,
	address		VARCHAR(255) NOT NULL,
	INDEX h(hash),
	INDEX a(address(12)));

/* moderator subscription log */
create TABLE ${TROOT}_mod_slog (
	tai		TIMESTAMP,
	address		VARCHAR(255) NOT NULL,
	fromline	VARCHAR(255) NOT NULL,
	edir		CHAR(1) NOT NULL,
	etype		CHAR(1) NOT NULL,	
	INDEX (tai));

/* "allow" address table */
create TABLE ${TROOT}_allow (
	hash		TINYINT UNSIGNED NOT NULL,
	address		VARCHAR(255) NOT NULL,
	INDEX h(hash),
	INDEX a(address(12)));

/* extra address table log */
create TABLE ${TROOT}_allow_slog (
	tai		TIMESTAMP,
	address		VARCHAR(255) NOT NULL,
	fromline	VARCHAR(255) NOT NULL,
	edir		CHAR(1) NOT NULL,
	etype		CHAR(1) NOT NULL,	
	INDEX (tai));

/* blacklist address table */
create TABLE ${TROOT}_deny (
	hash		TINYINT UNSIGNED NOT NULL,
	address		VARCHAR(255) NOT NULL,
	INDEX h(hash),
	INDEX a(address(12)));

/* blacklist subscription log */
create TABLE ${TROOT}_deny_slog (
	tai		TIMESTAMP,
	address		VARCHAR(255) NOT NULL,
	fromline	VARCHAR(255) NOT NULL,
	edir		CHAR(1) NOT NULL,
	etype		CHAR(1) NOT NULL,	
	INDEX (tai));

/* main list inserts a cookie here. Sublists check it */
CREATE TABLE ${TROOT}_cookie (
	msgnum		INTEGER UNSIGNED NOT NULL,
	tai		TIMESTAMP NOT NULL,
	cookie		CHAR($COOKIE) NOT NULL,
	chunk		TINYINT UNSIGNED NOT NULL DEFAULT 0,
	bodysize	INTEGER UNSIGNED NOT NULL DEFAULT 0,
	PRIMARY KEY (msgnum));

/* main and sublist log here when the message is done */
/* done=0 for arrived, done=4 for sent, 5 for receit. */
/* tai reflects last change */
CREATE TABLE ${TROOT}_mlog (
	msgnum		INTEGER UNSIGNED NOT NULL,
	listno		INTEGER UNSIGNED NOT NULL,
	tai		TIMESTAMP,
	subs		INTEGER UNSIGNED NOT NULL DEFAULT 0,
	done		TINYINT NOT NULL DEFAULT 0,
	PRIMARY KEY listmsg (listno,msgnum,done));

/* ezmlm-get when creating a digests inserts a cookie here. Sublists check it */
CREATE TABLE ${TROOT}_digest_cookie (
	msgnum		INTEGER UNSIGNED NOT NULL,
	tai		TIMESTAMP NOT NULL,
	cookie		CHAR($COOKIE) NOT NULL,
	chunk		TINYINT UNSIGNED NOT NULL DEFAULT 0,
	bodysize	INTEGER UNSIGNED NOT NULL DEFAULT 0,
	PRIMARY KEY (msgnum));

/* ezmlm-get and digest sublists log here when the message is done */
/* done=0 for arrived, done=4 for sent, 5 for receit. */
/* tai reflects last change */
CREATE TABLE ${TROOT}_digest_mlog (
	msgnum		INTEGER UNSIGNED NOT NULL,
	listno		INTEGER UNSIGNED NOT NULL,
	tai		TIMESTAMP,
	subs		INT UNSIGNED NOT NULL DEFAULT 0,
	done		TINYINT NOT NULL DEFAULT 0,
	PRIMARY KEY listmsg (listno,msgnum,done));

EOF

fi
exit 0

