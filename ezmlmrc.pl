#$Id$
#
# English ---> polish translation by Sergiusz Paw³owicz, 1998.
# Translation (C) to Free Software Foundation, 1998.
# Wszelkie uwagi dotycz±ce t³umaczenia proszê przesy³aæ na adres
#                                        cheeze@hyperreal.art.pl
# Troche opisow po polsku znajdziecie pod adresem
#                        http://www.arch.pwr.wroc.pl/~ser/lists/
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
ISO-8859-2
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
|<#B#>/ezmlm-issubn -n '<#D#>/deny' || { echo "Przykro mi, ale nakazano mi odrzucaæ Twoje listy... Skontaktuj siê, proszê, z opiekunem listy, osi±galnym pod adresem: <#L#>-owner@<#H#>, je¿eli masz jakiekolwiek w±tpliwo¶ci co do tego stanu rzeczy (#5.7.2)"; exit 100 ; }
# switch -u=> restrict to subs of list & digest. If not m
# do it with ezmlm-issubn, if 'm' do it with ezmlm-gate
</#uM/>
|<#B#>/ezmlm-issubn '<#D#>' '<#D#>/digest' '<#D#>/allow' '<#D#>/mod' || { echo "Przykro mi, ale tylko prenumeratorzy mog± wysy³aæ pocztê na listê... Jesli jeste¶ pewna/pewien prenumerowania tej w³a¶nie listy, prze¶lij, proszê, kopiê tego listu opiekunowi: <#L#>-owner@<#H#>, aby dopisa³ on Twój nowy adres pocztowy (#5.7.2)"; exit 100 ; }
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
Wypisanie?     wy¶lij list: <#L#>-unsubscribe@<#H#>
Wiêcej pomocy? wy¶lij list: <#L#>-help@<#H#>
</text/bottom/>

--- Poni¿ej znajduj± siê komendy menad¿era list dyskusyjnych "ezmlm".

Spe³niam komendy administracyjne automatycznie.
Wy¶lij jedynie pusty list pod którykolwiek z tych adresów:

   <<#L#>-subscribe@<#H#>>:
   Zapisanie siê na listê o nazwie <#L#>.

   <<#L#>-unsubscribe@<#H#>>:
   Wypisanie siê z listy o nazwie <#L#>.

Wy¶lij list pod nastêpuj±ce adresy, je¶li chcesz uzykaæ krótk± notkê
informacyjn± o li¶cie dyskusyjnej lub jej FAQ (czyli 'najczê¶ciej
zadawana pytania na temat listy wraz z odpowiedziami'):
   <<#L#>-info@<#H#>>
   <<#L#>-faq@<#H#>>

</#d/>
I odpowiadaj±ce powy¿szemu adresy, je¶li chodzi ci o przegl±d listy:
   <<#L#>-digest-subscribe@<#H#>>
   <<#L#>-digest-unsubscribe@<#H#>>

# ezmlm-make -i needed to add ezmlm-get line. If not, we can't do
# multi-get!
</text/bottom#ai/>
   <<#L#>-get.12_45@<#H#>>:
   Chêæ otrzymania kopii listów od nr 12 do 45 z archiwum.
   Jednym listem mo¿esz otrzymaæ maksymalnie 100 listów.

</text/bottom#aI/>
   <<#L#>-get.12@<#H#>>:
   Chêæ otrzymania kopii listu nr 12 z archiwum.
</text/bottom#i/>
   <<#L#>-index.123_456@<#H#>>:
   Chêæ otrzymania tytu³ów listów od nr 123 do 456 z archiwum.
   Tytu³y s± wysy³ane do Ciebie w paczkach po sto na jeden list.
   Mo¿esz maksymalnie zamówiæ 2000 tytu³ów na jeden raz.

# Lists need to be both archived and indexed for -thread to work
</text/bottom#ai/>
   <<#L#>-thread.12345@<#H#>>:
   Chêæ otrzymania wszystkich kopii listów z tym samym tytu³em
   jaki posiada list o numerze 12345.

# The '#' in the tag below is optional, since no flags follow.
# The name is optional as well, since the file will always be open
# at this point.
</text/bottom#/>

NIE WYSY£AJ POLECEÑ ADMINISTRACYJNYCH NA LISTÊ DYSKUSYJN¡!
Jesli to zrobisz, administrator listy ich nie zobaczy, natomiast
wszyscy prenumeratorzy nie¼le siê na Ciebie wkurz± i zostaniesz
uznany(a) za internetowego ¿ó³todzioba, z którym nie warto gadaæ.

Aby okresliæ adres Bóg@niebo.prezydent.pl jako swój adres subskrybcyjny,
wy¶lij list na adres:
<<#L#>-subscribe-Bóg=niebo.prezydent.pl@<#H#>>.
Wy¶lê Ci wtedy potwierdzenie na ten w³asnie adres; kiedy je otrzymasz,
po prostu odpowiedz na nie, aby staæ siê prenumeratorem.

</text/bottom#x/>
Je¿eli instrukcje, których Ci udzieli³em, s± zbyt trudne, albo
nie jeste¶ zadowolony z ich efektów, skontaktuj siê z opiekunem listy
pod adresem <#L#>-owner@<#H#>. 
Proszê byæ cierpliwym, opiekun listy jest o wiele wolniejszy
od komputera, którym ja jestem ;-)
</text/bottom/>

