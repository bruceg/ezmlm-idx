#$Id$
#
# ezmlmrc 
# #######
# Controls the actions of ezmlm-make as patched with ezmlm-idx-0.31 or later.
#
# The base directory 'DIR' is always created by ezmlm-make, as is DIR/key.
# Everything else is done from here.
#
# ezmlm-make looks for this file, first as .ezmlmrc in the directory that the
# lists .qmail files will be placed in (if you've used the -c command line
# switch), then /etc/ezmlmrc, then ezmlmrc in the ezmlm-make binary directory.
# Thus, you can customize ezmlm-make on a global level by placing a customized
# copy of ezmlmrc in /etc and on a user level by copying it to .ezmlmrc in
# the user's home directory AND use the ezmlm-make -c switch.
#
# Tags are:
#	</filename/>       : put succeeding text lines in DIR/filename
#	</-filename/>      : erase DIR/filename.
#	</+dirname/>       : create directory DIR/dirname
#	</:lname/dirname>  : symlink DIR/.qmail-list-lname -> DIR/dirname
#
# The name in the tag can be suffixed with '#' and any number of flags,
# corresponding to command line switches. The item will be created/extended
# only if all the flags listed are set. Files can be extended as long as they
# were the last one created, but not if another file has been started since
# then. Flags that are not recognized are silently ignored.
# 
# Thus, </filename#aP/> creates the file if and only if the list is archived
# (-a) and not public (-P). If the next tag is </filename#m/>, the file is
# extended with the lines up to the next tag if the list is message moderated
# (-m). If the next tag is </another/>, 'filename' is closed. Any further
# tags leading to the reopenining of 'filename' will overwrite the file, not
# extend it.
#
# A set of user-defined command line switches (xX, yY, zZ) are available for
# customization.
#
# Within the text, certain tags are substituted. Other tags are copied as
# is. <#A#> and <#R#> are substituted by ezmlm-manage and -store (see man pages)
# and <#l#> (lower case L) is replaced dynamically by the list name for
# programs handling both 'list' and 'list-digest'.
#
# Substitutions are:
# <#B#> ezmlm binaries path   <#C#> digest code         <#D#> dir
# <#H#> host                  <#L#> local               <#F#> flags
# <#T#> dot                   <#0#> arg for -0. <#3#>...<#9#> arg for -3..9
# <#1#> ext1                  <#2#> ext2 [if dot is /path/.qmail-ext1-ext2-name]
# The latter useful when a single user is controlling several virtual domains.
#
# -0 is used for the main list address when setting up sublists
# -4 for specifying the ezmlm-tstdig switches used in dir/editor. Default
#    -k64 -m30 -t24. Only used if -g is used.
# -5 for list-owner address. Mail to list-owner will be forwarded to this addr.
# -6 for sql connection info
# -7 for contents of DIR/modpost
# -8 for contents of DIR/modsub
# -9 for contents of DIR/remote
#
# For demonstration purposes, the '-x' switch results in the following
# non-standard actions:
# - Removal of many non-text MIME parts from messages.
# - Limit posts to 2 bytes <= msg body size <= 40000
#
# Attempts to create links or directories that already exist, will result
# in a FATAL error. Attempts to open files that have already been closed
# or already exits, will cause the old file to be overwritten.
#
# One of the major problems with ezmlm-lists is DIR/inlocal. For normal
# users, it is set up to the list name (user-list or so), which is correct.
# However, for user 'ezmlm' in control of virtual domain 'host.dom.com'
# the list name is 'list@host.dom.com', but inlocal should be 'ezmlm-list',
# not 'list'. Similarly, if ezmlm-domain1 is in control of 'host.dom.com,
# list@host.dom.com, should yield an inlocal of 'ezmlm-domain1-list'. To
# always get the lists correct, place this file as '.ezmlmrc' in the 
# users home directory (~ezmlm/.ezmlmrc) and change the inlocal text below
# to 'ezmlm-<#L#>' or 'ezmlm-<#1#>-<#L#>, respectively.
# config to support future editing without giving ezmlm-make command line
# arguments other than dir. Useful for GUI/WWW editing tools
</config/>
F:<#F#>
D:<#D#>
T:<#T#>
L:<#L#>
H:<#H#>
C:<#C#>
0:<#0#>
3:<#3#>
4:<#4#>
5:<#5#>
6:<#6#>
7:<#7#>
8:<#8#>
9:<#9#>
</charset/>
# Explicitly specify character-set, when this ezmlmrc was used.
# Use Quoted-Printable to make averyone happy.
iso-8859-2:Q
</inlocal/>
<#L#>
</sublist#0/>
<#0#>
</+archive/>
</+subscribers/>
</+bounce/>
</+text/>
# dirs for digests
</+digest#d/>
</+digest/subscribers#d/>
</+digest/bounce#d/>
# for extra address db
</+allow/>
</+allow/subscribers/>
# for blacklist
</+deny#k/>
</+deny/subscribers#k/>
# moderator db & mod queue dirs. Needed for -m, -r -s, so we just
# make them by default.
</+mod/>
</+mod/subscribers/>
</+mod/pending/>
</+mod/accepted/>
</+mod/rejected/>
# links: dot -> dir/editor
</:/editor/>
</:-owner/owner/>
</:-digest-owner/owner#d/>
</:-return-default/bouncer/>
</:-digest-return-default/digest/bouncer#d/>
</:-default/manager/>
# for message moderation only
</:-accept-default/moderator#m/>
</:-reject-default/moderator#m/>
# Get rid of configuration flags for editing mode so we can start with a
# clean slate.
</-modpost#eM/>
</-modsub#eS/>
</-remote#eR/>
</-public#eP/>
</-indexed#eI/>
</-archived#eA/>
</-prefix#eF/>
</-text/trailer#eT/>
</-sublist#e^0/>
</-mimeremove#eX/>
# Not needed, except for message moderation.
</-moderator#eM/>
# We don't clean out text files to make it easier for users
# doing manual config by e.g. touching dir/remote.
# subscription moderation
</modsub#s/>
<#8#>
# remote admin
</remote#r/>
<#9#>
# message moderation
</modpost#m/>
<#7#>
# List owner mail
</owner#5/>
<#5#>
</owner#^5/>
<#D#>/Mailbox
</#W/>
|<#B#>/ezmlm-warn '<#D#>' || exit 0
# Handles subscription. Add flags if you want a non-default digest format.
# Service subject commands to the # request address if the -q switch is given.
# Also -l and -d enable subscriber listing/text file editing, for remote adms.
# -u gives subscriber only archive access
</manager#iG/>
|<#B#>/ezmlm-get '<#D#>' <#C#>
</manager#ig/>
|<#B#>/ezmlm-get -s '<#D#>' <#C#>
</manager#q/>
|<#B#>/ezmlm-request '<#D#>'
# Ok to add -l/-d even for non-mod lists, since ezmlm-manage
# won't allow it unless there are remote admins.
</manager#LN/>
|<#B#>/ezmlm-manage '<#D#>'
</manager#lN/>
|<#B#>/ezmlm-manage -l '<#D#>'
</manager#Ln/>
|<#B#>/ezmlm-manage -e '<#D#>'
</manager#ln/>
|<#B#>/ezmlm-manage -le '<#D#>'
</manager#W/>
|<#B#>/ezmlm-warn '<#D#>' || exit 0
</#dW/>
|<#B#>/ezmlm-warn -d '<#D#>' || exit 0
</editor/>
# reject shouldn't be configured for sublist.
</#^0/>
# full reject is now default, to get To/Cc: listaddress requirement
|<#B#>/ezmlm-reject '<#D#>'
# -k => reject posts from blacklisted addresses. Done for moderated
# lists as well - allows removal of unwanted noise.
</#k^0/>
|<#B#>/ezmlm-issubn -n '<#D#>/deny' || { echo "Omlovám se, ale mám pøíkaz odmítnout pøíspìvky od vás. Kontaktujte <#L#>-owner@<#H#>, máte-li dotazy k tomuto faktu (#5.7.2)"; exit 100 ; }
# switch -u=> restrict to subs of list & digest. If not m
# do it with ezmlm-issubn, if 'm' do it with ezmlm-gate
</#uM/>
|<#B#>/ezmlm-issubn '<#D#>' '<#D#>/digest' '<#D#>/allow' '<#D#>/mod' || { echo "Omlouvám se, ale do tohoto listu smìjí pøispívat pouze pøihlá¹ení u¾ivatelé. Jste-li pøihlá¹eným u¾ivatelem, pøepo¹lete tuto zprávu na adresu <#L#>-owner@<#H#>, aby akceptoval va¹i novou adresu (#5.7.2)"; exit 100 ; }
</#um/>
|<#B#>/ezmlm-gate '<#D#>' '<#D#>' '<#D#>/digest' '<#D#>/allow' '<#D#>/mod'
# For message moderation, editor has store/clean
</#mU/>
|<#B#>/ezmlm-store '<#D#>'
|<#B#>/ezmlm-clean '<#D#>' || exit 0
</#mu/>
|<#B#>/ezmlm-clean -R '<#D#>' || exit 0
# for non-message moderated lists, it has send
</#M/>
|<#B#>/ezmlm-send '<#D#>'
# all lists have warn unless -w.
</#W/>
|<#B#>/ezmlm-warn '<#D#>' || exit 0
# for digest bounces
</#dW/>
|<#B#>/ezmlm-warn -d '<#D#>' || exit 0
</#d^4/>
|<#B#>/ezmlm-tstdig -m30 -k64 -t48 '<#D#>' || exit 99
</#d4/>
|<#B#>/ezmlm-tstdig <#4#> '<#D#>' || exit 99
</#d/>
|<#B#>/ezmlm-get '<#D#>' || exit 0
# bouncer is complicated. We use ezmlm-receipt if -6 AND -w, but ezmlm-return
# if (-6 and -W) OR (not -6 and -w). Since there is no or, we need 2 lines.
</bouncer/>
|<#B#>/ezmlm-weed
</#^6/>
|<#B#>/ezmlm-return -D '<#D#>'
</#6W/>
|<#B#>/ezmlm-return -D '<#D#>'
</#6w/>
|<#B#>/ezmlm-receipt -D '<#D#>'
</digest/bouncer#d/>
|<#B#>/ezmlm-weed
</#^6d/>
|<#B#>/ezmlm-return -d '<#D#>'
</#6Wd/>
|<#B#>/ezmlm-return -d '<#D#>'
</#6wd/>
|<#B#>/ezmlm-receipt -d '<#D#>'
# moderator is set up only for message moderated lists. However, '-e' does
# not remove it since we can't remove the symlinks to it (they're outside
# of the list dir.
</moderator#m/>
|<#B#>/ezmlm-moderate '<#D#>'
</#mU/>
|<#B#>/ezmlm-clean '<#D#>' || exit 0
</#mu/>
|<#B#>/ezmlm-clean -R '<#D#>' || exit 0
</headerremove#E/>
return-path
return-receipt-to
content-length
precedence
x-confirm-reading-to
x-pmrqc
# Only one allowed
list-help
list-unsubscribe
list-post
</lock/>
</lockbounce/>
</digest/lockbounce#d/>
</digest/lock#d/>
</public#p/>
</archived#a/>
</indexed#i/>
</inhost/>
<#H#>
</outhost/>
<#H#>
</outlocal/>
<#L#>
</mailinglist/>
contact <#L#>-help@<#H#>; run by ezmlm
# Headeradd needs to always exist
</headeradd#E/>
# Good for mailing list stuff (and vacation program)
Precedence: bulk
# To prevent indexing by findmail.com
X-No-Archive: yes
# rfc2369
List-Help: <mailto:<#l#>-help@<#h#>>
List-Unsubscribe: <mailto:<#l#>-unsubscribe@<#h#>>
List-Subscribe: <mailto:<#l#>-subscribe@<#h#>>
List-Post: <mailto:<#L#>@<#H#>>
# max & min message size
</msgsize#x/>
40000:2
# remove mime parts if -x
</mimeremove#x/>
application/excel
application/rtf
application/msword
application/ms-tnef
text/html
text/rtf
text/enriched
text/x-vcard
application/activemessage
application/andrew-inset
application/applefile
application/atomicmail
application/dca-rft
application/dec-dx
application/mac-binhex40
application/mac-compactpro
application/macwriteii
application/news-message-id
application/news-transmission
application/octet-stream
application/oda
application/pdf
application/postscript
application/powerpoint
application/remote-printing
application/slate
application/wita
application/wordperfect5.1
application/x-bcpio
application/x-cdlink
application/x-compress
application/x-cpio
application/x-csh
application/x-director
application/x-dvi
application/x-hdf
application/x-httpd-cgi
application/x-koan
application/x-latex
application/x-mif
application/x-netcdf
application/x-stuffit
application/x-sv4cpio
application/x-sv4crc
application/x-tar
application/x-tcl
application/x-tex
application/x-texinfo
application/x-troff
application/x-troff-man
application/x-troff-me
application/x-troff-ms
application/x-ustar
application/x-wais-source
audio/basic
audio/mpeg
audio/x-aiff
audio/x-pn-realaudio
audio/x-pn-realaudio
audio/x-pn-realaudio-plugin
audio/x-realaudio
audio/x-wav
image/gif
image/ief
image/jpeg
image/png
image/tiff
image/x-cmu-raster
image/x-portable-anymap
image/x-portable-bitmap
image/x-portable-graymap
image/x-portable-pixmap
image/x-rgb
image/x-xbitmap
image/x-xpixmap
image/x-xwindowdump
text/x-sgml
video/mpeg
video/quicktime
video/x-msvideo
video/x-sgi-movie
x-conference/x-cooltalk
x-world/x-vrml
# These can also be excluded, but for many lists it is desirable
# to allow them. Uncomment to add to mimeremove.
# application/zip
# application/x-gtar
# application/x-gzip
# application/x-sh
# application/x-shar
# chemical/x-pdb
# --------------------- Handle SQL connect info
</-sql#^6e/>
</-digest/sql#^6e/>
</-allow/sql#^6e/>
</sql#6W/>
<#6#>
</sql#6w/>
<#6#>:<#L#>@<#H#>
</digest/sql#6dW/>
<#6#>_digest
</digest/sql#6dw/>
<#6#>_digest:<#L#>_digest@<#H#>
</allow/sql#6/>
<#6#>_allow
# -------------------- End sql stuff
</prefix#f/>
[<#L#>]
</text/trailer#t/>
---------------------------------------------------------------------
Pro odhlá¹ení, po¹lete mail na <#L#>-unsubscribe@<#H#>
Dal¹í pøíkazy vypí¹e e-mail: <#L#>-help@<#H#>
</text/bottom/>

--- Administrativní pøíkazy pro list <#l#> ---

Umím automaticky obsluhovat administrativní pøíkazy. Neposílejte
je prosím do listu! Po¹lete zprávu na adresu pøíslu¹ného pøíkazu:

Pro pøihlá¹ení do listu, po¹lete e-mail na adresu:
   <<#L#>-subscribe@<#H#>>

Pro odhlá¹ení se z listu pou¾ijte adresu
   <<#L#>-unsubscribe@<#H#>>

Informace o listu a FAQ (èasto kladené dotazy) získáte zasláním dopisu
na následující adresy:
   <<#L#>-info@<#H#>>
   <<#L#>-faq@<#H#>>

</#d/>
Podobné adresy existují i pro list digestù:
   <<#L#>-digest-subscribe@<#H#>>
   <<#L#>-digest-unsubscribe@<#H#>>

# ezmlm-make -i needed to add ezmlm-get line. If not, we can't do
# multi-get!
</text/bottom#ai/>
Zprávy èíslo 123 a¾ 145 z archívu (maximálnì 100 zpráv na jeden mail)
získáte zasláním zprávy na následující adresu:
   <<#L#>-get.123_145@<#H#>>

</text/bottom#aI/>
Zprávu èíslo 12 získáte pomocí následující adresy:
   <<#L#>-get.12@<#H#>>

</text/bottom#i/>
Seznam zpráv se subjectem a autorem zpráv 123-456 - e-mail:
   <<#L#>-index.123_456@<#H#>>

Tyto adresy v¾dy vrací seznam 100 zpráv, maximálnì 2000 na jeden po¾adavek.
Tak¾e ve vý¹e uvedeném pøíkladì ve skuteènosti dostanete seznam zpráv 100-499.

# Lists need to be both archived and indexed for -thread to work
</text/bottom#ai/>
Pro získání zpráv se stejným subjectem jako zpráva 12345 po¹lete
prázdný mail na adresu:
   <<#L#>-thread.12345@<#H#>>

# The '#' in the tag below is optional, since no flags follow.
# The name is optional as well, since the file will always be open
# at this point.
</text/bottom#/>
Zprávy ve skuteènosti nemusí být prázdné - budu jejich obsah ignorovat.
Jediná dùle¾itá vìc je ADRESA na kterou mail posíláte.

Mù¾ete se také do listu pøihlásit pod jinou adresou - napøíklad
"pepa@domena.cz". Staèí pøidat pomlèku a novou adresu s rovnítkem (=)
namísto zavináèe:
<<#L#>-subscribe-pepa=domena.cz@<#H#>>

Odhlásit tuto adresu lze pomocí mailu na adresu
<<#L#>-unsubscribe-pepa=domena.cz@<#H#>>

V obou pøípadech po¹lu ¾ádost o souhlas na tuto adresu. Kdy¾ ji dostanete,
jednodu¹e na ni odpovìzte a va¹e pøihlá¹ení/odhlá¹ení se dokonèí.

</text/bottom/>
Pokud nedosáhnete po¾adovaných výsledkù, kontaktujte mého správce
na adrese <#L#>-owner@<#H#>. Prosím o trpìlivost, mùj správce je
podstatnì pomalej¹í ne¾ já ;-)
</text/bottom/>

