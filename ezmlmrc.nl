0.40 - This version identifier must be on line 1 and start in pos 1.
#
# NOTE: This file has not been tested in a production environment!
#
#$Id: ezmlmrc.nl,v 0.02 2003/06/22 21:43:12 oldeman Exp $
#$Name: ezmlm-idx-040 $
#
# Nederlandse vertaling: Willem Oldeman <willem@king-pin.nl>
#                        Op- of aanmerkingen over deze vertaling
#                        rechtstreeks naar mij sturen s.v.p.
#
# Dutch translation: Willem Oldeman <willem@king-pin.nl>
#                    Please send any comments regarding this
#                    translation directly to me.
#
# Changes:
# 2003/04/28     This is the first translation, inspired by the
#                english version and by Frank Tegtmeyer, who did
#                the german translation
# 2003/06/22     Finished translating the rest of the file,
#                it took most of my Sunday afternoon
# 2003/01/08     Missed line "Administrative commands" (line 466)
#
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
# -3 is for the new from header if we want that header replaced
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
X:<#X#>
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
# clean state.
</-modpost#eM/>
</-modsub#eS/>
</-remote#eR/>
</-public#eP/>
</-indexed#eA/>
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
</manager#ab/>
|<#B#>/ezmlm-get -P '<#D#>' <#C#>
</manager#aGB/>
|<#B#>/ezmlm-get '<#D#>' <#C#>
</manager#agB/>
|<#B#>/ezmlm-get -s '<#D#>' <#C#>
</manager#q/>
|<#B#>/ezmlm-request '<#D#>'
# Ok to add -l/-d even for non-mod lists, since ezmlm-manage
# won't allow it unless there are remote admins. The lack of logic other than
# AND makes this very tedious ...
# first lists with normal confirmation:
</manager#LNHJ/>
|<#B#>/ezmlm-manage '<#D#>'
</manager#lNHJ/>
|<#B#>/ezmlm-manage -l '<#D#>'
</manager#LnHJ/>
|<#B#>/ezmlm-manage -e '<#D#>'
</manager#lnHJ/>
|<#B#>/ezmlm-manage -le '<#D#>'
# ... now no confirmation for subscribe ...
</manager#LNhJ/>
|<#B#>/ezmlm-manage -S '<#D#>'
</manager#lNhJ/>
|<#B#>/ezmlm-manage -lS '<#D#>'
</manager#LnhJ/>
|<#B#>/ezmlm-manage -eS '<#D#>'
</manager#lnhJ/>
|<#B#>/ezmlm-manage -leS '<#D#>'
# ... now no confirmation for unsubscribe ...
</manager#LNHj/>
|<#B#>/ezmlm-manage -U '<#D#>'
</manager#lNHj/>
|<#B#>/ezmlm-manage -lU '<#D#>'
</manager#LnHj/>
|<#B#>/ezmlm-manage -eU '<#D#>'
</manager#lnHj/>
|<#B#>/ezmlm-manage -leU '<#D#>'
# ... and finally no confirmation at all ...
</manager#LNhj/>
|<#B#>/ezmlm-manage -US '<#D#>'
</manager#lNhj/>
|<#B#>/ezmlm-manage -lUS '<#D#>'
</manager#Lnhj/>
|<#B#>/ezmlm-manage -eUS '<#D#>'
</manager#lnhj/>
|<#B#>/ezmlm-manage -leUS '<#D#>'
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
|<#B#>/ezmlm-issubn -n '<#D#>/deny' || { echo "Sorry, ik heb opdracht gekregen om uw berichten te weigeren. U kunt contact opnemen met <#L#>-owner@<#H#> als u hier vragen over heeft (#5.7.2)"; exit 100 ; }
# switch -u=> restrict to subs of list & digest. If not m
# do it with ezmlm-issubn, if 'm' do it with ezmlm-gate
</#uM/>
|<#B#>/ezmlm-issubn '<#D#>' '<#D#>/digest' '<#D#>/allow' '<#D#>/mod' || { echo "Sorry, alleen leden mogen berichten insturen. Als u lid bent, stuur dit bericht dan door naar <#L#>-owner@<#H#> om uw adres toe te laten voegen (#5.7.2)"; exit 100 ; }
</#um/>
|<#B#>/ezmlm-gate '<#D#>' '<#D#>' '<#D#>/digest' '<#D#>/allow' '<#D#>/mod'
# For message moderation, editor has store/clean
</#mUO/>
|<#B#>/ezmlm-store '<#D#>'
</#mUo/>
|<#B#>/ezmlm-store -P '<#D#>'
</#mU/>
|<#B#>/ezmlm-clean '<#D#>' || exit 0
</#mu/>
|<#B#>/ezmlm-clean -R '<#D#>' || exit 0
# for non-message moderated lists, it has send
</#M/>
|<#B#>/ezmlm-send '<#D#>'
# ezmlm-archive here for normal lists. Put into moderator for mess-mod lists
</#Mi/>
|<#B#>/ezmlm-archive '<#D#>' || exit 0
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
# bouncer for list and digest
</bouncer/>
|<#B#>/ezmlm-weed
|<#B#>/ezmlm-return -D '<#D#>'
</digest/bouncer#d/>
|<#B#>/ezmlm-weed
|<#B#>/ezmlm-return -d '<#D#>'
# moderator is set up only for message moderated lists. However, '-e' does
# not remove it since we can't remove the symlinks to it (they're outside
# of the list dir.
</moderator#m/>
|<#B#>/ezmlm-moderate '<#D#>'
</#mi/>
|<#B#>/ezmlm-archive '<#D#>' || exit 0
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
list-subscribe
list-unsubscribe
list-help
</headerremove#E^0/>
# For sublists, these should be left in
list-post
# remove from header if -3 'new_from_line'
</#3E/>
from
</lock/>
</lockbounce/>
</digest/lockbounce#d/>
</digest/lock#d/>
</public#p/>
</archived#a/>
</indexed#a/>
</inhost/>
<#H#>
</outhost/>
<#H#>
</outlocal/>
<#L#>
</mailinglist/>
contact <#L#>-help@<#H#>; beheerd door ezmlm
# Headeradd needs to always exist but leave out stuff for sublists
</headeradd#E^0/>
# Good for mailing list stuff (and vacation program)
Precedence: bulk
# To prevent indexing by findmail.com
X-No-Archive: yes
# rfc2369, first from main list only, others from sublist only
List-Post: <mailto:<#L#>@<#H#>>
</headeradd#E/>
List-Help: <mailto:<#l#>-help@<#h#>>
List-Unsubscribe: <mailto:<#l#>-unsubscribe@<#h#>>
List-Subscribe: <mailto:<#l#>-subscribe@<#h#>>
# add new from line "From: arg" if -3 'arg'
</#3E/>
From: <#3#>
# max & min message size
</msgsize#x/>
30000:2
# remove mime parts if -x
</mimeremove#xE/>
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
</prefix#fE/>
[<#L#>]
</text/trailer#tE/>
---------------------------------------------------------------------
Om uit te schrijven, stuur een e-mail naar: <#L#>-unsubscribe@<#H#>
Voor extra commando's, stuur een e-mail naar: <#L#>-help@<#H#>
</text/bottom#E/>

--- Commando's voor de <#l#> lijst ---

Ik kan automatisch administratieve verzoeken afhandelen. Stuur
deze s.v.p. niet naar het list adres! U moet uw verzoek naar het
goede commando adres sturen:

Voor help en uitleg voor de verschillende verzoeken dient u
een bericht te sturen naar:
   <<#L#>-help@<#H#>>

Om u in te schrijven, stuurt u een bericht aan:
   <<#L#>-subscribe@<#H#>>

Om u uit te schrijven stuurt u een bericht naar het adres wat in
de ``List-Unsubscribe'' HEADER staat van alle list berichten.
Als uw adres ongewijzigd is gebleven sinds uw inschrijving, kunt u
ook een bericht sturen naar:
   <<#L#>-unsubscribe@<#H#>>

</#dE/>
of voor de digest naar:
   <<#L#>-digest-unsubscribe@<#H#>>

</#HEJ/>
Om adressen toe te voegen of te verwijderen, zal ik een verzoek
om bevestiging sturen naar het opgegeven adres. Als u dat ontvangt
kunt u door middel van een reply uw wens bevestigen.

</#E/>
Indien u contact wilt opnemen met de eigenaar van deze lijst, stuurt
u een bericht naar:

    <<#L#>-owner@<#H#>>

Sluit s.v.p. een ge-forward bericht mee, waarin alle HEADERS intact
zijn gelaten. Dan is het makkelijker om u te helpen.

--- In de bijlage is een kopie van het verzoek wat ik heb ontvangen.

</text/bounce-bottom#E/>

--- Hier is een kopie van het BOUNCE bericht wat ik heb ontvangen.

</text/bounce-num#E/>

Ik heb een lijst bewaard met berichten van de <#L#> mailing list die
ge-bounced werden op uw adres.

</#aE/>
Deze berichten kunnen nog in het archief staan.

</#aE/>
Om de berichten 123-145 op te vragen (maximaal 100 per verzoek),
stuurt u een e-mail naar:
   <<#L#>-get.123_145@<#H#>>

Om een overzicht van onderwerpen en schrijver te ontvangen van de
laatste 100 berichten, stuurt u een e-mail naar:
   <<#L#>-index@<#H#>>

</#E/>
Dit zijn de nummers van de berichten:

</text/dig-bounce-num#E/>

Ik heb een lijst met digests bewaard van de <#L#>-digest mailing list
die ge-bounced werden op uw adres.
Van iedere digest die u gemist hebt, heb ik het nummer bewaard van
het eerste bericht in de digest.

</#aE/>
Ik bewaar de digests niet, maar u kunt de berichten uit het
archief ophalen.

Om berichten 123-145 op te halen (maximaal 100 per verzoek),
stuurt u een e-mail naar:
   <<#L#>-get.123_145@<#H#>>

Om een overzicht te ontvangen van onderwerpen en schrijvers van
de laatste 100 berichten, stuurt u een e-mail naar:
   <<#L#>-index@<#H#>>

</#E/>
Dit zijn de digest bericht nummers:

</text/bounce-probe#E/>

De berichten die u worden gestuurd door de <#l#> mailing list
worden ge-bounced. Ik heb u hierover een waarschuwing gestuurd,
maar dat bericht werd ook gebounced.
Ik voeg een kopie bij van het bounce-bericht wat ik heb gekregen.

Dit is een test om te zien of uw adres bereikbaar is. Als deze test
ook ge-bounced word, zal ik uw adres van de <#l#>@<#H#>
mailing list verwijderen.

U kunt zich opnieuw inschrijven door een e-mail te sturen naar:
   <<#l#>-subscribe@<#H#>>

</text/bounce-warn#E/>

De berichten die u worden gestuurd door de <#l#> mailing list
lijken te bouncen. Ik voeg een kopie bij van het eerste bounce-
bericht wat ik heb ontvangen.

Als dit bericht weer ge-bounced wordt, stuur ik een test om te zien
of uw adres bereikbaar is. Als deze test ook ge-bounced wordt
zal ik uw adres van de <#l#> mailing list verwijderen.

</text/digest#dE/>
Om u in te schrijven op de digest, stuurt u een e-mail naar:
	<<#L#>-digest-subscribe@<#H#>>

Om u uit te schrijven van de digest, stuurt u een e-mail naar:
	<<#L#>-digest-unsubscribe@<#H#>>

Om een bericht naar de mailing list te sturen, stuurt u de e-mail naar:
	<<#L#>@<#H#>>

</text/get-bad#E/>
Sorry, dat bericht staat niet in het archief.

</text/help#E/>
Dit is een algemeen help bericht. Het bericht wat ik gekregen heb, is
niet naar mijn commando adressen gestuurd.

Hier is een lijst van gebruikte commando adressen:

Stuur een e-mail voor info en veel gestelde vragen (FAQ) naar:
   <<#L#>-info@<#H#>>
   <<#L#>-faq@<#H#>>

</#dE/>
Gelijksoortige adressen bestaan voor deze list:
   <<#L#>-digest-subscribe@<#H#>>
   <<#L#>-digest-unsubscribe@<#H#>>

# ezmlm-make -i needed to add ezmlm-get line. If not, we can't do
# multi-get!
</#aE/>
Om berichten 123-145 te ontvangen (maximaal 100 per verzoek),
stuurt u een e-mail naar:
   <<#L#>-get.123_145@<#H#>>

Om een overzicht van onderwerpen en schrijvers van berichten 123-456
te ontvangen, stuurt u een e-mail naar:
   <<#L#>-index.123_456@<#H#>>

Om alle berichten te ontvangen met hetzelfde onderwerp als bericht 12345
stuurt u een e-mail naar:
   <<#L#>-thread.12345@<#H#>>

</#E/>
De e-mail die u stuurt hoeft niet leeg te zijn, maar ik zal
hun inhoud negeren. Alleen het ADRES waar u naar toe stuurt
is voor mij van belang.

U kunt zich onder een ander adres inschrijven, bijvoorbeeld
"john@host.domain".
Voeg een minteken toe en het adres waarin u het @ teken vervangt
door een =, achter het commando:
    <<#L#>-subscribe-john=host.domain@<#H#>>

Om uw andere adres uit te schrijven stuurt u een e-mail naar:
    <<#L#>-unsubscribe-john=host.domain@<#H#>>

</text/mod-help#E/>
Dank u dat u de <#L#>@<#H#> mailing list wilt moderaten.

Mijn commando's zijn anders dan die van andere mailing lists,
maar ik denk dat u ze intuitief en gemakkelijk zult vinden.

Hier zijn een paar aanwijzingen voor de taken die u als eigenaar
of moderator moet uitvoeren.

Algemene list instructies staan aan het eind van dit bericht.

Remote inschrijven:
-------------------
Als moderator kunt u ieder adres in- of uitschrijven van de list.
Om bijvoorbeeld "john@host.domain" in te schrijven, stuurt u een
e-mail naar <<#L#>-subscribe-john=host.domain@<#H#>>.
Achter het subscribe commando geeft u een minteken in en het adres
wat u in wilt schrijven. In dat adres vervangt u het @ teken voor een =.

Op gelijke wijze kunt u een adres verwijderen door een e-mail
te sturen naar:
   <<#L#>-unsubscribe-john=host.domain@<#H#>>

</#dE/>
Voor de digest list is dat:
   <<#L#>-digest-subscribe-john=host.domain@<#H#>>
   <<#L#>-digest-unsubscribe-john=host.domain@<#H#>>

</#E/>
Dat is alles. Subject en tekst hoeft u niet in te voeren.

</#rE/>
Ik zal u een verzoek om bevestiging sturen, om zeker te maken
dat u het verzoek heeft ingediend. U kunt reply-en en
uw wens wordt uitgevoerd.
</#RE/>
Ik zal een verzoek naar dat adres sturen, in dit geval
<john@host.domain>. De gebruiker hoeft alleen maar een reply
te sturen om te bevestigen.
</#E/>

De bevestigingen zijn bedoeld om het voor derden moeilijk te
maken om anderman's adressen aan de lijst toe te voegen.

Ik zal de gebruiker informeren als zijn of haar inschrijf-
status veranderd is.

Inschrijven
-----------

Iedere gebruiker kan zich in- of uitschrijven door
een e-mail te sturen naar:

    <<#L#>-subscribe@<#H#>>
    <<#L#>-unsubscribe@<#H#>>

</#dE/>
Voor de digest list:

    <<#L#>-digest-subscribe@<#H#>>
    <<#L#>-digest-unsubscribe@<#H#>>

</#E/>
De gebruiker zal een verzoek om bevestiging ontvangen
om zeker te maken dat hij of zij eigenaar is van dat adres.
Zodra dit bevestigd is, wordt de gebruiker uitgeschreven.

</#sE/>
Omdat deze list "moderated" is, stuur ik een tweede verzoek naar
de moderators. Omdat de gebruiker al aan heeft gegeven zich in
te willen schrijven, kunt u als moderator zeker zijn dat het
adres van de gebruiker echt is.
Als u uw goedkeuring wilt geven aan het verzoek van deze gebruiker
stuurt u een reply op mijn "CONFIRM" bericht.
Als u geen goedkeuring geeft, kunt u mijn bericht verwijderen of
kunt u contact opnemen met de gebruiker voor meer informatie.
</#SE/>
Inschrijvingen werken op dezelfde manier.
</#E/>

De gebruiker kan ook het volgende manier gebruiken:

   <<#L#>-subscribe-mary=host.domain@<#H#>>
   <<#L#>-unsubscribe-mary=host.domain@<#H#>>

om zich in te schrijven als "mary@host.domain". Alleen als zij
mail ontvangt op dat adres kan zij mijn verzoek om bevestiging
ontvangen en bevestigen.

Uw adres in identiteit worden niet zichtbaar gemaakt aan de
gebruiker die zich in wil schrijven, tenzij u rechtstreeks e-mail
aan hem of haar stuurt.

</#rlE/>
Om een lijst met leden te ontvangen van <#L#>@<#H#>, stuurt u
een e-mail naar:
   <<#L#>-list@<#H#>>

Om een log te ontvangen van <#L#>@<#H#>, stuurt u
een e-mail naar:
   <<#L#>-log@<#H#>>

</#rldE/>
Voor digest subscribers:
   <<#L#>-digest-list@<#H#>>
en:
   <<#L#>-digest-log@<#H#>>

</#rnE/>
U kunt de teksten zelf aanpassen van de berichten die de mailing list
verstuurt. Om de lijst met teksten en instructies te ontvangen
stuurt u een e-mail naar:
   <<#L#>-edit@<#H#>>

</#mE/>
Moderated
---------
Als berichten "moderated" zijn, zal ik het bericht bewaren en u
een kopie sturen met instructies. Het bericht wat u ontvangt zal
"MODERATE for ..." in de subject regel hebben.

Om het bericht te accepteren, kunt u reply-en naar het "Reply-To:"
adres. U hoeft het bericht zelf niet mee te sturen. (In feite negeer
ik de inhoud van het bericht. Ik kijk alleen naar het adres waarnaar
u antwoordt.

Om het bericht te weigeren, stuurt u een email naar het "From:" adres.
Dit kan doorgaans door een "Reply-to-all" te doen en dan alle
adressen behalve het "reject" adres te verwijderen.
U kunt de afzender informatie sturen door commentaar tussen de twee
regels te zetten die met drie % beginnen.
Nogmaals: Uw identiteit zal ik niet vrijgeven.

Ik zal het bericht verwerken aan de hand van de eerste reply die ik
ontvang. Als ik een verzoek krijg om het bericht te accepteren als ik al
een verzoek om afwijzing heb ontvangen, of vice versa, zal ik u dat
laten weten.

Als ik (normaliter binnen 5 dagen) geen moderate reply heb ontvangen,
zal ik het bericht aan de afzender terugsturen met een verklaring.
Uw administrator kan de list dusdanig opzetten dat dergelijke berichten
gewist worden, zonder kennisgeving.
</#E/>

Vakantie
--------
Als u tijdelijk op een ander adres bereikbaar bent, kunt u alle berichten
forwarden (met de goede "Mailing-List:" header) naar het nieuwe adres.
Vanaf het nieuwe adres kunt u dan "moderaten".
Als alternatief kunt de de berichten forwarden naar een vriend(in) zodat
hij of zij de taak van u over kan nemen. Stem dit wel af met de
eigenaar van de list.

Als u automatisch alle verzoeken wilt accepteren als u weg bent, kunt
u uw mailprogramma instellen om een auto-reply te geven.

</#rE/>
Als u vanaf een ander adres administratieve handelingen uit wilt voeren,
wordt de gebruiker gevraagd om bevestiging, niet aan u.
Daarna wordt er een bevestigingsverzoek aan alle moderatoren gestuurd.
Dat doe ik omdat ik niet kan weten of u het originele verzoek hebt gestuurd.

Onthoud dat uw originele verzoek (en adres) in dit geval wel naar de
gebruiker worden gestuurd.
</#E/>

Veel succes gewenst!

PS: U kunt contact opnemen met de eigenaar (<#L#>-owner@<#H#>) als u vragen
hebt of problemen ondervind.

</text/mod-reject#E/>
Sorry, uw (begesloten) bericht is niet geaccepteerd door de moderator.
Opmerkingen van de moderator staan hieronder.
</text/mod-request#E/>
De bijgesloten e-mail is aangeboden aan de <#L#>@<#H#>
mailing list. Als u dit bericht wilt accepteren voor distributie,
stuur dan een e-mail naar:

!A

Dit gebeurt gewoonlijk als u een "reply" geeft op dit bericht.
Het juiste adres moet beginnen met "<#L#>-accept". Als dit niet zou werken,
kunt u het adres kopieeren en plakken naar de "To:" regel van een nieuw
bericht.

U kunt ook hier klikken:
	mailto:<#A#>
</#E/>

Om dit bericht te weigeren stuurt u een e-mail naar:

!R

Gewoonlijk kunt u "reply-to-all" aanklikken en dan alle adressen verwijderen,
behalve het adres wat begint met
"<#L#>-reject".
</#xE/>

Om te weigeren, kunt u ook hier klikken:
	mailto:<#R#>
</#E/>

U hoeft het hele bericht niet bij te sluiten in uw antwoord.
Als u opmerkingen naar de afzender van het geweigerde bericht wilt sturen,
kunt u dit hieronder tussen de twee regels met drie % plaatsen.

%%% Begin commentaar
%%% Einde commentaar

Bedankt voor uw hulp!

--- Hieronder staat het bijgesloten bericht.

</text/mod-sub#E/>
--- Ik heb u in- of uitgeschreven op verzoek van een moderator
van de <#l#>@<#H#> mailing list.

Indien dit niet is wat u wenst, stuur dan zo spoedig mogelijk
uw opmerkingen naar de eigenaar (<#l#>-owner@<#H#>).

</text/mod-timeout#E/>
Sorry, de moderators van de <#L#> list hebben niet
gereageerd op uw bericht. Daarom stuur ik het bericht naar u terug.
U kunt het bericht opnieuw indienen of rechtstreeks contact opnemen
met een moderator.

--- Hieronder staat het bericht wat u aanbood.

</text/mod-sub-confirm#E/>
Ik vraag uw toestemming om het adres

!A

toe te voegen aan de <#l#> mailinglist.

Om te bevestigen kunt u een reply sturen naar dit adres:

!R

Dit gebeurt gewoonlijk als u "reply" klikt. Als dat niet zou werken
kunt u bovenstaand adres kopieeren en plakken in de "To:" regel van
een nieuw bericht
</#xE/>

U kunt ook hier klikken:
	mailto:<#R#>
</#E/>

Als dit niet is wat u wenst, kunt u dit bericht negeren.

Dank u voor uw hulp!

</text/mod-unsub-confirm#E/>
Ik heb een verzoek ontvangen om

!A

van de <#l#> mailing list te verwijderen.
Als u hiermee instemt, stuur dan een reply naar dit adres:

!R

Dit gebeurt gewoonlijk als u op reply klikt. Als dat niet zou werken
kunt u bovenstaand adres kopieeren en plakken in de "To:" regel van
een nieuw bericht.
</#xE/>

U kunt ook hier klikken:
	mailto:<#R#>
</#E/>

Als dit niet is wat u wenst, kunt u dit bericht negeren.

Dank u voor uw hulp!

</text/sub-bad#E/>
Sorry, het bevestigingsnummer lijkt ongeldig.

Dit gebeurt meestal doordat ik het antwoord te laat heb gekregen. Ik moet
binnen 10 dagen antwoord krijgen op ieder verzoek.
Let ook op dat het bevestigingsnummer goed is overgenomen in het antwoord
wat ik krijg.

Ik heb een nieuw bevestigingsnummer voor u. Als u wilt dat

!A

wordt toegevoegd aan de <#l#> mailinglist, stuur dan
een reply naar dit adres:

!R
</#xE/>

Of klik hier:
	mailto:<#R#>
</#E/>

Controleer het reply-adres zorgvuldig of alles goed is overgenomen voor
u uw bericht verstuurt.

Excuus voor het ongemak.

	<#L#>-Owner <<#l#>-owner@<#H#>>

</text/sub-confirm#E/>
Om te bevestigen dat u

!A

toe wilt voegen aan de <#l#> mailing list, stuur dan
een reply naar dit adres:

!R

Dit gebeurt gewoonlijk als u "reply" klikt.
Als dit niet zou werken, kunt u bovenstaand adres kopieeren en
plakken in de "To:" regel van een new bericht.
</#xE/>

U kunt ook hier klikken:
	mailto:<#R#>
</#E/>

Deze bevestiging dient twee doelen. Ten eerste weet ik dat ik u e-mail kan
sturen. Ten tweede beschermt het u voor het geval dat iemand anders u
tegen uw wil inschrijft op deze mailing list.

</#qE/>
Sommige emailprogramma's hebben moeilijkheden met lange adressen. Als u
niet kunt antwoorden op dit verzoek, stuur dan een bericht naar:
<<#L#>-request@<#H#>> en zet bovenstaand
adres (helemaal) in de "Subject:" regel.

</#sE/>
Deze list is "moderated". Zodra u deze bevestiging hebt gestuurd, stuur
ik de moderators een verzoek om u daadwerkelijk toe te voegen.
Ik zal u meteen in kennis stellen als dat is gebeurd.

</text/sub-nop#E/>
Ik heb uw verzoek niet uit kunnen voeren: Het adres

!A

stond al op de <#l#> mailing list toen ik uw
verzoek kreeg en blijft daarom ingeschreven.

</text/sub-ok#E/>
Bevestiging: Ik heb het adres

!A

toegevoegd aan de <#l#> mailing list.

Welkom bij <#l#>@<#H#>!

Bewaart u dit bericht zorgvuldig, zodat u weet onder welk adres u
bent ingeschreven op deze mailing list.

Om u weer uit te schrijven stuurt u een e-mail naar:

    <<#l#>-unsubscribe-<#t#>@<#H#>>

</text/top/>
Hallo! Ik ben het ezmlm programma. Ik verzorg de
<#l#>@<#H#> mailing list.

Ik werk voor mijn eigenaar, die bereikt kan worden op
<#l#>-owner@<#H#>.

</text/unsub-bad#E/>
Sorry, het bevestigingsnummer lijkt ongeldig.

Dit gebeurt meestal doordat ik het antwoord te laat heb gekregen. Ik moet
binnen 10 dagen antwoord krijgen op ieder verzoek.
Let ook op dat het bevestigingsnummer goed is overgenomen in het antwoord
wat ik krijg.

Ik heb een nieuw bevestigingsnummer voor u. Als u wilt dat

!A

wordt verwijderd van de <#l#> mailinglist, stuur dan
een reply naar dit adres:

!R
</#xE/>

U kunt ook hier klikken:
	mailto:<#R#>
</#E/>

Ik heb niet gecontroleerd of uw adres op dit moment op de mailing list
staat. Om te zien onder welk adres u op dit moment bent ingeschreven,
kijkt u naar een bericht dat u van de mailing list hebt ontvangen.
Ieder bericht bevat uw email adres in het "return path";
Bijvoorbeeld,  mary@host.domain ontvangt berichten met het "return-path":
<<#l#>-return-<number>-mary=host.domain@<#H#>.

</#qE/>
Sommige emailprogramma's bevatten fouten en kunnen niet overweg met lange
adressen. Als u geen reply kunt sturen op dit verzoek, kunt u een bericht
sturen naar:
<<#L#>-request@<#H#>>
en plak het volledige adres wat hierboven staat in de "Subject:" regel.

</text/unsub-nop#E/>
Sorry, ik kan uw verzoek niet uitvoeren.
Het adres

!A

stond niet op de <#l#> mailing list toen ik uw verzoek
kreeg, en is geen lid van deze lijst.

Als u zich hebt uitgeschreven, maar nog steeds mail ontvangt, dan
bent u onder een ander adres ingeschreven dan wat u nu gebruikt.
Kijk in de HEADER van uw email en zoek naar een regel met:

'Return-Path: <<#l#>-return-1234-joe=host.domain@<#H#>>'

Hier ziet u dat het ingeschreven adres ``joe@host.domain'' is.
Om dit adres uit te schrijven zou u een bericht sturen naar:
'<#l#>-unsubscribe-joe=host.dom@<#H#>'.

Stuur een email naar dat adres, natuurlijk aangepast met uw adres.

Als het bericht een header bevat met ``List-Unsubscribe:'', dan kunt u
een e-mail sturen naar het adres in die header; Het uitschrijf commando
zit dan al in het adres verwerkt.

In veel emailprogramma's moet u de headers zichtbaar maken om het
"return-path" te kunnen zien:

Onder Eudora 4.0, klikt u op de "Blah blah ..." knop.
In PMMail, klikt u "Window->Show entire message/header".
Met Outlook Express kunt u Ctrl-F3 toetsen.
Onder KMail gebruikt u "View->Headers->All Headers".

Als dit niet werkt, kan ik u helaas niet helpen.
Stuurt u in dat geval een bericht van de mailing list door (forward) naar
mijn eigenaar:

	<<#l#>-owner@<#H#>>

en voeg er een notitie aan toe met uw wensen.
Mijn eigenaar is minder snel dan ik, dus heb dan even geduld s.v.p.

</text/unsub-ok#E/>
Bevestiging: Ik heb het adres

!A

verwijderd van de <#l#> mailing list. Dit adres is niet langer
ingeschreven.

</text/edit-do#nE/>
Bewerk onderstaande tekst en stuur het naar dit adres:

!R

U kunt ook de "reply" knop gebruiken in uw emailprogramma.

Ik kan de aanhalingstekens verwijderen die uw mailprogramma aan de tekst
toevoegt, zolang u de markeringsregels niet wijzigt.

De markeringsregels beginnen met '%%%' en moeten onveranderd blijven.
(extra tekens aan het begin van de regel worden wel geaccepteerd)


</text/edit-list#nE/>
Het <#L#>-edit.file commando kan gebruikt worden om een administrator
de teksten te laten wijzigen van de mailing list manager van de
<#L#>@<#H#> mailing list.

Hier volgt een overzicht van de teksten en wanneer ze gebruikt worden.
Om een tekst te wijzigen stuurt u een email naar
<#L#>-edit.file
waarbij u 'file' verandert in de naam van de tekst die u wilt
wijzigen. Instructies worden met de file meegestuurd.

File                Gebruik

bottom              Staat onder ieder bericht. Bevat algemene commando's
digest              Administratief gedeelte van digests
faq                 Veel gestelde vragen (FAQ's) van deze list
get_bad             In plaats van berichten die niet in het archief staan
help                Algemene help (tussen 'top' en 'bottom').
info                List info. De eerste regel zou betekenis moeten hebben
mod_help            help voor list moderators
mod_reject          bericht aan afzender van geweigerde mail
mod_request         bericht aan moderators met oorspronkelijk bericht
mod_sub             bericht aan gebruiker als deze toegelaten is
mod_sub_confirm     bericht aan moderator met verzoek om toelating
mod_timeout         aan afzender als gemodereerd bericht verlopen is
mod_unsub_confirm   aan remote admin voor bevestiging van uitschrijven
sub_bad             aan gebruiker als inschrijving is afgewezen
sub_confirm         aan gebruiker als inschrijving is goedgekeurd
sub_nop             aan gebruiker na hernieuwde inschrijving
sub_ok              aan gebruiker na succesvolle inschrijving
top                 staat boven ieder bericht
</#tnE/>
trailer             staat onder aan elk uitgezonden bericht
</#nE/>
unsub_bad           aan gebruiker als uitschrijving mislukt is
unsub_confirm       aan gebruiker om uitschrijving te bevestigen
unsub_nop           aan niet-gebruiker na uitschrijf verzoek
unsub_ok            aan ex-gebruiker na succesvolle uitschrijving

</text/edit-done#nE/>
Het tekstbestand is met succes gewijzigd.
</text/info#E/>
Deze list bevat nog geen infobestand.
</text/faq#E/>
Veel gestelde vragen (FAQ) van de <#l#>@<#H#> list.

Dit overzicht is nog leeg.