--- Za³±cznik jest kopi± polecenia, które otrzyma³em.

</text/bounce-bottom/>

--- Za³±cznik jest kopi± odrzuconego listu, który dosta³em.

</text/bounce-num/>

Na wszelki wypadek przechowujê odrzucone listy z twojego adresu,
wys³ane na listê o nazwie <#L#>. Aby dostaæ kopiê tych listów
z archiuwm, np. listu o numerze 12345, wyslij pust± wiadomosæ
na adres: <#L#>-get.12345@<#H#>.

</#ia/>
Aby zamówiæ listê tytu³ów i autorów stu ostatnich wiadomo¶ci,
wy¶lij pusty list na adres: <#L#>-index@<#H#>.

Aby dostaæ pêczek listów od 123 do 131 (maksymalnie 100 na jeden raz),
wy¶lij pust± wiadomosæ na adres:
<#L#>-get.123_145@<#H#>.

<//>
Poni¿ej znajduj± sie numery listów:

</text/dig-bounce-num/>

Trzymam indeks przesy³ek, które nie zosta³y prawid³owo dorêczone
pod Twój adres z listy <#L#>-digest. Mam zachowany numer pierwszego
listu ka¿dego przegl±du, z którym mia³e¶ k³opoty. Mo¿esz wiêc
spróbowaæ dostaæ listy z archiwum g³ównego.

Aby dostaæ list nr 12345 z archiwum, wy¶lij pusty list na
adres: <#L#>-get.12345@<#H#>.

</#ia/>
Aby dostaæ listê tytu³ów i autorów ostatnich stu listów, wy¶lij
pust± wiadomosc na adres: <#L#>-index@<#H#>.

Aby dostaæ pêczek listów od numeru 123 do 145 (maksymalnie sto naraz)
wy¶lij pust± wiadomosæ pod adres: <#L#>-get.123_145@<#H#>.

<//>
Poni¿ej znajdziesz numery przegl±dów listy:

</text/bounce-probe/>

Jeste¶ zapisany(a) na listê dyskusyjn± o nazwie <#l#>,
niestety listy z niej nie mog± do Ciebie dotrzeæ, gdy¿ s± odrzucane
przez twój serwer. Wys³a³em Ci ostrze¿enie o tym fakcie,
ale i ono przepad³o... Nie miej wiêc do mnie ¿alu, je¶li
zmuszony bêdê wykresliæ Ciê z listy prenumeratorów, a nast±pi
to w przypadku, je¶li i ten list nie zostanie przyjêty przez Twój
serwer pocztowy.

</text/bounce-warn/>

Poczta kierowana do Ciebie z listy <#l#> jest odrzucana przez Twój serwer.
Za³±czam kopiê pierwszej przesy³ki, z dostarczeniem której mia³em k³opoty.