--- Pøipojuji kopii po¾adavku, který jsem dostal.

</text/bounce-bottom/>

--- Pøipojuji kopii zprávy, která se mi vrátila.

</text/bounce-num/>

Uschoval jsem si seznam zpráv z listu <#L#>, které se z va¹í adresy
vrátily.

</#a/>
Kopie tìchto zpráv mù¾ete získat v archívu.
</#aI/>
Pro získání zprávy 12345 z archívu, po¹lete prázdný mail na adresu
   <<#L#>-get.12345@<#H#>>

</#ia/>
Zprávy èíslo 123 a¾ 145 získáte z archívu (maximálnì 100 zpráv na jeden mail)
získáte zasláním zprávy na následující adresu:
   <<#L#>-get.123_145@<#H#>>

Seznam zpráv se subjectem a autorem zpráv 123-456 - e-mail:
   <<#L#>-index@<#H#>>

<//>
Následují èísla zpráv:

</text/dig-bounce-num/>

Uschoval jsem èísla digestù z listu <#L#>-digest, které se vrátily
z va¹í adresy. Pro ka¾dý takový digest jsem si zapamatoval èíslo
první zprávy v digestu. Nearchivuji si digesty samotné, ale
mù¾ete si vy¾ádat jednotlivé zprávy z archívu hlavního listu.

