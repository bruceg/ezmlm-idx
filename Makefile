#$Id$
SHELL=/bin/sh
default: it

clean: \
TARGETS
	rm -f `cat TARGETS`

alloc.0: \
alloc.3
	nroff -man alloc.3 > alloc.0

alloc.a: \
makelib alloc.o alloc_re.o
	./makelib alloc.a alloc.o alloc_re.o

alloc.o: \
compile alloc.c alloc.h alloc.c error.h alloc.c
	./compile alloc.c

alloc_re.o: \
compile alloc_re.c alloc.h alloc_re.c byte.h alloc_re.c
	./compile alloc_re.c

author.o: \
compile author.c mime.h
	./compile author.c

auto-ccld.sh: \
conf-cc conf-ld conf-sub warn-auto.sh
	( cat warn-auto.sh; \
	sub=`head -n 1 conf-sub` ; \
	echo CC=\'`head -n 1 conf-cc` `head -n 1 sub_$$sub/conf-sqlcc`\'; \
	echo LD=\'`head -n 1 conf-ld`\' \
	) > auto-ccld.sh

auto-str: \
load auto-str.o substdio.a error.a str.a
	./load auto-str substdio.a error.a str.a 

auto-str.o: \
compile auto-str.c substdio.h auto-str.c readwrite.h auto-str.c \
exit.h auto-str.c
	./compile auto-str.c

auto_cron.c: \
auto-str conf-cron
	./auto-str auto_cron `head -n 1 conf-cron` > auto_cron.c

auto_cron.o: \
compile auto_cron.c
	./compile auto_cron.c

auto_bin.c: \
auto-str conf-bin
	./auto-str auto_bin `head -n 1 conf-bin` > auto_bin.c

auto_bin.o: \
compile auto_bin.c
	./compile auto_bin.c

auto_qmail.c: \
auto-str conf-qmail
	./auto-str auto_qmail `head -n 1 conf-qmail` > auto_qmail.c

auto_qmail.o: \
compile auto_qmail.c
	./compile auto_qmail.c

auto_version.c: \
auto-str VERSION
	./auto-str auto_version `head -n 1 VERSION` >auto_version.c

auto_version.o: \
compile auto_version.c
	./compile auto_version.c

byte_chr.o: \
compile byte_chr.c byte.h byte_chr.c
	./compile byte_chr.c

byte_copy.o: \
compile byte_copy.c byte.h byte_copy.c
	./compile byte_copy.c

byte_cr.o: \
compile byte_cr.c byte.h byte_cr.c
	./compile byte_cr.c

byte_diff.o: \
compile byte_diff.c byte.h byte_diff.c
	./compile byte_diff.c

byte_rchr.o: \
compile byte_rchr.c byte.h byte_rchr.c
	./compile byte_rchr.c

byte_zero.o: \
compile byte_zero.c byte.h byte_zero.c
	./compile byte_zero.c

case.0: \
case.3
	nroff -man case.3 > case.0

case.a: \
makelib case_diffb.o case_diffs.o case_starts.o case_lowerb.o case_startb.o
	./makelib case.a case_diffb.o case_lowerb.o case_startb.o \
	case_diffs.o case_starts.o

case_diffb.o: \
compile case_diffb.c case.h case_diffb.c
	./compile case_diffb.c

case_diffs.o: \
compile case_diffs.c case.h
	./compile case_diffs.c

case_lowerb.o: \
compile case_lowerb.c case.h case_lowerb.c
	./compile case_lowerb.c

case_startb.o: \
compile case_startb.c case.h case_startb.c
	./compile case_startb.c

checktag.o: \
compile checktag.c stralloc.h scan.h fmt.h strerr.h cookie.h \
errtxt.h subscribe.h slurp.h byte.h
	./compile checktag.c

choose: choose.sh warn-auto.sh
	rm -f choose
	cat warn-auto.sh choose.sh \
	> choose
	chmod 555 choose

case_starts.o: \
compile case_starts.c case.h
	./compile case_starts.c

compile: \
make-compile warn-auto.sh systype
	( cat warn-auto.sh; ./make-compile "`cat systype`" ) > \
	compile
	chmod 755 compile

constmap.o: \
compile constmap.c constmap.h constmap.c alloc.h constmap.c case.h \
constmap.c
	./compile constmap.c

cookie.o: \
compile cookie.c cookie.h cookie.c str.h cookie.c uint32.h cookie.c \
surfpcs.h uint32.h surfpcs.h cookie.c
	./compile cookie.c

copy.o: \
compile copy.c copy.h stralloc.h substdio.h str.h readwrite.h open.h qmail.h \
strerr.h getln.h case.h errtxt.h mime.h error.h quote.h byte.h
	./compile copy.c

date2yyyymm.o:\
compile date2yyyymm.c yyyymm.h
	./compile date2yyyymm.c

dateline.o:\
compile dateline.c yyyymm.h stralloc.h fmt.h cgi.h
	./compile dateline.c

date822fmt.o: \
compile date822fmt.c datetime.h date822fmt.c fmt.h date822fmt.c \
date822fmt.h date822fmt.c
	./compile date822fmt.c

datetime.0: \
datetime.3
	nroff -man datetime.3 > datetime.0

datetime.o: \
compile datetime.c datetime.h datetime.c
	./compile datetime.c

direntry.0: \
direntry.3
	nroff -man direntry.3 > direntry.0

direntry.h: \
compile trydrent.c direntry.h1 direntry.h2
	( ./compile trydrent.c >/dev/null 2>&1 \
	&& cat direntry.h2 || cat direntry.h1 ) > direntry.h
	rm -f trydrent.o

concatHDR.o: \
compile concatHDR.c mime.h stralloc.h strerr.h byte.h errtxt.h
	./compile concatHDR.c

decodeB.o: \
compile decodeB.c mime.h uint32.h stralloc.h strerr.h errtxt.h
	./compile decodeB.c

decodeHDR.o: \
compile decodeHDR.c mime.h stralloc.h strerr.h error.h case.h byte.h \
uint32.h errtxt.h
	./compile decodeHDR.c

decodeQ.o: \
compile decodeQ.c mime.h stralloc.h strerr.h errtxt.h
	./compile decodeQ.c

encodeB.o: \
compile encodeB.c mime.h uint32.h stralloc.h strerr.h errtxt.h
	./compile encodeB.c

encodeQ.o: \
compile encodeQ.c mime.h stralloc.h strerr.h errtxt.h
	./compile encodeQ.c

unfoldHDR.o: \
compile unfoldHDR.c mime.h stralloc.h strerr.h errtxt.h
	./compile unfoldHDR.c

env.0: \
env.3
	nroff -man env.3 > env.0

env.a: \
makelib env.o envread.o
	./makelib env.a env.o envread.o

env.o: \
compile env.c env.h str.h
	./compile env.c

envread.o: \
compile envread.c env.h envread.c str.h envread.c
	./compile envread.c

error.0: \
error.3
	nroff -man error.3 > error.0

error.a: \
makelib error.o error_str.o
	./makelib error.a error.o error_str.o

error.o: \
compile error.c error.c error.h error.c
	./compile error.c

error_str.0: \
error_str.3
	nroff -man error_str.3 > error_str.0

error_str.o: \
compile error_str.c error_str.c error.h error_str.c
	./compile error_str.c

error_temp.0: \
error_temp.3
	nroff -man error_temp.3 > error_temp.0

ezmlm-accept: \
ezmlm-accept.sh warn-auto.sh conf-bin
	(cat warn-auto.sh; \
	echo EZPATH=\'`head -n 1 conf-bin`\'; \
	cat ezmlm-accept.sh ) > ezmlm-accept

ezmlm-accept.0: \
ezmlm-accept.1
	nroff -man ezmlm-accept.1 > ezmlm-accept.0

ezmlm-archive: \
load ezmlm-archive.o auto_version.o getconf.o slurpclose.o slurp.o getln.a \
sig.a strerr.a substdio.a stralloc.a alloc.a error.a str.a fs.a open.a \
lock.a fd.a getopt.a idxthread.o yyyymm.a
	./load ezmlm-archive auto_version.o getconf.o slurpclose.o slurp.o \
	getln.a sig.a idxthread.o yyyymm.a strerr.a substdio.a stralloc.a \
	alloc.a error.a str.a fs.a open.a lock.a fd.a getopt.a

ezmlm-archive.0: \
ezmlm-archive.1
	nroff -man ezmlm-archive.1 > ezmlm-archive.0

ezmlm-archive.o: \
compile ezmlm-archive.c alloc.h error.h stralloc.h gen_alloc.h str.h \
sig.h slurp.h getconf.h strerr.h getln.h substdio.h readwrite.h lock.h \
makehash.h fmt.h strerr.h errtxt.h idx.h idxthread.h sgetopt.h subgetopt.h \
scan.h open.h
	./compile ezmlm-archive.c

ezmlm-cgi: \
load ezmlm-cgi.o getconf.o slurpclose.o slurp.o constmap.o getln.a sig.a \
mime.a strerr.a substdio.a stralloc.a alloc.a error.a str.a fs.a open.a \
lock.a fd.a getopt.a env.a case.a datetime.o now.o mime.a wait.a yyyymm.a
	./load ezmlm-cgi getconf.o slurpclose.o slurp.o constmap.o getln.a \
	mime.a sig.a env.a case.a datetime.o now.o mime.a wait.a yyyymm.a \
	strerr.a substdio.a stralloc.a alloc.a error.a str.a fs.a open.a \
	lock.a fd.a getopt.a

ezmlm-cgi.0: \
ezmlm-cgi.1
	nroff -man ezmlm-cgi.1 > ezmlm-cgi.0

ezmlm-cgi.o: \
compile ezmlm-cgi.c alloc.h direntry.h error.h stralloc.h gen_alloc.h str.h \
sig.h slurp.h getconf.h strerr.h getln.h substdio.h readwrite.h env.h \
makehash.h fmt.h strerr.h errtxt.h idx.h idxthread.h mime.h alloc.h cgi.h \
constmap.h sgetopt.h subgetopt.h datetime.h now.h fork.h wait.h
	./compile ezmlm-cgi.c

ezmlm-check: \
ezmlm-check.sh warn-auto.sh conf-bin
	(cat warn-auto.sh; \
	echo EZPATH=\'`head -n 1 conf-bin`\'; \
	echo QMPATH=\'`head -n 1 conf-qmail`\'; \
	cat ezmlm-check.sh ) > ezmlm-check

ezmlm-check.0: \
ezmlm-check.1
	nroff -man ezmlm-check.1 > ezmlm-check.0

ezmlm-clean: \
load ezmlm-clean.o auto_qmail.o auto_version.o getconf.o copy.o mime.a \
now.o datetime.o date822fmt.o slurpclose.o slurp.o qmail.o quote.o \
getln.a env.a sig.a strerr.a substdio.a stralloc.a alloc.a surf.a hdr.a \
error.a str.a fs.a case.a open.a seek.a wait.a lock.a fd.a makehash.o getopt.a
	./load ezmlm-clean auto_qmail.o auto_version.o getconf.o copy.o hdr.a \
	mime.a now.o datetime.o date822fmt.o slurpclose.o slurp.o qmail.o \
	quote.o getln.a env.a sig.a strerr.a substdio.a stralloc.a alloc.a \
	error.a str.a fs.a case.a open.a seek.a wait.a lock.a fd.a makehash.o \
	getopt.a surf.a

ezmlm-clean.0: \
ezmlm-clean.1
	nroff -man ezmlm-clean.1 > ezmlm-clean.0

ezmlm-clean.o: \
compile ezmlm-clean.c error.h stralloc.h gen_alloc.h str.h hdr.h \
env.h sig.h slurp.h getconf.h strerr.h byte.h getln.h case.h copy.h mime.h \
qmail.h substdio.h readwrite.h seek.h quote.h datetime.h now.h cookie.h \
date822fmt.h direntry.h fmt.h strerr.h errtxt.h idx.h sgetopt.h subgetopt.h \
open.h scan.h lock.h
	./compile ezmlm-clean.c

ezmlm-confirm: \
load ezmlm-confirm.o auto_qmail.o getconf.o auto_bin.o copy.o mime.a \
cookie.o now.o datetime.o date822fmt.o slurpclose.o slurp.o qmail.o quote.o \
surf.a getln.a env.a sig.a strerr.a substdio.a stralloc.a alloc.a wrap.a \
error.a str.a fs.a case.a open.a seek.a wait.a lock.a fd.a getopt.a \
auto_version.o
	./load ezmlm-confirm auto_qmail.o getconf.o copy.o mime.a \
	cookie.o now.o datetime.o date822fmt.o slurpclose.o slurp.o \
	qmail.o quote.o surf.a getln.a env.a sig.a wrap.a strerr.a \
	substdio.a stralloc.a alloc.a error.a str.a fs.a case.a \
	auto_bin.o open.a seek.a wait.a lock.a fd.a getopt.a \
	auto_version.o

ezmlm-confirm.0: \
ezmlm-confirm.1
	nroff -man ezmlm-confirm.1 > ezmlm-confirm.0

ezmlm-confirm.o: \
compile ezmlm-confirm.c error.h stralloc.h gen_alloc.h str.h \
env.h sig.h slurp.h getconf.h strerr.h byte.h getln.h case.h \
qmail.h substdio.h readwrite.h seek.h quote.h datetime.h now.h \
fmt.h strerr.h cookie.h errtxt.h idx.h copy.h mime.h open.h lock.h wrap.h \
subgetopt.h sgetopt.h auto_bin.h fork.h wait.h
	./compile ezmlm-confirm.c


ezmlm-cron: \
load ezmlm-cron.o strerr.a stralloc.a alloc.a error.a open.a auto_qmail.o \
getopt.a getln.a str.a substdio.a sig.a fs.a open.a fd.a lock.a wait.a \
case.a auto_cron.o wrap.a auto_version.o
	./load ezmlm-cron getopt.a getln.a wrap.a strerr.a substdio.a \
	stralloc.a alloc.a sig.a fs.a open.a fd.a lock.a error.a \
	wait.a case.a str.a auto_qmail.o auto_cron.o auto_version.o

ezmlm-cron.0: \
ezmlm-cron.1
	nroff -man ezmlm-cron.1 > ezmlm-cron.0

ezmlm-cron.o: \
compile ezmlm-cron.c strerr.h substdio.h stralloc.h error.h str.h sig.h \
fork.h readwrite.h wait.h errtxt.h idx.h sgetopt.h auto_qmail.h case.h \
fmt.h auto_cron.h scan.h open.h lock.h byte.h getln.h 
	./compile ezmlm-cron.c

ezmlm-gate: \
load ezmlm-gate.o subdb.a auto_bin.o getopt.a getln.a env.a sig.a strerr.a \
stralloc.a alloc.a error.a str.a case.a wait.a substdio.a open.a lock.a \
fs.a getconf.o slurpclose.o slurp.o seek.a wrap.a auto_version.o sql.lib
	./load ezmlm-gate subdb.a getconf.o slurpclose.o slurp.o \
	getopt.a getln.a auto_bin.o env.a sig.a fs.a wrap.a \
	strerr.a substdio.a stralloc.a alloc.a error.a str.a case.a wait.a \
	open.a lock.a seek.a auto_version.o `cat sql.lib`

ezmlm-gate.0: \
ezmlm-gate.1
	nroff -man ezmlm-gate.1 > ezmlm-gate.0

ezmlm-gate.o: \
compile ezmlm-gate.c idx.h errtxt.h subscribe.h auto_bin.h \
sgetopt.h subgetopt.h substdio.h getconf.h \
env.h sig.h strerr.h stralloc.h alloc.h error.h str.h case.h \
fork.h wait.h exit.h getln.h open.h
	./compile ezmlm-gate.c

ezmlm-get: \
load ezmlm-get.o idxthread.o subdb.a auto_qmail.o getopt.a now.o getconf.o \
datetime.o date822fmt.o slurpclose.o slurp.o qmail.o quote.o makehash.o \
cookie.o surf.a yyyymm.a auto_version.o hdr.a \
constmap.o getln.a env.a sig.a strerr.a substdio.a mime.a stralloc.a alloc.a \
error.a str.a fs.a case.a open.a seek.a wait.a lock.a fd.a copy.o sql.lib
	./load ezmlm-get idxthread.o subdb.a auto_qmail.o getopt.a getconf.o \
	now.o datetime.o date822fmt.o cookie.o makehash.o slurpclose.o slurp.o \
	yyyymm.a auto_version.o hdr.a \
	constmap.o substdio.a copy.o mime.a strerr.a stralloc.a alloc.a \
	qmail.o quote.o surf.a getln.a env.a sig.a \
	error.a str.a fs.a case.a \
	open.a seek.a wait.a lock.a fd.a `cat sql.lib`

ezmlm-get.o: \
compile ezmlm-get.c idx.h errtxt.h error.h getconf.h stralloc.h gen_alloc.h \
str.h cookie.h env.h sig.h slurp.h strerr.h byte.h getln.h case.h qmail.h \
substdio.h readwrite.h seek.h quote.h sgetopt.h subgetopt.h datetime.h now.h \
date822fmt.h fmt.h strerr.h copy.h errtxt.h idx.h idxthread.h mime.h \
constmap.h makehash.h hdr.h open.h lock.h scan.h
	./compile ezmlm-get.c

ezmlm-get.0: \
ezmlm-get.1
	nroff -man ezmlm-get.1 > ezmlm-get.0

ezmlm-idx: \
load ezmlm-idx.o auto_version.o slurp.o slurpclose.o mime.a wait.a \
getopt.a getln.a strerr.a sig.h sig.a open.a lock.a substdio.a stralloc.a \
alloc.a error.a str.a fd.a case.a fs.a getconf.o makehash.o surf.o mime.a
	./load ezmlm-idx auto_version.o slurp.o slurpclose.o mime.a \
	wait.a getln.a strerr.a sig.a open.a lock.a mime.a substdio.a \
	stralloc.a alloc.a error.a str.a fd.a getopt.a case.a fs.a \
	getconf.o makehash.o surf.o

ezmlm-idx.o: \
compile ezmlm-idx.c stralloc.h getconf.h byte.h \
substdio.h subfd.h strerr.h error.h sgetopt.h \
lock.h sig.h slurp.h open.h getln.h case.h \
str.h fmt.h readwrite.h exit.h idx.h mime.h errtxt.h uint32.h
	./compile ezmlm-idx.c

ezmlm-idx.0: \
ezmlm-idx.1
	nroff -man ezmlm-idx.1 > ezmlm-idx.0

ezmlm-glconf: \
ezmlm-glconf.sh warn-auto.sh conf-bin
	(cat warn-auto.sh; \
	echo EZPATH=\'`head -n 1 conf-bin`\'; \
	cat ezmlm-glconf.sh ) > ezmlm-glconf

ezmlm-glconf.0: \
ezmlm-glconf.1
	nroff -man ezmlm-glconf.1 > ezmlm-glconf.0

ezmlm-issubn: \
load ezmlm-issubn.o subdb.a getconf.o slurpclose.o slurp.o \
env.a fs.a strerr.a getln.a getopt.a sql.lib auto_version.o \
substdio.a stralloc.a alloc.a error.a str.a case.a open.a lock.a
	./load ezmlm-issubn subdb.a getconf.o slurpclose.o slurp.o \
	getopt.a env.a fs.a strerr.a \
	getln.a substdio.a stralloc.a alloc.a error.a str.a case.a \
	open.a lock.a auto_version.o `cat sql.lib`

ezmlm-issubn.0: \
ezmlm-issubn.1
	nroff -man ezmlm-issubn.1 > ezmlm-issubn.0

ezmlm-issubn.o: \
compile ezmlm-issubn.c strerr.h subscribe.h env.h errtxt.h sgetopt.h idx.h
	./compile ezmlm-issubn.c

ezmlm-limit: \
load ezmlm-limit.o getconf.o slurpclose.o slurp.o substdio.a stralloc.a \
alloc.a error.a str.a case.a open.a lock.a getopt.a fs.a sig.a now.o
	./load ezmlm-limit getconf.o slurpclose.o slurp.o getopt.a \
	strerr.a substdio.a stralloc.a alloc.a error.a str.a case.a \
	open.a lock.a fs.a sig.a now.o

ezmlm-limit.0: \
ezmlm-limit.1
	nroff -man ezmlm-limit.1 > ezmlm-limit.0

ezmlm-limit.o: \
compile ezmlm-limit.c stralloc.h strerr.h substdio.h readwrite.h sig.h lock.h \
getconf.h fmt.h now.h sgetopt.h error.h errtxt.h idx.h datetime.h scan.h \
open.h
	./compile ezmlm-limit.c

ezmlm-list: \
load ezmlm-list.o subdb.a fs.a getconf.o slurpclose.o slurp.o \
strerr.a getln.a substdio.a stralloc.a alloc.a \
error.a open.a str.a case.a getopt.a auto_version.o sql.lib
	./load ezmlm-list subdb.a fs.a getconf.o slurpclose.o slurp.o \
	strerr.a getln.a getopt.a substdio.a stralloc.a \
	alloc.a error.a open.a str.a case.a auto_version.o `cat sql.lib`

ezmlm-list.0: \
ezmlm-list.1
	nroff -man ezmlm-list.1 > ezmlm-list.0

ezmlm-list.o: \
compile ezmlm-list.c stralloc.h gen_alloc.h substdio.h getln.h strerr.h \
error.h readwrite.h exit.h open.h errtxt.h subscribe.h exit.h sgetopt.h \
idx.h fmt.h
	./compile ezmlm-list.c

ezmlm-make: \
load ezmlm-make.o auto_bin.o open.a getln.a getopt.a substdio.a strerr.a \
stralloc.a alloc.a error.a lock.a str.a auto_version.o
	./load ezmlm-make auto_bin.o open.a getln.a getopt.a substdio.a \
	strerr.a stralloc.a alloc.a error.a lock.a str.a auto_version.o

ezmlm-make.0: \
ezmlm-make.1
	nroff -man ezmlm-make.1 > ezmlm-make.0

ezmlm-make.o: \
compile ezmlm-make.c ezmlm-make.c ezmlm-make.c sgetopt.h subgetopt.h \
sgetopt.h ezmlm-make.c stralloc.h gen_alloc.h stralloc.h ezmlm-make.c \
strerr.h ezmlm-make.c exit.h ezmlm-make.c readwrite.h ezmlm-make.c \
open.h ezmlm-make.c substdio.h ezmlm-make.c str.h ezmlm-make.c \
auto_bin.h ezmlm-make.c ezmlm-make.c ezmlm-make.c ezmlm-make.c \
errtxt.h idx.h getln.h lock.h
	./compile ezmlm-make.c

ezmlm-manage: \
load ezmlm-manage.o auto_qmail.o auto_version.o getconf.o subdb.a log.o \
makehash.o cookie.o now.o datetime.o date822fmt.o slurpclose.o slurp.o \
qmail.o quote.o surf.a getln.a env.a sig.a strerr.a substdio.a stralloc.a \
alloc.a error.a str.a fs.a case.a open.a seek.a wait.a lock.a fd.a getopt.a \
mime.a copy.o auto_version.o hdr.a sql.lib
	./load ezmlm-manage subdb.a auto_qmail.o getconf.o copy.o makehash.o \
	mime.a log.o cookie.o now.o datetime.o date822fmt.o slurpclose.o \
	slurp.o qmail.o quote.o surf.a getln.a env.a sig.a strerr.a \
	substdio.a stralloc.a alloc.a error.a str.a fs.a case.a hdr.a \
	open.a seek.a wait.a lock.a fd.a getopt.a auto_version.o \
	`cat sql.lib`

ezmlm-manage.0: \
ezmlm-manage.1
	nroff -man ezmlm-manage.1 > ezmlm-manage.0

ezmlm-manage.o: \
compile ezmlm-manage.c ezmlm-manage.c ezmlm-manage.c error.h \
ezmlm-manage.c stralloc.h gen_alloc.h stralloc.h ezmlm-manage.c str.h \
ezmlm-manage.c env.h ezmlm-manage.c sig.h ezmlm-manage.c slurp.h \
ezmlm-manage.c getconf.h ezmlm-manage.c strerr.h ezmlm-manage.c \
byte.h ezmlm-manage.c getln.h ezmlm-manage.c case.h ezmlm-manage.c \
qmail.h substdio.h qmail.h ezmlm-manage.c substdio.h substdio.h \
ezmlm-manage.c readwrite.h ezmlm-manage.c seek.h ezmlm-manage.c \
quote.h ezmlm-manage.c datetime.h ezmlm-manage.c now.h datetime.h \
datetime.h now.h ezmlm-manage.c ezmlm-manage.c fmt.h open.h lock.h scan.h \
ezmlm-manage.c subscribe.h strerr.h strerr.h subscribe.h mime.h \
sgetopt.h subgetopt.h cookie.h idx.h errtxt.h copy.h hdr.h
	./compile ezmlm-manage.c

ezmlm-mktab.0: \
ezmlm-mktab.1
	nroff -man ezmlm-mktab.1 > ezmlm-mktab.0

ezmlm-moderate: \
load ezmlm-moderate.o auto_qmail.o getconf.o auto_bin.o copy.o mime.a \
cookie.o now.o datetime.o date822fmt.o slurpclose.o slurp.o qmail.o quote.o \
surf.a getln.a env.a sig.a strerr.a substdio.a stralloc.a alloc.a wrap.a \
error.a str.a fs.a case.a open.a seek.a wait.a lock.a fd.a getopt.a hdr.a \
makehash.o auto_version.o
	./load ezmlm-moderate auto_qmail.o getconf.o copy.o mime.a hdr.a \
	makehash.o cookie.o now.o datetime.o date822fmt.o slurpclose.o \
	slurp.o qmail.o quote.o surf.a getln.a env.a sig.a wrap.a strerr.a \
	substdio.a stralloc.a alloc.a error.a str.a fs.a case.a \
	auto_bin.o open.a seek.a wait.a lock.a fd.a getopt.a \
	auto_version.o

ezmlm-moderate.0: \
ezmlm-moderate.1
	nroff -man ezmlm-moderate.1 > ezmlm-moderate.0

ezmlm-moderate.o: \
compile ezmlm-moderate.c error.h stralloc.h gen_alloc.h str.h open.h \
env.h sig.h slurp.h getconf.h strerr.h byte.h getln.h case.h hdr.h \
qmail.h substdio.h readwrite.h seek.h quote.h datetime.h now.h lock.h \
fmt.h strerr.h cookie.h errtxt.h idx.h copy.h mime.h \
subgetopt.h sgetopt.h auto_bin.h fork.h wait.h
	./compile ezmlm-moderate.c

ezmlm-request: \
load ezmlm-request.o subdb.a getconf.o constmap.o getln.a auto_qmail.o qmail.o \
strerr.a slurpclose.o slurp.o getopt.a env.a open.a fd.a sig.a case.a hdr.a \
substdio.a error.a stralloc.a alloc.a str.a case.a fs.a wait.a seek.a surf.a \
makehash.o date822fmt.o now.o datetime.o quote.o copy.o mime.a sql.lib
	./load ezmlm-request subdb.a getconf.o constmap.o getln.a auto_qmail.o \
	qmail.o date822fmt.o datetime.o now.o quote.o auto_version.o hdr.a \
	slurpclose.o slurp.o env.a open.a sig.a wait.a getopt.a makehash.o \
	strerr.a substdio.a error.a copy.o stralloc.a alloc.a substdio.a \
	surf.a str.a case.a fs.a fd.a sig.a wait.a seek.a mime.a `cat sql.lib`

ezmlm-request.0:
	nroff -man ezmlm-request.1 > ezmlm-request.0

ezmlm-request.o: \
compile ezmlm-request.c stralloc.h subfd.h strerr.h error.h qmail.h env.h \
sig.h open.h getln.h case.h str.h readwrite.h exit.h substdio.h quote.h \
getconf.h constmap.h fmt.h byte.h errtxt.h idx.h datetime.h mime.h \
subscribe.h now.h copy.h cookie.h
	./compile ezmlm-request.c

ezmlm-reject: \
load ezmlm-reject.o getln.a strerr.a substdio.a error.a stralloc.a open.a \
qmail.o env.a seek.a fd.a wait.a auto_qmail.o auto_version.o \
alloc.a getconf.o slurp.o slurpclose.o str.a getopt.a case.a constmap.o fs.a
	./load ezmlm-reject qmail.o getln.a strerr.a substdio.a error.a fs.a \
	env.a constmap.o getconf.o slurp.o slurpclose.o stralloc.a alloc.a \
	seek.a str.a getopt.a case.a open.a fd.a wait.a auto_qmail.o \
	auto_version.o

ezmlm-reject.0: \
ezmlm-reject.1
	nroff -man ezmlm-reject.1 > ezmlm-reject.0

ezmlm-reject.o: \
compile ezmlm-reject.c strerr.h substdio.h readwrite.h stralloc.h gen_alloc.h \
stralloc.h getln.h sgetopt.h subgetopt.h constmap.h getconf.h errtxt.h \
scan.h fmt.h idx.h qmail.h env.h seek.h byte.h case.h str.h
	./compile ezmlm-reject.c

ezmlm-return: \
load ezmlm-return.o quote.o getconf.o subdb.a log.o \
slurpclose.o slurp.o now.o cookie.o surf.a lock.a env.a sig.a \
strerr.a getln.a substdio.a stralloc.a alloc.a error.a str.a fs.a \
case.a open.a sql.lib
	./load ezmlm-return quote.o getconf.o subdb.a \
	log.o slurpclose.o slurp.o now.o cookie.o surf.a lock.a \
	env.a sig.a strerr.a getln.a substdio.a stralloc.a alloc.a \
	error.a str.a fs.a case.a open.a `cat sql.lib`

ezmlm-return.0: \
ezmlm-return.1
	nroff -man ezmlm-return.1 > ezmlm-return.0

ezmlm-return.o: \
compile ezmlm-return.c stralloc.h gen_alloc.h stralloc.h \
ezmlm-return.c str.h ezmlm-return.c env.h ezmlm-return.c sig.h \
ezmlm-return.c slurp.h ezmlm-return.c getconf.h ezmlm-return.c \
strerr.h ezmlm-return.c byte.h ezmlm-return.c case.h ezmlm-return.c \
getln.h ezmlm-return.c substdio.h ezmlm-return.c error.h direntry.h \
ezmlm-return.c quote.h ezmlm-return.c readwrite.h ezmlm-return.c \
fmt.h ezmlm-return.c now.h datetime.h now.h ezmlm-return.c cookie.h \
subscribe.h open.h scan.h lock.h slurpclose.h strerr.h
	./compile ezmlm-return.c

ezmlm-send: \
load ezmlm-send.o auto_qmail.o getconf.o qmail.o constmap.o slurp.o \
slurpclose.o wait.a getln.a strerr.a sig.a env.a open.a lock.a sql.lib hdr.a \
substdio.a cookie.o stralloc.a alloc.a error.a str.a fd.a case.a fs.a surf.a \
getopt.a copy.o mime.a subdb.a makehash.o surf.o makehash.o str.a \
quote.o auto_version.o
	./load ezmlm-send subdb.a cookie.o surf.a auto_qmail.o getconf.o \
	getopt.a qmail.o quote.o constmap.o slurp.o slurpclose.o hdr.a \
	wait.a getln.a strerr.a sig.a env.a open.a lock.a substdio.a \
	stralloc.a alloc.a error.a fd.a case.a fs.a getopt.a copy.o \
	mime.a makehash.o str.a auto_version.o `cat sql.lib`

ezmlm-send.0: \
ezmlm-send.1
	nroff -man ezmlm-send.1 > ezmlm-send.0

ezmlm-send.o: \
compile ezmlm-send.c stralloc.h gen_alloc.h copy.h \
subfd.h substdio.h strerr.h error.h qmail.h env.h makehash.h sgetopt.h \
lock.h sig.h open.h getln.h case.h scan.h str.h fmt.h readwrite.h quote.h \
exit.h getconf.h constmap.h byte.h errtxt.h idx.h mime.h subscribe.h \
uint32.h
	./compile ezmlm-send.c

ezmlm-split: \
load ezmlm-split.o auto_qmail.o getconf.o \
slurpclose.o slurp.o qmail.o quote.o wait.a \
getln.a env.a sig.a strerr.a substdio.a stralloc.a alloc.a \
error.a str.a fs.a case.a open.a fd.a
	./load ezmlm-split auto_qmail.o getconf.o slurpclose.o \
	slurp.o qmail.o quote.o getln.a env.a sig.a strerr.a \
	substdio.a stralloc.a alloc.a error.a str.a fs.a case.a \
	open.a fd.a wait.a

ezmlm-split.0: \
ezmlm-split.1
	nroff -man ezmlm-split.1 > ezmlm-split.0

ezmlm-split.o: \
compile ezmlm-split.c error.h stralloc.h gen_alloc.h str.h open.h scan.h \
env.h sig.h getconf.h strerr.h byte.h getln.h case.h \
qmail.h substdio.h  readwrite.h quote.h \
fmt.h errtxt.h idx.h uint32.h
	./compile ezmlm-split.c

ezmlm-store: \
load ezmlm-store.o auto_qmail.o getconf.o subdb.a log.o auto_bin.o mime.a \
cookie.o now.o datetime.o date822fmt.o slurpclose.o slurp.o qmail.o quote.o \
surf.a getln.a env.a sig.a strerr.a substdio.a stralloc.a alloc.a sql.lib \
error.a str.a fs.a case.a open.a seek.a wait.a lock.a fd.a getopt.a copy.o \
hdr.a wrap.a auto_version.o makehash.o
	./load ezmlm-store auto_qmail.o getconf.o subdb.a copy.o mime.a hdr.a \
	log.o cookie.o now.o datetime.o date822fmt.o slurpclose.o wrap.a \
	slurp.o qmail.o quote.o surf.a getln.a env.a sig.a strerr.a \
	substdio.a stralloc.a alloc.a error.a str.a fs.a case.a makehash.o \
	open.a seek.a wait.a lock.a fd.a getopt.a auto_bin.o \
	auto_version.o `cat sql.lib`

ezmlm-store.0: \
ezmlm-store.1
	nroff -man ezmlm-store.1 > ezmlm-store.0

ezmlm-store.o: \
compile ezmlm-store.c error.h stralloc.h gen_alloc.h str.h \
sgetopt.h subgetopt.h fork.h wait.h auto_bin.h lock.h mime.h \
env.h sig.h slurp.h getconf.h strerr.h byte.h getln.h case.h \
qmail.h substdio.h readwrite.h seek.h quote.h datetime.h now.h \
fmt.h subscribe.h strerr.h cookie.h errtxt.h idx.h copy.h
	./compile ezmlm-store.c

ezmlm-sub: \
load ezmlm-sub.o subdb.a getconf.o slurpclose.o slurp.o \
log.o now.o fs.a strerr.a getln.a getopt.a fs.a sql.lib \
substdio.a stralloc.a alloc.a error.a str.a case.a open.a lock.a \
auto_version.o
	./load ezmlm-sub subdb.a getconf.o slurpclose.o slurp.o \
	log.o now.o fs.a strerr.a getopt.a fs.a \
	getln.a substdio.a stralloc.a alloc.a error.a str.a case.a \
	open.a lock.a auto_version.o `cat sql.lib`

ezmlm-sub.0: \
ezmlm-sub.1
	nroff -man ezmlm-sub.1 > ezmlm-sub.0

ezmlm-sub.o: \
compile ezmlm-sub.c strerr.h ezmlm-sub.c subscribe.h strerr.h \
getln.h substdio.h stralloc.h readwrite.h \
strerr.h subscribe.h log.h errtxt.h sgetopt.h scan.h idx.h
	./compile ezmlm-sub.c

ezmlm-test: \
warn-auto.sh conf-qmail tests tests/* Makefile
	(cat warn-auto.sh; \
	echo QMPATH=\'`head -n 1 conf-qmail`\'; \
	cat tests/* ) >ezmlm-test;
	chmod 755 ezmlm-test

ezmlm-test.0: \
ezmlm-test.1
	nroff -man ezmlm-test.1 > ezmlm-test.0

ezmlm-tstdig: \
load ezmlm-tstdig.o getopt.a getconf.o now.o fs.a strerr.a getln.a \
lock.a substdio.a stralloc.a alloc.a error.a str.a case.a sig.a \
open.a slurpclose.o slurp.o env.a auto_version.o
	./load ezmlm-tstdig getopt.a getconf.o env.a now.o fs.a strerr.a \
	lock.a getln.a substdio.a stralloc.a alloc.a error.a str.a case.a \
	sig.a slurpclose.o slurp.o open.a auto_version.o

ezmlm-tstdig.0: \
ezmlm-tstdig.1
	nroff -man ezmlm-tstdig.1 > ezmlm-tstdig.0

ezmlm-tstdig.o: \
compile ezmlm-tstdig.c strerr.h sgetopt.h getconf.h \
sig.h now.h errtxt.h stralloc.h sig.h env.h fmt.h substdio.h readwrite.h \
now.h idx.h scan.h case.h str.h open.h
	./compile ezmlm-tstdig.c

ezmlm-unsub: \
load ezmlm-unsub.o subdb.a getconf.o slurpclose.o slurp.o \
log.o now.o fs.a strerr.a getln.a getopt.a fs.a sql.lib \
substdio.a stralloc.a alloc.a error.a str.a case.a open.a lock.a \
auto_version.o
	./load ezmlm-unsub subdb.a getopt.a getconf.o slurpclose.o slurp.o \
	log.o now.o fs.a strerr.a fs.a \
	getln.a substdio.a stralloc.a alloc.a error.a str.a case.a \
	open.a lock.a auto_version.o `cat sql.lib`

ezmlm-unsub.0: \
ezmlm-unsub.1
	nroff -man ezmlm-unsub.1 > ezmlm-unsub.0

ezmlm-unsub.o: \
compile ezmlm-unsub.c strerr.h subscribe.h \
log.h errtxt.h sgetopt.h scan.h idx.h readwrite.h stralloc.h substdio.h 
	./compile ezmlm-unsub.c

ezmlm-warn: \
load ezmlm-warn.o auto_qmail.o getconf.o mime.a cookie.o makehash.o subdb.a \
now.o slurpclose.o slurp.o quote.o datetime.o date822fmt.o qmail.o surf.a \
case.a strerr.a sig.a getln.a substdio.a stralloc.a alloc.a error.a hdr.a \
open.a lock.a str.a fs.a fd.a wait.a env.a copy.o getopt.a auto_version.o \
sql.lib
	./load ezmlm-warn auto_qmail.o getconf.o mime.a hdr.a makehash.o \
	cookie.o subdb.a getopt.a now.o slurpclose.o slurp.o quote.o \
	datetime.o date822fmt.o qmail.o surf.a case.a strerr.a sig.a \
	getln.a substdio.a stralloc.a alloc.a error.a open.a lock.a \
	env.a str.a fs.a fd.a wait.a copy.o auto_version.o `cat sql.lib`

ezmlm-warn.0: \
ezmlm-warn.1
	nroff -man ezmlm-warn.1 > ezmlm-warn.0

ezmlm-warn.o: \
compile ezmlm-warn.c direntry.h readwrite.h getln.h open.h scan.h lock.h \
substdio.h stralloc.h gen_alloc.h slurp.h getconf.h byte.h error.h str.h \
sig.h now.h datetime.h fmt.h cookie.h qmail.h substdio.h quote.h copy.h \
mime.h idx.h errtxt.h sgetopt.h subgetopt.h
	./compile ezmlm-warn.c

ezmlm-weed: \
load ezmlm-weed.o getln.a strerr.a substdio.a error.a case.a stralloc.a \
alloc.a str.a
	./load ezmlm-weed getln.a strerr.a substdio.a error.a \
	case.a stralloc.a alloc.a str.a 

ezmlm-weed.0: \
ezmlm-weed.1
	nroff -man ezmlm-weed.1 > ezmlm-weed.0

ezmlm-weed.o: \
compile ezmlm-weed.c stralloc.h gen_alloc.h stralloc.h ezmlm-weed.c \
str.h ezmlm-weed.c byte.h ezmlm-weed.c readwrite.h ezmlm-weed.c case.h \
substdio.h ezmlm-weed.c getln.h ezmlm-weed.c strerr.h ezmlm-weed.c
	./compile ezmlm-weed.c

ezmlm.0: \
ezmlm.5
	nroff -man ezmlm.5 > ezmlm.0

ezmlmglrc.0: \
ezmlmglrc.5
	nroff -man ezmlmglrc.5 > ezmlmglrc.0

ezmlmrc.0: \
ezmlmrc.5
	nroff -man ezmlmrc.5 > ezmlmrc.0

ezmlmrc.all: \
ezmlmrc.ch_GB ezmlmrc.cs ezmlmrc.da ezmlmrc.de ezmlmrc.en_US \
ezmlmrc.es ezmlmrc.fr ezmlmrc.hu ezmlmrc.id ezmlmrc.it ezmlmrc.jp \
ezmlmrc.nl ezmlmrc.pl ezmlmrc.pt ezmlmrc.pt_BR ezmlmrc.ru ezmlmrc.sv

ezmlmrc.ch_GB: \
makelang ezmlmrc.template lang/ch_GB.text lang/ch_GB.sed
	./makelang ch_GB

ezmlmrc.cs: \
makelang ezmlmrc.template lang/cs.text lang/cs.sed
	./makelang cs

ezmlmrc.da: \
makelang ezmlmrc.template lang/da.text lang/da.sed
	./makelang da

ezmlmrc.de: \
makelang ezmlmrc.template lang/de.text lang/de.sed
	./makelang de

ezmlmrc.en_US: \
makelang ezmlmrc.template lang/en_US.text lang/en_US.sed
	./makelang en_US

ezmlmrc.es: \
makelang ezmlmrc.template lang/es.text lang/es.sed
	./makelang es

ezmlmrc.fr: \
makelang ezmlmrc.template lang/fr.text lang/fr.sed
	./makelang fr

ezmlmrc.hu: \
makelang ezmlmrc.template lang/hu.text lang/hu.sed
	./makelang hu

ezmlmrc.id: \
makelang ezmlmrc.template lang/id.text lang/id.sed
	./makelang id

ezmlmrc.it: \
makelang ezmlmrc.template lang/it.text lang/it.sed
	./makelang it

ezmlmrc.jp: \
makelang ezmlmrc.template lang/jp.text lang/jp.sed
	./makelang jp

ezmlmrc.nl: \
makelang ezmlmrc.template lang/nl.text lang/nl.sed
	./makelang nl

ezmlmrc.pl: \
makelang ezmlmrc.template lang/pl.text lang/pl.sed
	./makelang pl

ezmlmrc.pt: \
makelang ezmlmrc.template lang/pt.text lang/pt.sed
	./makelang pt

ezmlmrc.pt_BR: \
makelang ezmlmrc.template lang/pt_BR.text lang/pt_BR.sed
	./makelang pt_BR

ezmlmrc.ru: \
makelang ezmlmrc.template lang/ru.text lang/ru.sed
	./makelang ru

ezmlmrc.sv: \
makelang ezmlmrc.template lang/sv.text lang/sv.sed
	./makelang sv

ezmlmsubrc.0: \
ezmlmsubrc.5
	nroff -man ezmlmsubrc.5 > ezmlmsubrc.0

fd.a: \
makelib fd_copy.o fd_move.o
	./makelib fd.a fd_copy.o fd_move.o

fd_copy.0: \
fd_copy.3
	nroff -man fd_copy.3 > fd_copy.0

fd_copy.o: \
compile fd_copy.c fd_copy.c fd.h fd_copy.c
	./compile fd_copy.c

fd_move.0: \
fd_move.3
	nroff -man fd_move.3 > fd_move.0

fd_move.o: \
compile fd_move.c fd.h fd_move.c
	./compile fd_move.c

find-systype: \
find-systype.sh auto-ccld.sh
	cat auto-ccld.sh find-systype.sh > find-systype
	chmod 755 find-systype

fmt_str.o: \
compile fmt_str.c fmt.h fmt_str.c
	./compile fmt_str.c

fmt_uint.o: \
compile fmt_uint.c fmt.h fmt_uint.c
	./compile fmt_uint.c

fmt_uint0.o: \
compile fmt_uint0.c fmt.h fmt_uint0.c
	./compile fmt_uint0.c

fmt_ulong.o: \
compile fmt_ulong.c fmt.h fmt_ulong.c
	./compile fmt_ulong.c

fork.h: \
compile load tryvfork.c fork.h1 fork.h2
	( ( ./compile tryvfork.c && ./load tryvfork ) >/dev/null \
	2>&1 \
	&& cat fork.h2 || cat fork.h1 ) > fork.h
	rm -f tryvfork.o tryvfork

fs.a: \
makelib fmt_str.o fmt_uint.o fmt_uint0.o fmt_ulong.o scan_ulong.o \
scan_8long.o
	./makelib fs.a fmt_str.o fmt_uint.o fmt_uint0.o \
	fmt_ulong.o scan_ulong.o scan_8long.o

getconf.o: \
compile getconf.c stralloc.h gen_alloc.h stralloc.h getconf.c slurp.h \
getconf.c strerr.h getconf.c getconf.h getconf.c byte.h
	./compile getconf.c

getln.0: \
getln.3
	nroff -man getln.3 > getln.0

getln.a: \
makelib getln.o getln2.o
	./makelib getln.a getln.o getln2.o

getln.o: \
compile getln.c substdio.h getln.c byte.h getln.c stralloc.h \
gen_alloc.h stralloc.h getln.c getln.h getln.c
	./compile getln.c

getln2.0: \
getln2.3
	nroff -man getln2.3 > getln2.0

getln2.o: \
compile getln2.c substdio.h getln2.c stralloc.h gen_alloc.h \
stralloc.h getln2.c byte.h getln2.c getln.h getln2.c
	./compile getln2.c

getopt.0: \
getopt.3
	nroff -man getopt.3 > getopt.0

getopt.a: \
makelib subgetopt.o sgetopt.o
	./makelib getopt.a subgetopt.o sgetopt.o

hasflock.h: \
tryflock.c compile load
	( ( ./compile tryflock.c && ./load tryflock ) >/dev/null \
	2>&1 \
	&& echo \#define HASFLOCK 1 || exit 0 ) > hasflock.h
	rm -f tryflock.o tryflock

hassgact.h: \
trysgact.c compile load
	( ( ./compile trysgact.c && ./load trysgact ) >/dev/null \
	2>&1 \
	&& echo \#define HASSIGACTION 1 || exit 0 ) > hassgact.h
	rm -f trysgact.o trysgact

haswaitp.h: choose compile haswaitp.h1 haswaitp.h2 load trywaitp.c
	./choose cl trywaitp haswaitp.h1 haswaitp.h2 > haswaitp.h

hdr.a: \
makelib hdr_add.o hdr_boundary.o hdr_ctboundary.o hdr_datemsgid.o \
hdr_from.o hdr_mime.o hdr_transferenc.o
	./makelib hdr.a hdr_add.o hdr_boundary.o hdr_ctboundary.o \
	hdr_datemsgid.o hdr_from.o hdr_mime.o hdr_transferenc.o

hdr_add.o: \
compile hdr_add.c hdr.h qmail.h
	./compile hdr_add.c

hdr_boundary.o: \
compile hdr_boundary.c hdr.h qmail.h makehash.h
	./compile hdr_boundary.c

hdr_ctboundary.o: \
compile hdr_ctboundary.c hdr.h qmail.h
	./compile hdr_ctboundary.c

hdr_datemsgid.o: \
compile hdr_datemsgid.c hdr.h qmail.h makehash.h stralloc.h datetime.h \
fmt.h date822fmt.h
	./compile hdr_datemsgid.c

hdr_from.o: \
compile hdr_from.c hdr.h qmail.h stralloc.h quote.h
	./compile hdr_from.c

hdr_mime.o: \
compile hdr_mime.c hdr.h qmail.h makehash.h stralloc.h str.h
	./compile hdr_mime.c

hdr_transferenc.o: \
compile hdr_transferenc.c hdr.h qmail.h
	./compile hdr_transferenc.c

install: \
load install.o getln.a strerr.a substdio.a stralloc.a alloc.a open.a \
error.a str.a fs.a
	./load install getln.a strerr.a substdio.a stralloc.a \
	alloc.a open.a error.a str.a fs.a 

install.o: \
compile install.c substdio.h install.c stralloc.h gen_alloc.h \
stralloc.h install.c getln.h install.c readwrite.h install.c exit.h \
install.c open.h install.c error.h install.c strerr.h install.c \
byte.h install.c
	./compile install.c

idxthread.o: \
compile idxthread.c idxthread.h alloc.h error.h stralloc.h str.h lock.h idx.h \
substdio.h fmt.h readwrite.h idx.h errtxt.h substdio.h byte.h yyyymm.h \
open.h getln.h scan.h byte.h
	./compile idxthread.c

issub.o: \
compile issub.c stralloc.h gen_alloc.h getln.h readwrite.h substdio.h \
open.h byte.h case.h lock.h error.h subscribe.h strerr.h uint32.h fmt.h
	./compile issub.c

it: \
symlinks ezmlm-idx ezmlm-accept ezmlm-archive ezmlm-check ezmlm-gate ezmlm-get \
ezmlm-clean ezmlm-confirm ezmlm-glconf ezmlm-moderate ezmlm-store ezmlm-tstdig \
ezmlm-make ezmlm-manage ezmlm-send ezmlm-reject ezmlm-return \
ezmlm-warn ezmlm-weed ezmlm-list ezmlm-sub ezmlm-unsub ezmlm-cgi ezmlm-limit \
ezmlm-issubn ezmlm-cron ezmlm-request ezmlm-test ezmlm-split ezmlmrc.all \
ezmlmrc

load: \
make-load warn-auto.sh systype
	( cat warn-auto.sh; ./make-load "`cat systype`" ) > load
	chmod 755 load

lock.a: \
makelib lock_ex.o lock_exnb.o
	./makelib lock.a lock_ex.o lock_exnb.o

lock_ex.o: \
compile lock_ex.c hasflock.h lock.h
	./compile lock_ex.c

lock_exnb.o: \
compile lock_exnb.c hasflock.h lock.h
	./compile lock_exnb.c

log.o: \
compile log.c substdio.h log.c readwrite.h log.c stralloc.h \
gen_alloc.h stralloc.h log.c log.h log.c now.h datetime.h now.h log.c \
fmt.h log.c open.h log.c
	./compile log.c

logmsg.o: \
compile logmsg.c stralloc.h fmt.h
	./compile logmsg.c

make-compile: \
make-compile.sh auto-ccld.sh
	cat auto-ccld.sh make-compile.sh > make-compile
	chmod 755 make-compile

make-load: \
make-load.sh auto-ccld.sh
	cat auto-ccld.sh make-load.sh > make-load
	chmod 755 make-load

make-makelib: \
make-makelib.sh auto-ccld.sh
	cat auto-ccld.sh make-makelib.sh > make-makelib
	chmod 755 make-makelib

makehash.o: \
makehash.c makehash.h surf.h uint32.h stralloc.h byte.h
	./compile makehash.c

makelang: \
makelang.sh warn-auto.sh
	cat warn-auto.sh makelang.sh >makelang
	chmod 755 makelang

makelib: \
make-makelib warn-auto.sh systype
	( cat warn-auto.sh; ./make-makelib "`cat systype`" ) > \
	makelib
	chmod 755 makelib

man: \
ezmlm.0 ezmlm-gate.0 ezmlm-idx.0 ezmlm-get.0 ezmlm-check.0 ezmlm-tstdig.0 \
ezmlm-make.0 ezmlm-manage.0 ezmlm-send.0 ezmlm-reject.0 ezmlm-accept.0 \
ezmlm-return.0 ezmlm-warn.0 ezmlm-weed.0 ezmlm-list.0 ezmlm-sub.0 \
ezmlm-unsub.0 alloc.0 case.0 datetime.0 direntry.0 env.0 error.0 \
error_str.0 error_temp.0 ezmlm.0 fd_copy.0 fd_move.0 getln.0 getln2.0 \
ezmlm-issubn.0 ezmlm-cron.0 ezmlm-glconf.0 ezmlmglrc.0 ezmlm-test.0 \
ezmlmsubrc.0 ezmlm-mktab.0 ezmlm-split.0 ezmlm-archive.0 ezmlm-cgi.0 \
getopt.0 now.0 sgetopt.0 stralloc.0 subfd.0 subgetopt.0 substdio.0 \
substdio_copy.0 substdio_in.0 substdio_out.0 surf.0 surfpcs.0 wait.0 \
ezmlm-clean.0 ezmlm-moderate.0 ezmlm-store.0 ezmlm-request.0 ezmlmrc.0 \
ezmlm-limit.0 ezmlm-confirm.0

mime.a: \
makelib concatHDR.o decodeHDR.o unfoldHDR.o \
decodeQ.o encodeQ.o decodeB.o encodeB.o author.o
	./makelib mime.a concatHDR.o decodeHDR.o decodeQ.o encodeQ.o \
	decodeB.o encodeB.o unfoldHDR.o author.o

now.0: \
now.3
	nroff -man now.3 > now.0

now.o: \
compile now.c now.c datetime.h now.c now.h datetime.h datetime.h \
now.h now.c
	./compile now.c

open.a: \
makelib open_append.o open_read.o open_trunc.o
	./makelib open.a open_append.o open_read.o open_trunc.o

open_append.o: \
compile open_append.c open_append.c open_append.c open.h \
open_append.c
	./compile open_append.c

open_read.o: \
compile open_read.c open_read.c open_read.c open.h open_read.c
	./compile open_read.c

open_trunc.o: \
compile open_trunc.c open_trunc.c open_trunc.c open.h open_trunc.c
	./compile open_trunc.c

opensql.o: \
compile opensql.c error.h strerr.h errtxt.h \
	str.h case.h stralloc.h subscribe.h
	./compile opensql.c

putsubs.o: \
compile putsubs.c error.h substdio.h strerr.h readwrite.h getln.h \
str.h open.h case.h errtxt.h stralloc.h subscribe.h qmail.h fmt.h
	./compile putsubs.c

qmail.o: \
compile qmail.c substdio.h qmail.c readwrite.h qmail.c wait.h qmail.c \
exit.h qmail.c fork.h qmail.c fd.h qmail.c qmail.h substdio.h \
substdio.h qmail.h qmail.c auto_qmail.h qmail.c env.h qmail.c str.h
	./compile qmail.c

quote.o: \
compile quote.c stralloc.h gen_alloc.h stralloc.h quote.c str.h \
quote.c quote.h quote.c
	./compile quote.c

scan_8long.o: \
compile scan_8long.c scan.h scan_8long.c
	./compile scan_8long.c

scan_ulong.o: \
compile scan_ulong.c scan.h scan_ulong.c
	./compile scan_ulong.c

searchlog.o: \
compile searchlog.c case.h stralloc.h scan.h open.h datetime.h errtxt.h str.h \
datetime.h date822fmt.h substdio.h readwrite.h strerr.h error.h subscribe.h \
getln.h
	./compile searchlog.c

seek.a: \
makelib seek_set.o
	./makelib seek.a seek_set.o

seek_set.o: \
compile seek_set.c seek_set.c seek.h seek_set.c
	./compile seek_set.c

setup: \
it man install conf-bin conf-man
	./install "`head -n 1 conf-bin`" < BIN
	./install "`head -n 1 conf-man`" < MAN

sgetopt.0: \
sgetopt.3
	nroff -man sgetopt.3 > sgetopt.0

sgetopt.o: \
compile sgetopt.c substdio.h sgetopt.c subfd.h substdio.h substdio.h \
subfd.h sgetopt.c sgetopt.h sgetopt.h subgetopt.h sgetopt.h sgetopt.c \
subgetopt.h subgetopt.h sgetopt.c
	./compile sgetopt.c

sig.a: \
makelib sig_catch.o sig_pipe.o
	./makelib sig.a sig_catch.o sig_pipe.o

sig_catch.o: \
compile sig_catch.c sig_catch.c sig.h sig_catch.c hassgact.h \
sig_catch.c
	./compile sig_catch.c

sig_pipe.o: \
compile sig_pipe.c sig_pipe.c sig.h sig_pipe.c
	./compile sig_pipe.c

slurp.o: \
compile slurp.c stralloc.h gen_alloc.h stralloc.h slurp.c slurp.h \
slurp.c error.h slurp.c open.h slurp.c slurpclose.h
	./compile slurp.c

slurpclose.o: \
compile slurpclose.c stralloc.h gen_alloc.h stralloc.h slurpclose.c \
readwrite.h slurpclose.c slurpclose.h slurpclose.c error.h \
slurpclose.c
	./compile slurpclose.c

str.a: \
makelib str_len.o str_diff.o str_diffn.o str_cpy.o str_chr.o \
str_rchr.o str_start.o byte_chr.o byte_rchr.o byte_diff.o byte_copy.o \
byte_cr.o byte_zero.o
	./makelib str.a str_len.o str_diff.o str_diffn.o str_cpy.o \
	str_chr.o str_rchr.o str_start.o byte_chr.o byte_rchr.o \
	byte_diff.o byte_copy.o byte_cr.o byte_zero.o

str_chr.o: \
compile str_chr.c str.h str_chr.c
	./compile str_chr.c

str_cpy.o: \
compile str_cpy.c str.h str_cpy.c
	./compile str_cpy.c

str_diff.o: \
compile str_diff.c str.h str_diff.c
	./compile str_diff.c

str_diffn.o: \
compile str_diffn.c str.h str_diffn.c
	./compile str_diffn.c

str_len.o: \
compile str_len.c str.h str_len.c
	./compile str_len.c

str_rchr.o: \
compile str_rchr.c str.h str_rchr.c
	./compile str_rchr.c

str_start.o: \
compile str_start.c str.h str_start.c
	./compile str_start.c

stralloc.0: \
stralloc.3
	nroff -man stralloc.3 > stralloc.0

stralloc.a: \
makelib stralloc_eady.o stralloc_pend.o stralloc_copy.o \
stralloc_opys.o stralloc_opyb.o stralloc_cat.o stralloc_cats.o \
stralloc_catb.o stralloc_arts.o stralloc_num.o
	./makelib stralloc.a stralloc_eady.o stralloc_pend.o \
	stralloc_copy.o stralloc_opys.o stralloc_opyb.o \
	stralloc_cat.o stralloc_cats.o stralloc_catb.o \
	stralloc_arts.o stralloc_num.o

stralloc_arts.o: \
compile stralloc_arts.c byte.h stralloc_arts.c str.h stralloc_arts.c \
stralloc.h gen_alloc.h stralloc.h stralloc_arts.c
	./compile stralloc_arts.c

stralloc_cat.o: \
compile stralloc_cat.c byte.h stralloc_cat.c stralloc.h gen_alloc.h \
stralloc.h stralloc_cat.c
	./compile stralloc_cat.c

stralloc_catb.o: \
compile stralloc_catb.c stralloc.h gen_alloc.h stralloc.h \
stralloc_catb.c byte.h stralloc_catb.c
	./compile stralloc_catb.c

stralloc_cats.o: \
compile stralloc_cats.c byte.h stralloc_cats.c str.h stralloc_cats.c \
stralloc.h gen_alloc.h stralloc.h stralloc_cats.c
	./compile stralloc_cats.c

stralloc_copy.o: \
compile stralloc_copy.c byte.h stralloc_copy.c stralloc.h gen_alloc.h \
stralloc.h stralloc_copy.c
	./compile stralloc_copy.c

stralloc_eady.o: \
compile stralloc_eady.c alloc.h stralloc_eady.c stralloc.h \
gen_alloc.h stralloc.h stralloc_eady.c gen_allocdefs.h \
gen_allocdefs.h gen_allocdefs.h stralloc_eady.c
	./compile stralloc_eady.c

stralloc_num.o: \
compile stralloc_num.c alloc.h stralloc.h
	./compile stralloc_num.c

stralloc_opyb.o: \
compile stralloc_opyb.c stralloc.h gen_alloc.h stralloc.h \
stralloc_opyb.c byte.h stralloc_opyb.c
	./compile stralloc_opyb.c

stralloc_opys.o: \
compile stralloc_opys.c byte.h stralloc_opys.c str.h stralloc_opys.c \
stralloc.h gen_alloc.h stralloc.h stralloc_opys.c
	./compile stralloc_opys.c

stralloc_pend.o: \
compile stralloc_pend.c alloc.h stralloc_pend.c stralloc.h \
gen_alloc.h stralloc.h stralloc_pend.c gen_allocdefs.h \
gen_allocdefs.h gen_allocdefs.h stralloc_pend.c
	./compile stralloc_pend.c

strerr.a: \
makelib strerr.o strerr_sys.o strerr_die.o
	./makelib strerr.a strerr.o strerr_sys.o strerr_die.o

strerr.o: \
compile strerr.c stralloc.h gen_alloc.h stralloc.h strerr.c strerr.h \
strerr.c
	./compile strerr.c

strerr_die.o: \
compile strerr_die.c substdio.h strerr_die.c subfd.h substdio.h \
substdio.h subfd.h strerr_die.c exit.h strerr_die.c strerr.h \
strerr_die.c
	./compile strerr_die.c

strerr_sys.o: \
compile strerr_sys.c error.h strerr_sys.c strerr.h strerr_sys.c
	./compile strerr_sys.c

subdb.a: \
makelib checktag.o issub.o logmsg.o subscribe.o opensql.o putsubs.o \
	tagmsg.o searchlog.o
	./makelib subdb.a checktag.o issub.o logmsg.o subscribe.o \
	opensql.o putsubs.o tagmsg.o searchlog.o

subfd.0: \
subfd.3
	nroff -man subfd.3 > subfd.0

subfderr.o: \
compile subfderr.c readwrite.h subfderr.c substdio.h subfderr.c \
subfd.h substdio.h substdio.h subfd.h subfderr.c
	./compile subfderr.c

subgetopt.0: \
subgetopt.3
	nroff -man subgetopt.3 > subgetopt.0

subgetopt.o: \
compile subgetopt.c subgetopt.h subgetopt.h subgetopt.c
	./compile subgetopt.c

subscribe.o: \
compile subscribe.c stralloc.h gen_alloc.h stralloc.h \
getln.h readwrite.h substdio.h strerr.h open.h byte.h case.h \
lock.h error.h uint32.h subscribe.h idx.h fmt.h str.h
	./compile subscribe.c

substdi.o: \
compile substdi.c substdio.h substdi.c byte.h substdi.c error.h \
substdi.c
	./compile substdi.c

substdio.0: \
substdio.3
	nroff -man substdio.3 > substdio.0

substdio.a: \
makelib substdio.o substdi.o substdo.o subfderr.o substdio_copy.o
	./makelib substdio.a substdio.o substdi.o substdo.o \
	subfderr.o substdio_copy.o

substdio.o: \
compile substdio.c substdio.h substdio.c
	./compile substdio.c

substdio_copy.0: \
substdio_copy.3
	nroff -man substdio_copy.3 > substdio_copy.0

substdio_copy.o: \
compile substdio_copy.c substdio.h substdio_copy.c
	./compile substdio_copy.c

substdio_in.0: \
substdio_in.3
	nroff -man substdio_in.3 > substdio_in.0

substdio_out.0: \
substdio_out.3
	nroff -man substdio_out.3 > substdio_out.0

substdo.o: \
compile substdo.c substdio.h substdo.c str.h substdo.c byte.h \
substdo.c error.h substdo.c
	./compile substdo.c

surf.0: \
surf.3
	nroff -man surf.3 > surf.0

surf.a: \
makelib surf.o surfpcs.o
	./makelib surf.a surf.o surfpcs.o

surf.o: \
compile surf.c surf.h surf.c uint32.h surf.c
	./compile surf.c

surfpcs.0: \
surfpcs.3
	nroff -man surfpcs.3 > surfpcs.0

surfpcs.o: \
compile surfpcs.c surf.h surfpcs.c surfpcs.h uint32.h surfpcs.h \
surfpcs.c
	./compile surfpcs.c

systype: \
find-systype trycpp.c
	./find-systype > systype

tagmsg.o: \
compile tagmsg.c stralloc.h slurp.h scan.h fmt.h strerr.h cookie.h
	./compile tagmsg.c

uint32.h: \
tryulong32.c compile load uint32.h1 uint32.h2
	( ( ./compile tryulong32.c && ./load tryulong32 && \
	./tryulong32 ) >/dev/null 2>&1 \
	&& cat uint32.h2 || cat uint32.h1 ) > uint32.h
	rm -f tryulong32.o tryulong32

wait.0: \
wait.3
	nroff -man wait.3 > wait.0

wait.a: \
makelib wait_pid.o
	./makelib wait.a wait_pid.o

wait_pid.o: \
compile wait_pid.c wait_pid.c wait_pid.c error.h haswaitp.h
	./compile wait_pid.c

wrap.a: \
makelib wrap_execbin.o wrap_execsh.o wrap_execv.o wrap_exitcode.o \
wrap_fork.o wrap_waitpid.o wrap_stat.o
	./makelib wrap.a wrap_execbin.o wrap_execsh.o wrap_execv.o \
	wrap_exitcode.o wrap_fork.o wrap_waitpid.o wrap_stat.o

wrap_execbin.o: \
compile wrap_execbin.c auto_bin.h stralloc.h wrap.h
	./compile wrap_execbin.c

wrap_execsh.o: \
compile wrap_execsh.c wrap.h
	./compile wrap_execsh.c

wrap_execv.o: \
compile wrap_execv.c error.h errtxt.h strerr.h wrap.h
	./compile wrap_execv.c

wrap_exitcode.o: \
compile wrap_exitcode.c errtxt.h strerr.h wrap.h
	./compile wrap_exitcode.c

wrap_fork.o: \
compile wrap_fork.c fork.h errtxt.h strerr.h wrap.h
	./compile wrap_fork.c

wrap_stat.o: \
compile wrap_stat.c error.h errtxt.h strerr.h wrap.h
	./compile wrap_stat.c

wrap_waitpid.o: \
compile wrap_waitpid.c wait.h errtxt.h strerr.h wrap.h
	./compile wrap_waitpid.c

yyyymm.a: \
makelib date2yyyymm.o dateline.o
	./makelib yyyymm.a date2yyyymm.o dateline.o

ch: \
ezmlmrc.ch
	cp -f ezmlmrc.ch_GB ezmlmrc

ch_GB: \
ezmlmrc.ch_GB
	cp -f ezmlmrc.ch_GB ezmlmrc

cs: \
ezmlmrc.cs
	cp -f ezmlmrc.cs ezmlmrc

da: \
ezmlmrc.da
	cp -f ezmlmrc.da ezmlmrc

de: \
ezmlmrc.de
	cp -f ezmlmrc.de ezmlmrc

en_US: \
ezmlmrc.en_US
	cp -f ezmlmrc.en_US ezmlmrc

en: \
ezmlmrc.en_US
	cp -f ezmlmrc.en_US ezmlmrc

es: \
ezmlmrc.es
	cp -f ezmlmrc.es ezmlmrc

us: \
ezmlmrc.en_US
	cp -f ezmlmrc.en_US ezmlmrc

ezmlmrc: \
ezmlmrc.en_US
	cp -f ezmlmrc.en_US ezmlmrc

fr: \
ezmlmrc.fr
	cp -f ezmlmrc.fr ezmlmrc

hu: \
ezmlmrc.hu
	cp -f ezmlmrc.hu ezmlmrc

id: \
ezmlmrc.id
	cp -f ezmlmrc.id ezmlmrc

ita: \
ezmlmrc.it
	cp -f ezmlmrc.it ezmlmrc

jp: \
ezmlmrc.jp
	cp -f ezmlmrc.jp ezmlmrc

nl: \
ezmlmrc.nl
	cp -f ezmlmrc.nl ezmlmrc

pl: \
ezmlmrc.pl
	cp -f ezmlmrc.pl ezmlmrc

pt: \
ezmlmrc.pt
	cp -f ezmlmrc.pt ezmlmrc

pt_BR: \
ezmlmrc.pt_BR
	cp -f ezmlmrc.pt_BR ezmlmrc

ru: \
ezmlmrc.ru
	cp -f ezmlmrc.ru ezmlmrc

sv: \
ezmlmrc.sv
	cp -f ezmlmrc.sv ezmlmrc

symlinks: \
ezmlm-mktab checktag.c issub.c logmsg.c subscribe.c opensql.c \
putsubs.c tagmsg.c searchlog.c

ezmlm-mktab: \
conf-sub
	rm -f ezmlm-mktab
	sub=`head -n 1 conf-sub` ; ln sub_$$sub/ezmlm-mktab ezmlm-mktab
	touch ezmlm-mktab

checktag.c: \
conf-sub
	rm -f checktag.c checktag.o
	sub=`head -n 1 conf-sub` ; ln sub_$$sub/checktag.c checktag.c
	touch checktag.c

issub.c: \
conf-sub
	rm -f issub.c issub.o
	sub=`head -n 1 conf-sub` ; ln sub_$$sub/issub.c issub.c
	touch issub.c

logmsg.c: \
conf-sub
	rm -f logmsg.c logmsg.o
	sub=`head -n 1 conf-sub` ; ln sub_$$sub/logmsg.c logmsg.c
	touch logmsg.c

subscribe.c: \
conf-sub
	rm -f subscribe.c subscribe.o
	sub=`head -n 1 conf-sub` ; ln sub_$$sub/subscribe.c subscribe.c
	touch subscribe.c

opensql.c: \
conf-sub
	rm -f opensql.c opensql.o
	sub=`head -n 1 conf-sub` ; ln sub_$$sub/opensql.c opensql.c
	touch opensql.c

putsubs.c: \
conf-sub
	rm -f putsubs.c putsub.o
	sub=`head -n 1 conf-sub` ; ln sub_$$sub/putsubs.c putsubs.c
	touch putsubs.c

tagmsg.c: \
conf-sub
	rm -f tagmsg.c tagmsg.o
	sub=`head -n 1 conf-sub` ; ln sub_$$sub/tagmsg.c tagmsg.c
	touch tagmsg.c

searchlog.c: \
conf-sub
	rm -f searchlog.c searchlog.o
	sub=`head -n 1 conf-sub` ; ln sub_$$sub/searchlog.c searchlog.c
	touch searchlog.c

sql.lib: \
conf-sub
	sub=`head -n 1 conf-sub` ; head -n 1 sub_$$sub/conf-sqlld > sql.tmp
	mv sql.tmp sql.lib