Je¿eli niniejszy list ostrzegawczy równie¿ przepadnie, wy¶lê Ci
przesy³kê testow±, której odbicie siê od Twojego adresu spowoduje
trwa³e wypisanie z pocztowej listy dyskusyjnej.

</text/digest#ia/>
Aby zaprenumerowaæ przegl±d listy, napisz pod adres:
	<#L#>-digest-subscribe@<#H#>

Aby wykre¶liæ swoój adres pocztowy z listy subskrybentów, napisz pod adres:
	<#L#>-digest-unsubscribe@<#H#>

Aby wys³aæ cokolwiek na listê, pisz pod adres:
	<#L#>@<#H#>

</#iax/>
	Opiekun listy pocztowej - 
        - <<#L#>-owner@<#H#>>

</text/get-bad/>
Przykro mi, tego listu nie ma w archiwum.

</text/help/>
Niestety nie poda³e¶ prawid³owego adresu prenumeraty.
List, który otrzyma³e¶, jest niczym innym, jak standardow± instrukcj±
obs³ugi menad¿era list dyskusyjnych zainstalowanego na naszym serwerze.
Mam nadziejê, ¿e teraz ju¿ sobie poradzisz.

</text/mod-help/>
Dziêkujê za zgodê na zostanie moderatorem listy o adresie:
<#L#>@<#H#>.

Moje komendy s± nieco inne, ni¿ bywaj± w programach obs³uguj±cych
pocztowe listy dyskusyjne. Mo¿esz nawet na pocz±tku stwierdziæ, ¿e s±
niezwyk³e, ale jak tylko zaczniesz ich u¿ywaæ, docenisz prostotê systemu
oraz szybko¶æ, z jak± obs³ugujê twoje ¿yczenia.

Poni¿ej kilka s³ów o tym, jak dzia³a moderowanie: 

Zdalne wpisanie na listê prenumeratorów
---------------------------------------
Bêd±c moderatorem, mo¿esz zapisaæ lub wypisaæ internautê
o adresie janek@komputer-janka.domena poprzez wys³anie listu na adres:

<#L#>-subscribe-janek=komputer-janka.domena@<#H#>
<#L#>-unsubscribe-janek=komputer-janka.domena@<#H#>

</#g/>
Odpowiednio dla przegl±du listy:

<#L#>-digest-subscribe-janek=komputer-janka.domena@<#H#>
<#L#>-digest-unsubscribe-janek=komputer-janka.domena@<#H#>

<//>
To wszystko. Nie musisz nadawaæ listowi tytu³u, nie musisz
równie¿ pisaæ nic w jego tre¶ci!

</#r/>
Wy¶lê Ci pro¶bê o potwierdzenie 
Co zrobiæ z moim listem?
Po prostu odpowiedz na niego i gotowe.
</#R/>
Wy¶lê pro¶bê o potwierdzenie chêci uczestnictwa na adres
autora listu.
Wystarczy, ze na te pro¶be odpowie, a bedzie zapisany.
<//>

Zawiadomiê subskrybenta, je¿eli jego/jej dane dotyczace
jego subskrybcji zostan± zmienione.

Prenumerata
-----------

Ka¿dy mo¿e zapisaæ siê na listê, albo z niej wypisaæ, poprzez
wys³anie listu na adres:

<#L#>-subscribe@<#H#>
<#L#>-unsubscribe@<#H#>

</#g/>
Obs³uga przegl±du listy:

<#L#>-digest-subscribe@<#H#>
<#L#>-digest-unsubscribe@<#H#>

Chêtny otrzyma pro¶be o potwierdzenie ¿yczenia wypisania siê z listy,
aby siê upewniæ czy ¿yczenie to zosta³o rzeczywiscie przez niego
wys³ane. Je¿eli zostanie to potwierdzone, zostanie usuniêty 
z listy prenumeratorów.
Podobna procedura obowi±zuje podczas zapisywania siê na listê.

</#s/>
Podczas procedury zapisywania siê na listê, pro¶ba
o potwierdzenie zapisu jest wysy³ana równie¿ do moderatora listy.
Pozytywna odpowied¼ moderatora dope³ni zapisu na listê.
</#S/>
Zapisy funkcjonuj± w ten sam sposób.
<//>