</#aI/>

Pro získání zprávy 12345 z archívu, po¹lete prázdný mail na adresu
   <<#L#>-get.12345@<#H#>>

</#ia/>
Zprávy èíslo 123 a¾ 145 získáte z archívu (maximálnì 100 zpráv na jeden mail)
získáte zasláním zprávy na následující adresu:
   <<#L#>-get.123_145@<#H#>>

Seznam zpráv se subjectem a autorem zpráv 123-456 - e-mail:
   <<#L#>-index@<#H#>>

<//>
Následují èísla prvních zpráv v digestech:

</text/bounce-probe/>

Vracejí se mi zprávy pro vás z listu <#l#>.
Poslal jsem vám varovnou zprávu, ale ta se také vrátila.
Pøipojuji kopii varovné zprávy.

Toto je testovací zpráva pro ovìøení, jestli je va¹e adresa dosa¾itelná.
Pokud se tato zpráva vrátí, zru¹ím va¹i adresu z listu <#l#>@<#H#>
bez dal¹ího upozornìní. Mù¾ete se pøihlásit znovu posláním prázdné zprávy
na adresu
   <<#l#>-subscribe@<#H#>>

</text/bounce-warn/>

Vracejí se mi zprávy pro vás z listu <#l#>.
Pøipojuji kopii první vrácené zprávy, kterou jsem dostal.