U¿ytkownik mo¿e równie¿ u¿ywaæ adresów:

<#L#>-subscribe-janek=komputer-janka.domena@<#H#>
<#L#>-unsubscribe-janek=komputer-janka.domena@<#H#>

aby poczta z listy wêdrowa³a na inny adres bed±cy jego w³asno¶ci±.
Tylko wówczas, kiedy u¿ytkownik dostaje listy na adres
janek@komputer-janka.domena, bêdzie w stanie odpowiedzieæ
na ¿yczenie potwierdzenia skierowanego od menad¿era listy.

Wszystkie powy¿sze kroki s± przedsiêbrane po to, by unikaæ
z³o¶liwo¶ci typu zapisanie kogo¶ wbrew jego woli na listê
oraz aby moderator móg³ byæ pewnym, ¿e adres prenumeratora
rzeczywi¶cie istnieje.

Twój rzeczywisty adres pocztowy nie bêdzie ujawniony prenumaratorowi.
Pozwoli to zachowaæ Ci anonimowo¶æ.

</#rl/>
Aby uzyskaæ listê prenumeratorów <#L#>@<#H#>, 
wy¶lij list na adres:
   <<#L#>-list@<#H#>>

Chc±c uzyskaæ dziennik pok³adowy listy <#L#>@<#H#>,
wy¶lij list na adres:
   <<#L#>-list@<#H#>>

</#rld/>
Adresy w³a¶ciwe dla przegl±du listy:
   <<#L#>-digest-list@<#H#>>
i:
   <<#L#>-digest-log@<#H#>>

</#rn/>
Mo¿esz zdalnie zmieniaæ pliki tekstowe, którymi pos³uguje siê automat
podczas obs³ugi listy dyskusyjnej. Aby uzyskaæ zbiór plików oraz
instrukcje, jak je zmieniaæ, napisz pod adres:
   <<#L#>-edit@<#H#>>

</#m/>
Przesy³ki moderowane
--------------------
Je¿eli przesy³ki podlegaj± moderowaniu, umieszczam wys³ane listy
w kolejce pocztowej i wysy³am ¿yczenie przyjrzenia sie listowi do
moderatora.

Wystarczy, aby¶ odpowiedzia³ na adres zwrotny tego listu, u¿ywaj±c
funkcji "odpowiedz (reply)", aby zaakceptowaæ tre¶æ listu. 

Je¿eli natomiast chcesz odmówiæ zgody na wys³anie listu, wy¶lij
pocztê na adres wziêty z nag³ówka "From:" (mo¿na zwykle tego
dokonaæ poprzez u¿ycie opcji "reply-to-all/odpowiedz-na-wszystkie-
-adresy" i skasowanie z tej listy w³asnego adresu oraz adresu 
akceptuj±cego). Mo¿esz równie¿ dodaæ jak±¶ notkê do wysy³aj±cego
odrzucony przez Ciebie list, aby wyt³umaczyæ mu, dlaczego to
uczyni³e¶. Notkê dodajesz umieszczaj±c j± w tre¶ci powy¿szego listu
odrzucaj±cego, wstawiaj±c j± pomiêdzy dwie linie, zawieraj±ce
trzy znaki '%'.

Wezmê pod uwagê pierwszy list, który od Ciebie dostanê.
Je¿eli wy¶lesz najpierw potwierdzenie akceptacji listu, który
wcze¶niej odrzuci³e¶, lub odwrotnie, oczywi¶cie Ci o tym powiem.

Je¿eli nie dostanê odpowiedzi od moderatora przez okre¶lony czas,
zwykle jest to piêæ dni, zwrócê list nadawcy z opisem zaistnia³ej
sytuacji.
<//>

Wakacje
-------
Je¿eli korzystasz tymczasowo z innego adresu, po prostu przekieruj
wszystkie listy z nag³ówkiem 'Mailing-List:',
albo te, których tematy zaczynaj± siê s³owami
"MODERATE for <#L#>@<#H#>",
lub
"CONFIRM subscribe to <#L#>@<#H#>",
na nowy adres.
Mo¿esz wtedy moderowaæ z nowego adresu. Innym sposobem jest
przekierowanie tych listów do przyjaciela, który mo¿e Ciê zast±piæ.
Musisz oczywi¶cie uzgodniæ przedtem wszystko z opiekunem listy.

Je¿eli chcesz automatycznie aprobowaæ wszelkie przesy³ki kierowane
na listê, skonfiguruj swój program pocztowy tak, aby automatycznie
odpowiada³ na listy maj±ce tematy zgodne z powy¿szymi kryteriami.

</#r/>
Uwa¿aj, je¿eli bêdziesz próbowa³ zdalnie zarz±dzaæ list± z adresu,
który nie jest wpisany jako adres administratora, prenumerator,
a nie Ty, zostanie poproszony o potwierdzenie, które zostanie
potem przekazane do moderatorów.
Robiê tak, gdy¿ nie ma sposobu upewniæ siê czy to rzeczywi¶cie Ty
kryjesz siê za poczt± przychodz±c± z nieznanego adresu.

Pamiêtaj, ¿e w powy¿szym wypadku Twoje ¿ycznie skierowane do serwera
i równie¿ Twój adres s± przesy³ane do prenumeratora! Przestajesz byæ
wówczas anonimowy.
<//>

Powodzenia!

PS: Skontaktuj siê, proszê, z opiekunem listy pod adresem
<#L#>-owner@<#H#>
je¿eli masz jakie¶ pytania, lub napotkasz na problemy.

</text/mod-reject/>
Przykro mi, ale Twój list nie zosta³ zaakceptowany przez moderatora.
Je¿eli moderator uczyni³ jakie¶ uwagi, zamieszczam je poni¿ej.
</text/mod-request/>
Proszê o decyzjê czy b±d±c moderatorem zgadzasz siê na wys³anie
za³±czonej przesy³ki na listê o nazwie <#L#>.

Aby zgodziæ siê na natychmiastowe wys³anie listu do wszystkich
prenumeratorów, wy¶lij, proszê, list na adres:

# This does the default !A for normal setups, but puts in a 'mailto:address'
# for lists created with the -x switch.
!A
</#x/>

mailto:<#A#>
# Below is a minimalist tag. It just means that succeeding lines should be
# added to the currently open file in all cases.
<//>

Aby odmówiæ wys³ania listu, i zwróciæ go nadawcy, proszê
przes³aæ wiadomosæ na adres: 

!R
</#x/>

mailto:<#R#>
<//>

Nie musisz za³±czaæ kopii listu, który akceptujesz (lub nie) do
wys³ania na listê. Je¿eli chcesz przes³aæ jak±¶ notkê do nadawcy
odrzuconego listu, zawrzyj j± pomiêdzy dwie linie zaczynaj±ce siê
trzema znakami procentu ("%").

%%% Pocz±tek notki
%%% Koniec notki

Dziêkujê za pomoc!

--- W za³±czniu mo¿esz znale¼æ wys³any list.

</text/mod-sub#E/>
--- zapisa³em Ciê (lub wypisa³em) z listy
<#l#>@<#H#>
na ¿yczenie jej moderatora.

Je¶li nie jest to dzia³anie, którego pragn±³e¶, mo¿esz
przes³aæ swoje w±tpliwo¶ci do opiekuna listy:
<#l#>-owner@<#H#>

Je¿eli potrzebujesz informacji o tym, jak dostaæ siê do archiwum
listy <#L#>, prze¶lij pusty list na adres:
<#L#>-help@<#H#>

</text/mod-timeout/>
Przykro mi, ale moderatorzy nie zareagowali na Twój list.
Dlatego zwracam go Tobie, je¿eli uwa¿asz, ¿e kryje siê za tym
jaki¶ b³±d maszyny, wy¶lij list ponownie, albo skontaktuj siê
bezpo¶rednio z moderatorem listy.

--- W za³±czeniu Twoja oryginalna przesy³ka na listê.

</text/mod-sub-confirm/>
Bardzo proszê o zgodê na dodanie internauty o adresie:

<#A#>