Pokud se tato zpráva také vrátí, po¹lu testovací zprávu.
Pokud se vrátí i testovací zpráva, zru¹ím va¹i adresu z listu <#l#>
bez dal¹ího upozornìní.

</text/digest#d/>
Pro pøihlá¹ení do digestu po¹lete mail na adresu
	<#L#>-digest-subscribe@<#H#>

Pro odhlá¹ení z digestu pou¾ijte adresu
	<#L#>-digest-unsubscribe@<#H#>

Chcete-li poslat zprávu do listu, pi¹te na adresu
	<#L#>@<#H#>

</text/get-bad/>
Promiòte, ale tato zpráva v archívu není.

</text/help/>
Toto je zpráva se v¹eobecnou nápovìdou. Zpráva, kterou jsem dostal,
nebyla poslána na ¾ádnou z adres platných pro zasílání pøíkazù.

</text/mod-help/>
Dìkuji, ¾e jste se uvoli moderovat nebo spravovat list <#L#>@<#H#>.

Moje pøíkazy jsou ponìkud odli¹nìj¹í od jiných listù, ale myslím,
¾e je seznáte intuitivními a pøíjemnými k pou¾ití.

Tady jsou instrukce pro èinnosti, které pøípadnì mù¾ete vykonávat
jako správce listu nebo moderátor.

Vzdálené pøihlá¹ení
-------------------

Jako moderátor mù¾ete pøihla¹ovat a odhla¹ovat libovolnou adresu
do svého listu. Pro pøihlá¹ení u¾ivatele "pepa@domena.cz" jednodu¹e
doplòte pomlèku za název pøíkazu a pak tuto adres s rovnítkem
místo zavináèe. Napøíklad pro pøihlá¹ení vý¹e uvedené adresy
po¹lete mail na adresu
   <<#L#>-subscribe-pepa=domena.cz@<#H#>>