jako prenumeratora listy dyskusyjnej <#l#>.
Poniewa¿ powy¿sze ¿yczenie mog³o nie pochodziæ od Ciebie (patrz
koniec listu), ju¿ zd±¿y³em potwierdziæ, ¿e internauta o adresie:
<#A#>
rzeczywiscie podobne ¿yczenie wys³a³.

Aby udzieliæ zgody, proszê wys³aæ pust± odpowiedz na adres:

!R
</#x/>

mailto:<#R#>
<//>

Twój program pocztowy powienien automatycznie prawid³owo zaadresowaæ
list, je¿eli u¿yjesz opcji "Odpowiedz/Reply".

Je¿eli natomiast siê nie zgadzasz, po prostu zignoruj ten list.

Dziekujê za pomoc!

</text/mod-unsub-confirm/>
Bardzo proszê o Twoj± zgodê na usuniêcie internauty o adresie:

!A

z listy prenumeratorów listy <#l#>. 
Je¿eli siê zgadzasz, wyslij, proszê, pust± odpowiedz na adres:

!R
</#x/>

mailto:<#R#>
<//>

Twój program pocztowy powienien automatycznie prawid³owo zaadresowaæ
list, je¿eli u¿yjesz opcji "Odpowiedz/Reply".

Dziekujê za pomoc!

</text/sub-bad/>
Hmmm, numer potwierdzenia wydaje mi siê nieprawid³owy.

Najczêstsz± przyczyn± nieprawid³owo¶ci jest jego przeterminowanie.
Muszê dostaæ potwierdzenie ka¿dego ¿ycznia w ci±gu dziesiêciu dni.
Mo¿esz równie¿ upewniæ siê czy za³±czy³e¶(a¶) ca³y numer potwierdzenia
w poprzednim liscie, niektóre programy pocztowe maj± z³y nawyk
ucinania adresów, które wydaj± im siê zbyt d³ugie.

Przygotowa³em nowy numer potwierdzenia. Aby ponownie potwierdziæ
chêæ zapisania siê z adresu:

!A

na listê dyskusyjn± <#L#>, wy¶lij pust± odpowied¼
pod adres:

!R
</#x/>

mailto:<#R#>
<//>

Pamiêtaj, sprawd¼ dok³adnie czy zalaczy³e¶(a¶) pe³ny numer, oczywi¶cie
uczyñ to przed wys³aniem listu ;-)

Przepraszam za k³opoty.
Opiekun listy -
</#x/>
<#L#>-owner@<#H#>
<//>

</text/sub-confirm/>
Aby potwierdziæ chêæ zapisania siê z adresu:

!A

na listê dyskusyjn± o nazwie <#L#>,
wy¶lij, proszê, pust± odpowied¼ pod adres:

!R
</#x/>

mailto:<#R#>
<//>

Twój program pocztowy powienien automatycznie zaadresowaæ przesy³kê,
je¿eli u¿yjesz opcji "odpowiedz/reply".

Potwierdzenie ma na celu
a) sprawdzenie siê czy potrafiê wysy³aæ listy na adres przez Ciebie podany,
b) upewnienie siê czy ktos nie zrobi³ Ci g³upiego dowcipu, zapisuj±c Ciê
   bez twojej wiedzy na nasz± listê.

</text/sub-confirm#s/>
Lista jest moderowana. Kiedy wys³a³e¶ ju¿ potwierdzenie, chêæ prenumeraty
zosta³a przes³ana moderatorowi. Zawiadomiê Ciê odrêbnym listem, je¶li
zostaniesz wpisany(a) na listê prenumeratorów.

</text/sub-nop/>

Potwierdzenie: Adres poczty elektronicznej:

!A

jest ju¿ na li¶cie prenumeratorów <#L#>. By³ on ju¿ na li¶cie
przed wys³aniem ponownej pro¶by, a wiêc j± zignorujê.

</text/sub-ok#E/>
Potwierdzenie: Doda³em adres

!A

do listy prenumeratorów <#L#>.

**** WITAJ NA LI¦CIE <#L#>@<#H#>!

Proszê o zachowanie tej przesy³ki, bêdziesz dziêki niej pamiêta³,
z którego dok³adnie adresu jestes zapisany(a), informacja ta bêdzie
niezbêdna, je¶li chcia³(a)by¶ kiedy¶ z listy siê wypisaæ lub zmieniæ
adres korespondencyjny.