Podobnì mù¾ete odhla¹ovat u¾ivatele pomocí mailu na adresu
   <<#L#>-unsubscribe-pepa=domena.cz@<#H#>>

</#d/>
Pro digestový list:
   <<#L#>-digest-subscribe-pepa=domena.cz@<#H#>>
   <<#L#>-digest-unsubscribe-pepa=domena.cz@<#H#>>

<//>
To je v¹echno. ®ádný speciální subject ani obsah zprávy není potøeba!

</#r/>
Po¹lu vám ¾ádost o potvrzení, abych se ujistil, ¾e po¾adavek
opravdu pochází od vás. Jednodu¹e odpovìzte na mail,
který obdr¾íte a vá¹ pøíkaz se vykoná.
</#R/>
Po¹lu ¾ádost o potvrzení, v tomto pøípadì na adresu <pepa@domena.cz>.
V¹echno co bude muset udìlat u¾ivatel je odpovìdìt na tuto
¾ádost.
<//>

Potvrzení jsou nezbytná, aby se tøetím stranám co nejvíce
ztí¾ila mo¾nost pøidávat a ru¹it cizí adresu do/z listu.

Uvìdomuji u¾ivatele, kdy¾ se stav jeho pøihlá¹ení zmìní.

Pøihlá¹ení
----------

Libovolný u¾ivatel se smí pøihla¹ovat a odhla¹ovat do/z listu
zasláním zprávy na adresu

<#L#>-subscribe@<#H#>
<#L#>-unsubscribe@<#H#>

</#d/>
Pro digestový list:

<#L#>-digest-subscribe@<#H#>
<#L#>-digest-unsubscribe@<#H#>

U¾ivatel obdr¾í ¾ádost o potvrzení, abych se ujistil,
¾e mu skuteènì daná adresa patøí. Jakmile se toto ovìøí,
u¾ivatel je odhlá¹en.

</#s/>
Proto¾e toto je list s moderovaným pøihlá¹ením, po¹lu dal¹í
¾ádost o potvrzení moderátorovi. Proto¾e u¾ivatel ji¾ potvrdil
pøání být na listu, mù¾ete si jako moderátor být dostateènì jist,
¾e adresa pøihla¹ovaného je skuteèná. Pokud chcete pøijmout
u¾ivatelovu ¾ádost, jednodu¹e po¹lete odpovìï na tuto zprávu.
Pokud ne, sma¾te tuto zprávu a pøípadnì kontaktujte u¾ivatele
pro dal¹í informace.
</#S/>
Pøihlíá¹ení funguje stejnì.
<//>

U¾ivatel také mù¾e pou¾ít adresu

   <<#L#>-subscribe-jana=domena.cz@<#H#>>
   <<#L#>-unsubscribe-jana=domena.cz@<#H#>>

pro zaslání mailu pro "jana@domena.cz". Pokud tato skuteènì má
vý¹e uvedenou adresu, obdr¾í ¾ádost o potvrzení a mù¾e ji
potvrdit.

Va¹e adresa a identita není otevøena pøihlá¹enému, pokud mu sám
nepo¹lete mail.

</#rl/>
Pro získání seznamu pøihlá¹ených pro list <#L#>@<#H#>, po¹lete
zprávu na adresu
   <<#L#>-list@<#H#>>

Seznam provedených transakcí listu <#L#>@<#H#> získáte z adresy
   <<#L#>-list@<#H#>>

</#rld/>
Pro pøihlá¹ené do digestu:
   <<#L#>-digest-list@<#H#>>
a
   <<#L#>-digest-log@<#H#>>

</#rn/>
Mù¾ete vzdálenì editovat textové soubory, ze kterých se sestavují
odpovìdi, které posílám. Chcete-li získat seznam editovatelných souborù
a pokyny pro editaci, napi¹te na adresu
   <<#L#>-edit@<#H#>>

</#m/>
Moderované pøíspìvky
--------------------

Pokud je list moderovaný, ulo¾ím si zprávy, které obdr¾ím a po¹lu vám
kopii a instrukce. Zpráva pro vás bude mít subject "MODERATE for ...".

Chcete-li zprávu pøijmout, staèí poslat odpovìï (na adresu v "Reply-To:"),
kterou nastavím na pøíkaz pro pøijetí zprávy do listu. Nemusíte 
posílat obsah pùvodní zprávy. Ve skuteènosti ignoruji cokoli, co mi
po¹lete, pokud bude adresa, na kterou to po¹lete, korektní.

Pokud chcete zprávu odmítnout, po¹lete odpovìï na adresu ve "From:",
kterou nastavím na pøíkaz pro odmítnutí zprávy. Toto se obvykle udìlá
pøíkazem "Odpovìz v¹em" ve va¹em po¹tovním klientovi, pøièem¾
sma¾ete v¹echny ostatní adresy kromì adresy pro odmítnutí (reject).
Mù¾ete pøidat komentáø odesílateli - napi¹te jej mezi dva øádky,
zaèínající tøema znaky "%". Po¹lu autorovi pouze tento komentáø,
co¾ neprozradí va¹i identitu.

Se zprávou nalo¾ím podle první odpovìdi, kterou dostanu.
Uvìdomím vás, pokud mi po¹lete po¾adavek na potvrzení ji¾
odmítnuté zprávy a naopak,

Pokud nedostanu odpovìï od moderátora do urèité doby (implicitnì 5 dní),
vrátím zprávu odesílateli s vysvìtlením, ¾e byla odmítnuta.
Jako administrátor mù¾ete také list nastavit tak, ¾e ignorované
zprávy jsou jednodu¹e smazány bez upozornìní odesílateli.
<//>

Dovolená
--------
Pokud jste doèasnì na jiné adrese, staèí si pøeposlat v¹echny zprávy,
které mají správnou hlavièku "Mailing-List:" (nebo v¹echny se subjectem
"MODERATE for <#L#>@<#H#>" (nebo "CONFIRM subscribe to <#L#>@<#H#>")
na novou adresu. Mù¾ete také pøeposílat tyto zprávy pøíteli, který
bude moderovat za vás. Prosím uvìdomte o tomto také správce listserveru.

Pokud chcete automaticky potvrdit v¹echny po¾adavky bìhem své nepøítomnosti,
nastavte si po¹tovního klienta na posílání automatických odpovìdí na zprávy,
které splòují vý¹e uvedená kritéria.

</#r/>
Pokud zkusíte dìlat vzdálenou administraci z adresy, která není
va¹e vlastní, bude o potvrzení po¾ádán u¾ivatel, nikoli vy.
Po schválení u¾ivatelem po¹lu ¾ádost o potvrzení v¹em moderátorùm.
Toto dìlám proto, ¾e nemám zpùsob, jak zjistit, ¾e jste to skuteènì vy,
kdo poslal pùvodní po¾adavek.

Berte také na vìdomí, ¾e v tomto pøípadì je vá¹ pùvodní po¾adavek
(vèetnì va¹í adresy!) zaslán u¾ivateli s ¾ádostí o potvrzení.
<//>

Mnoho ¹tìstí!

PS: Prosím kontaktujte správce listu (<#L#>-owner@<#H#>),
budete-li mít dotazy nebo problémy.

</text/mod-reject/>
Je mi líto, ale va¹e ní¾e citované zpráva nebyla potvrzena moderátorem.
Pokud moderátor pøipojil nìjaký komentáø, uvádím jej ní¾e.
</text/mod-request/>
Ní¾e citovaná zpráva byla zaslána na adresu listu <#L#>@<#H#>.
Pokud souhlasíte s distribucí této zprávy v¹em pøihlá¹eným, po¹lete
mail na adresu

!A

Toho obvykle dosáhnete pomocí tlaèítka "Odpovìï/Reply". Mù¾ete zkontrolovat
adresu, zaèíná-li øetìzcem "<#L#>-accept". Pokud toto nefunguje, staèí
zkopírovat adresu a ulo¾it ji do políèka "To:" nové zprávy.
</#x/>

Mù¾ete také zkusit kliknout zde:
	mailto:<#A#>
<//>

Chcete-li zprávu odmítnout a zpùsobit její vrácení odesílateli,
po¹lete zprávu na adresu

!R

Toto se bvykle nejsnáze udìlá pomocí tlaèítka "Odpovìz v¹em/Reply to all"
a následného vymazání v¹ech adres kromì té, která zaèíná "<#L#>-reject".
</#x/>

Mù¾ete také zkusit kliknout zde:
	mailto:<#R#>
<//>

Není tøeba kopírovat zprávu ve svém potvrzení nebo odmítnutí této zprávy.
Chcete-li poslat komentáø odesílateli odmítnuté zprávy, doplòte jej
mezi následující øádky zaèínající tøemi znaky "%":

%%% Zaèátek komentáøe
%%% Konec komentáøe

Dìkuji za spolupráci.

--- Dále uvádím zaslanou zprávu.

</text/mod-sub#E/>
--- Pøihlásil nebo odhlásil jsem vás na ¾ádost moderátora
listu <#l#>@<#H#>.

Pokud to není akce, se kterou souhlasíte, po¹lete co nejdøíve
stí¾nost nebo dal¹í komentáøe správci listu (<#l#>-owner@<#H#>).

Chcete-li získat podrobnìj¹í návod pro práci s listem <#L#>, po¹lete
prázdnou zprávu na adresu
<#L#>-help@<#H#>.

</text/mod-timeout/>
Je mi líto, ale moderátor listu <#L#> nezareagoval na vá¹ pøíspìvek.
Tento tedy pova¾uji za odmítnutý a vracím vám jej. Pokud máte pocit,
¾e do¹lo k chybì, obra»te se pøímo na moderátora listu.

--- Dále uvádím vámi zaslanou zprávu.

</text/mod-sub-confirm/>
®ádám zdvoøile oprávnìní pøidat adresu

!A

do seznamu ètenáøù listu <#l#>. Po¾adavek vze¹el buïto od vás,
nebo ji¾ byl ovìøen u potenciálního ètenáøe.

Souhlasíte-li, po¹lete prázdný mail na adresu

!R

Toho obvykle dosáhnete pomocí tlaèítka "Odpovìï/Reply". Mù¾ete zkontrolovat
adresu, zaèíná-li øetìzcem "<#L#>-sc". Pokud toto nefunguje, staèí
zkopírovat adresu a ulo¾it ji do políèka "To:" nové zprávy.
</#x/>

Mù¾ete také zkusit kliknout zde:
	mailto:<#R#>
<//>

Nesouhlasíte-li, ignorujte tuto zprávu.

Díky za spolupráci!

</text/mod-unsub-confirm/>
Obdr¾el jsem po¾adavek na zru¹ení adresy

!A

z listu <#l#>. Souhlasíte-li, po¹lete prázdnou odpovìï
na tuto adresu:

!R

Toho obvykle dosáhnete pomocí tlaèítka "Odpovìï/Reply". Mù¾ete zkontrolovat
adresu, zaèíná-li øetìzcem "<#L#>-sc". Pokud toto nefunguje, staèí
zkopírovat adresu a ulo¾it ji do políèka "To:" nové zprávy.
</#x/>

Mù¾ete také zkusit kliknout zde:
	mailto:<#R#>
<//>

Nesouhlasíte-li, ignorujte tuto zprávu.

Dìkuji za spolupráci!

</text/sub-bad/>
Hmmm, tohle èíslo potvrzení nevypadá platnì.

Nejèastìj¹ím dùvodem výskytu neplatných èísel je vypr¹ení èasu.
Musím dostat odpovìï na ka¾dý po¾adavek nejpozdìji do deseti dnù.
Ujistìte se také, ¾e v odpovìdi, kterou jsem obdr¾el, bylo _celé_
èíslo potvrzení. Nìkteré e-mailové programy mohou oøíznout èást
adresy pro odpovìï, která mù¾e být i dosti dlouhá.

Posílám novou ¾ádost o potvrzení. Pro potvrzení, ¾e chcete pøidat adresu

!A

do listu <#l#>, po¹lete prázdnou odpovìï na adresu

!R
</#x/>

Mù¾ete také zkusit kliknout zde:
	mailto:<#R#>
<//>

Je¹tì jednou: ujistìte se, ¾e adresa pro odpovìï je skuteènì v poøádku
pøedtím, ne¾ potvrdíte tuto ¾ádost.

Omlouvám se za potí¾e.

	<#L#>-Owner <<#l#>-owner@<#H#>>

</text/sub-confirm/>
Chcete-li opravdu pøidat adresu

!A

do listu <#l#>, po¹lete prázdnou zprávu na adresu

!R

Toho obvykle dosáhnete pomocí tlaèítka "Odpovìï/Reply". Mù¾ete zkontrolovat
adresu, zaèíná-li øetìzcem "<#L#>-sc". Pokud toto nefunguje, staèí
zkopírovat adresu a ulo¾it ji do políèka "To:" nové zprávy.

</#x/>

Mù¾ete také zkusit kliknout zde:
	mailto:<#R#>
<//>

Vy¾adování tohoto potvrzení má dva dùvody. Za prvé ovìuje, ¾e jsem
schopen zasílat mail na va¹i adresu. A za druhé, chrání vás v pøípadì,
¾e nìkdo zfal¹uje ¾ádost o pøihlá¹ení s va¹ím jménem.

</#q/>
Nìkteré po¹tovní programy jsou chybné a nemohou zpracovávat dlouhé
adresy. Pokud nemù¾ete odpovìdìt na tuto adresu, po¹lete
zprávu na adresu <<#L#>-request@<#H#>>
a vlo¾te celou vý¹e uvedenou adresu do subjectu.

</text/sub-confirm#s/>
Tento list je moderovaný. Jakmile po¹lete potvrzení, po¾adavek
bude poslán moderátorovi tohoto listu. Uvìdomím vás, jakmile
bude pøihlá¹ení hotovo.

</text/sub-nop/>
Potvrzení: Adresa

!A

byla v ji¾ v listu <#l#>, kdy¾ jsem obdr¾el va¹i ¾ádost,
a zùstává pøihlá¹ena.

</text/sub-ok#E/>
Potvrzení: Pøidal jsem adresu

!A

do listu <#l#>.

Vítejte v listu <#l#>@<#H#>!

Prosím uschovejte si tuto zprávu pro informaci, z jaké adresy
bylo pøihlá¹ení do listu provedeno. Budete ji potøebovat v pøípadì,
¾e se budete chtít odhlásit nebo zmìnit svoji adresu.

</text/top/>
Zdravím, tady je program ezmlm. Spravuji diskusní list
<#l#>@<#H#>.

</#x/>
Pracuji pro svého správce, který mù¾e být zasti¾en na adrese
at <#l#>-owner@<#H#>.

</text/unsub-bad/>
Hmmm, tohle èíslo potvrzení nevypadá platnì.

Nejèastìj¹ím dùvodem výskytu neplatných èísel je vypr¹ení èasu.
Musím dostat odpovìï na ka¾dý po¾adavek nejpozdìji do deseti dnù.
Ujistìte se také, ¾e v odpovìdi, kterou jsem obdr¾el, bylo _celé_
èíslo potvrzení. Nìkteré e-mailové programy mohou oøíznout èást
adresy pro odpovìï, která mù¾e být i dosti dlouhá.

Posílám novou ¾ádost o potvrzení. Pro potvrzení, ¾e chcete zru¹it adresu

!A

do listu <#l#>, po¹lete prázdnou odpovìï na adresu

!R
</#x/>

Mù¾ete také zkusit kliknout zde:
        mailto:<#R#>
<//>

Je¹tì jednou: ujistìte se, ¾e adresa pro odpovìï je skuteènì v poøádku
pøedtím, ne¾ potvrdíte tuto ¾ádost.

Omlouvám se za potí¾e.

        <#L#>-Owner <<#l#>-owner@<#H#>>

</text/unsub-confirm/>
Potvrzujete-li, ¾e chcete adresu

!A

zru¹it z listu <#l#>, po¹lete prázdnou odpovìï na adresu

!R

Toho obvykle dosáhnete pomocí tlaèítka "Odpovìï/Reply". Mù¾ete zkontrolovat
adresu, zaèíná-li øetìzcem "<#L#>-sc". Pokud toto nefunguje, staèí
zkopírovat adresu a ulo¾it ji do políèka "To:" nové zprávy.
</#x/>

Mù¾ete také zkusit kliknout zde:
	mailto:<#R#>
<//>

Nekontroloval jsem, je-li va¹e adresa v souèasné dobì na listu.
Chcete-li zjistit, ze které adresy bylo provedeno pøihlá¹ení,
podívejte se do zpráv, které dostáváte z listu. Ka¾dá zpráva
má adresu v návratové cestì:
<<#l#>-return-<number>-jana=domena.cz@<#H#>.

</#q/>
Nìkteré po¹tovní programy jsou chybné a nemohou zpracovávat dlouhé
adresy. Pokud nemù¾ete odpovìdìt na tuto adresu, po¹lete
zprávu na adresu <<#L#>-request@<#H#>>
a vlo¾te celou vý¹e uvedenou adresu do subjectu.

</text/unsub-nop/>
Potvrzení: Adresa

!A

nebyla na listu <#l#> v dobì, kdy jsem obdr¾el
vá¹ po¾adavek a není na nìm ani teï.

Pokud se odhlásíte, ale stále vám budou chodit dopisy, je pøihlá¹ení
provedeno pod jinou adresou, ne¾ kterou v souèasné dobì pou¾íváte.
Podívejte se prosím do hlavièek zpráv na text

"Return-Path: <<#l#>-return-1234-pepa=domena.cz@<#H#>>'

Odhla¹ovací adresa pro tohoto u¾ivatele pak bude
"<#l#>-unsubscribe-pepa=domena.cz@<#H#>".
Staèí poslat mail na tuto adresu s tím, ¾e pepa=domena.cz nahradíte
skuteènými hodnotami. Pak odpovíte na ¾ádost o potvrzení a mìla
by vám dojít zpráva o odhlá¹ení z listu.

V nìkterých po¹tovních programech si musíte zprávu zobrazit vèetnì
hlavièek, jinak není hlavièka "Return-Path" viditelná.

Pokud toto stále nefunguje, pak je mi líto, ale nemohu vám pomoci.
Prosím PREPO©LETE (forward) zprávu z listu spolu s poznámkou o tom,
èeho se sna¾íte dosáhnout a seznamem adres, ze kterých potenciálnì
mù¾ete být pøihlá¹en[a] mému správci:

    <#l#>-owner@<#H#>

Tento se bude sna¾it vá¹ problém øe¹it. Mùj správce je tro¹ku pomalej¹í
ne¾ já, tak¾e prosím o trochu trpìlivosti.

</text/unsub-ok/>
Potvrzení: Zru¹il jsem adresu

!A

z listu <#l#>. Tato adresa ji¾ dále není v seznamu
pøihlá¹ených.

</text/edit-do#n/>
Prosím editujte následující soubor a po¹lete jej na adresu

!R

Vá¹ po¹tovní program by mìl mít tlaèítko "Odpovìï/Reply",
který tuto adresu pou¾ije automaticky.

Umím dokonce sám smazat citovací znaèky, které vá¹ po¹tovní program
pøidá pøed text, pokud ov¹em ponecháte znaèkovací øádky samotné.

Tyto øádky zaèínají tøemi znaky procento. Nesmí být modifikovány
(s výjimkou pøípadných znakù pøed nimi, pøidaných pøipadnì va¹ím
po¹tovním klientem).

</text/edit-list#n/>
Pøíkaz <#L#>-edit.soubor mù¾e být pou¾it vzdáleným správcem k editaci
textových souborù, ze kterých se skládají odpovìdi pro list <#L#>@<#H#>.

Následuje seznam pøíslu¹ných souborù a krátký popis toho, kdy je
jejich obsah vyu¾íván. Chcete-li editovat soubor, staèí poslat
mail na adresu <#L#>-edit.soubor, pøièem¾ je nutno "soubor"
nahradit skuteèným jménem souboru. Obdr¾íte soubor spolu s instrukcemi,
jak tento soubor editovat.

Soubor              Pou¾ití

bottom              pøidává se za ka¾dou odpovìï. V¹eobecné informace.
digest              "administrativní" èást digestù.
faq                 èasto kladené dotazy, specifické pro tento list.
get_bad             neni-li zpráva nalezena v archívu.
help                v¹eobecná nápovìda (mezi "top" a "bottom").
info                informace o listu. První øádek by mìl dávat smysl sám o sobì.
mod_help            nápovìda pro moderátory.
mod_reject          odesílateli odmítnuté zprávy.
mod_request         moderátorovi spolu s pøíspìvkem.
mod_sub             pøihla¹ovanému, jakmile jeho pøihlá¹ení potvrdí moderátor.
mod_sub_confirm     moderátorovi s ¾ádostí o potvrzení pøihlá¹ení.
mod_timeout         odesílateli, nestihne-li moderátor potvrdit zprávu.
mod_unsub_confirm   administrátorovi s ¾ádostí o potvrzení odhlá¹ení.
sub_bad             odesílateli, bylo-li pøihlá¹ení neplatné.
sub_confirm         odesílateli - ¾ádost potvrzení pøihlá¹ení.
sub_nop             odesílateli - pøi pokusu o opìtovné pøihlá¹ení
sub_ok              odesílateli - oznamuje pøihlá¹ení.
top                 zaèátek v¹ech odpovìdí.
</#tn/>
trailer             pøidá se za ka¾dý pøíspìvek do listu.
</#n/>
unsub_bad           odesílateli, byla li ¾ádost o odhlá¹ení neplatná.
unsub_confirm       odesílateli - ¾ádost o potvrzení odhlá¹ení.
unsub_nop           odesílateli - nebyl-li pøihlá¹en a sna¾il-li se odhlásit.
unsub_ok            odesílateli po úspì¹ném odhlá¹ení.

</text/edit-done#n/>
Textový soubor byl úspì¹nì upraven.
</text/info#E/>
®ádná informace nebyla k tomuto listu poskytnuta.
</text/faq#E/>
FAQ - Èasto kladené dotazy v listu <#l#>@<#H#>.

[ ®ádné zatím nejsou dostupné ]