</text/top/>
Czesæ! Nazywam siê "ezmlm" i zarz±dzam serwerem pocztowych
list dyskusyjnych, miêdzy innymi list± o nazwie:
<#L#>@<#H#>

</#x/>
Jestem komputerem, bezdusznym s³ug± opiekuna listy, cz³owieka :-),
którego mo¿na znale¼æ pod adresem:
<#L#>-owner@<#H#>.

</text/unsub-bad/>
Hmmm, numer potwierdzenia wydaje mi siê nieprawid³owy.

Najczêstsz± przyczyn± nieprawid³owo¶ci jest przeterminowanie,
muszê dostaæ potwierdzenie ka¿dego ¿ycznia w ci±gu dziesiêciu dni.
Mo¿esz równie¿ upewniæ siê czy za³±czy³e¶(a¶) ca³y numer potwierdzenia
w poprzednim li¶cie, niektóre programy pocztowe maj± z³y nawyk
ucinania adresów, które wydaj± im siê zbyt d³ugie.

Przygotowa³em nowy numer potwierdzenia. Aby ponownie potwierdziæ
¿e mam usun±æ adres

!A

z listy prenumeratorów <#l#>, wy¶lij pust± odpowiedz na adres:

!R
</#x/>

mailto:<#R#>
<//>

Pamiêtaj, sprawd¼ dokladnie czy za³aczy³e¶(a¶) pe³ny numer potwierdzenia,
uczyñ to oczywi¶cie przed wys³aniem listu ;-)

Przepraszam za k³opoty.
Opiekun -
<#L#>-owner@<#H#>

</text/unsub-confirm/>
Aby potwierdziæ, ¿e chcesz usunac adres

!A

z listy prenumaratorów <#L#>, prze¶lij, proszê, pust±
odpowied¼ na ten list pod adres:

!R
</#x/>

mailto:<#R#>
<//>

Twój program pocztowy powienien automatycznie zaadresowaæ przesy³kê,
je¿eli u¿yjesz opcji "odpowiedz/reply".

Nie sprawdzi³em jednak czy twój adres znajduje siê na li¶cie
prenumeratorów. Aby sprawdziæ, którego adresu u¿ywasz jako koresponde-
cyjnego, spojrzyj na jak±kolwiek wiadomosæ z listy dyskusyjnej.
Ka¿dy list ma schowany Twój adres w scie¿ce zwrotnej; np.
Bóg@niebo.prezydent.pl dostaje listy ze scie¿k± zwrotn±:
<<#l#>-return-<number>-Bóg=niebo.prezydent.pl@<#H#>.

</text/unsub-nop/>
Powiadomienie: Adresu pocztowego

!A

nie ma na li¶cie prenumeratorów <#l#>. Nie by³o go równie¿
przed dosteniem Twojego ¿yczenia wykre¶lenia z listy.. 

Je¿eli wypisa³e¶(a¶) siê, lecz nadal dostajesz listy, jeste¶ po prostu
zapisany(a) z innego adresu, ni¿ ten, którego w³a¶nie u¿ywasz.
Spojrzyj, proszê, ma nag³ówki któregokolwiek listu z prenumeraty,
znajdziesz tam frazê:

'Return-Path: <<#l#>-return-1234-user=host.dom@<#H#>>'

W³a¶ciwy adres, na który powinno wiêc trafiæ twoje ¿yczenie wypisania to:
'<#l#>-unsubscribe-user=host.dom@<#H#>'.
Po prostu napisz na powy¿szy adres, zamieniaj±c user=host.dom na twoje
prawdziwe dane, odpowiedz po¼niej na pro¶bê potwierdzenia, 
powieniene¶(a¶) wówczas dostaæ list, ¿e jeste¶ wykre¶lony(a)
z listy prenumeratorów.

Je¿eli moje rady nie skutkuj±, niestety nie mogê Ci ju¿ wiêcej pomóc.
Prze¶lij, proszê, jak±kolwiek przesy³kê z listy wraz z krótk± informacj±,
co chcesz osi±gn±æ, do opiekuna listy, pod adres:

</#x/>
mailto:<#L#>-owner@<#H#>
</#X/>
    <#l#>-owner@<#H#>
<//>

który to opiekun siê tym zajmie. We¼ pod uwagê, ¿e cz³owiek jest nieco
wolniejszy od komputera :-), wiêc b±d¼ cierpliwy(a).

</text/unsub-ok/>
Zawiadomienie: usun±³em adres pocztowy

!A

z listy prenumeratorów <#l#>.
Na adres ten nie bêd± ju¿ przychodzi³y przesy³ki z listy.

</text/edit-do#n/>
Przerób, proszê, za³±czony tekst i wy¶lij go na adres:

!R

Twój program powienien uczyniæ to automatycznie, je¿eli u¿yjesz
funkcji "reply/odpowiedz".

Mogê bez k³opotu usun±æ dodatki jakie mo¿e dodawaæ do tekstów
Twój program pocztowy, je¿eli zachowasz linie znaczników.

Linie znaczników s± to linie rozpoczynaj±ce siê "%%%".
Nie mog± one byæ zmienione, dodatkowe znaki dodane przez program
pocztowy na pocz±tku linii s± dozwolone.


</text/edit-list#n/>
Pliki <#L#>-edit.nazwa_pliku mog± byæ stosowane przez zdalnego
administratora do edycji tekstów zarz±dczych listy dyskusyjnej.

Tabela pod spodem jest list± plików wraz z krótkim opisem
ich dzia³ania. Aby zamieniæ okre¶lony plik, po prostu wy¶lij
pusty list na adres <#L#>-edit.nazwa_pliku ,
zamieniaj±c wyra¿enie 'nazwa_pliku' nazw± zamienianego pliku.
Instrukcja sposobu zamiany pliku zostanie dostarczona wraz ze
starym plikiem poczt± zwrotn±.

File                Use

bottom              spód ka¿dej odpowiedzi administracyjnej. G³ówne komendy.
digest              administracyjna czê¶æ przegl±du listy.
faq                 najczê¶ciej zadawane pytania na naszej li¶cie.
get_bad             tekst w miejsce listu nie znalezionego w archiwum.
help                g³ówna tre¶æ pomocy.
info                informacje o naszej li¶cie.
mod_help            pomoc dla moderatorów list.
mod_reject          list przesy³any nadawcy przesy³ki nie zaakceptowanej.
mod_request         tekst przesy³any razem z pro¶b± moderowania.
mod_sub             tekst przesy³any prenumeratorowi po zaakceptowaniu.
mod_sub_confirm     tekst do moderatora z pro¶b± o wpisanie na listê.
mod_timeout         do nadawcy przterminowanego listu.
mod_unsub_confirm   do zdalnego opiekuna z pro¶b± o potwierdzenie wypisu.
sub_bad             do prenumaratora, je¿eli potwierdzenie by³o z³e.
sub_confirm         do prenumeratora z pro¶b± dokonania potwierdzenia.
sub_nop             tre¶æ listu powitalnego dla ponownie subskrybuj±cego.
sub_ok              tre¶æ listu powitalnego dla nowozapisanego.
top                 pocz±tek ka¿dego listu, ostro¿nie!
</#tn/>
trailer             tekst dodawany do ka¿dego listu, ostro¿nie!
</#n/>
unsub_bad           do prenumeratora, je¶li jego potwierdzenie jest z³e.
unsub_confirm       tre¶æ potwierdzenia po ¿yczeniu wypisania siê z listy.
unsub_nop           do osoby nie bêd±cej prenumeratorem, wypisuj±cej siê.
unsub_ok            list po¿egnalny do rezygnuj±cego z prenumeraty.

</text/edit-done#n/>
Plik tekstowy zosta³ szczê¶liwie zamieniony.

</text/info#E/>
Nie mam ¿adnych informacji na temat listy :-(
</text/faq#E/>
FAQ - najczê¶ciej zadawane pytania na li¶cie
<#l#>@<#H#>.

Jeszcze nic tu nie ma :-(, przepraszamy!


