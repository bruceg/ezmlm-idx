0.40 - This version identifier must be on line 1 and start in pos 1.
#
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
# clean slate.
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
|<#B#>/ezmlm-issubn -n '<#D#>/deny' || { echo "Jag tillåter inte dina meddelanden. Kontakta <#L#>-owner@<#H#> ifall du har några frågor angående det (#5.7.2)"; exit 100 ; }
# switch -u=> restrict to subs of list & digest. If not m
# do it with ezmlm-issubn, if 'm' do it with ezmlm-gate
</#uM/>
|<#B#>/ezmlm-issubn '<#D#>' '<#D#>/digest' '<#D#>/allow' '<#D#>/mod' || { echo "Tyvärr, endast prenumeranter får posta. Ifall du är en prenumerant, orward this message to <#L#>-owner@<#H#> to get your new address included (#5.7.2)"; exit 100 ; }
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
kontakta <#L#>-help@<#H#>; körs med ezmlm
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
För att avsluta prenumerationen skicka e-mail till:
<#L#>-unsubscribe@<#H#>
För ytterligare kommandon, skicka e-mail till:
<#L#>-help@<#H#>
</text/bottom#E/>

--- Administrativa kommandon för <#l#> listan ---

Administrativa förfrågningar kan hanteras automatiskt. Skicka
dem inte till listans adress! Skicka istället ditt meddelande
till rätt "kommando adress":

För hjälp och en beskrivning över tillgängliga kommandon,
skicka ett brev till:
   <<#L#>-help@<#H#>>

För att prenumerera på listan, skicka ett brev till:
   <<#L#>-subscribe@<#H#>>

För att avsluta din prenumeration, skicka ett meddelande till
adressen som står i "List-Unsubscribe" raden i brevhuvudet
från ett brev som kom från listan. Ifall du inte bytt adress
sen du påbörjade din prenumeration, skicka ett brev till:
   <<#L#>-unsubscribe@<#H#>>

</#dE/>
eller för "digest" versionen:
   <<#L#>-unsubscribe@<#H#>>

</#E/>
För nya/avslutade prenumerationer, skickar jag ett bekräftelse
brev till adressen. När du får brevet, svara bara på det för
att genomföra prenumerationsförändringen.

Ifall du behöver komma i kontakt med en människa angående
listan, skicka ett brev till:

    <<#L#>-owner@<#H#>>

Var vänlig och VIDARESKICKA (forward) ett meddelande från listan
inklusive HELA brevhuvudet så vi lättare kan hjälpa dig.

--- Nedan finner du en kopia på förfrågan jag fick.

</text/bounce-bottom#E/>

--- Nedan finner du en kopia på det "studsade" meddelandet jag fick.

</text/bounce-num#E/>

Jag har skapat en lista på de meddelanden från <#L#> listan som
har "studsat" på väg till dig.

</#aE/>
Kopior av dessa meddelanden kan du finna i arkivet.

</#aE/>
För att hämta meddelande 123-145 (max 100 per förfrågan), skicka
ett brev till:
   <<#L#>-get.123_145@<#H#>>

För att få en lista på titlar och författare för de senaste 100
meddelandena, skicka ett brev till:
   <<#L#>-index@<#H#>>

</#E/>
Detta är meddelande nummren:

</text/dig-bounce-num#E/>

Jag har skapat en lista på "digest" meddelanden från <#L#>-digest
listan, som har "studsat" till din adress. För varje "digest" brev
som du missat, har jag skrivit upp första meddelandenummret i det
brevet.

</#aE/>
"Digest" meddelanden arkiveras inte, men du kanske kan finna dem
i akrivet för huvudlistan.

För att ta emot brev 123-145 (max 100 per förfrågan),
skicka ett brev till:
   <<#L#>-get.123_145@<#H#>>

För en lista över författare och titlar på de senaste 100
meddelandena, skicka ett brev till:
   <<#L#>-index@<#H#>>

</#E/>
Här är "digest" meddelande nummren:

</text/bounce-probe#E/>

Meddelanden till dig från <#l#> listan, verkar ha "studsat".
Jag skickade ett varningsbrev till dig om det, men det "studsade".
Nedan följer en kopia på det meddelandet.

Detta testbrev kontrollerar om din adress är nåbar. Ifall detta
brev också studsar, plockas din adress bort från
<#l#>@<#H#> listan, utan ytterligare varningar.

Du kan prenumerera på nytt genom att skicka ett brev
till denna adressen:
   <<#l#>-subscribe@<#H#>>

</text/bounce-warn#E/>

Meddelanden till dig från <#l#> listan har "studsat".
Jag bifogar en kopia på det första brevet till dig där
det inträffade.

Ifall detta meddelande också "studsar", kommer ett testbrev skickas
till dig. Ifall det brevet också studsar, plockas din adress bort
från <#l#> listan utan ytterligare varning.

</text/digest#dE/>
För prenumeration på "digest" versionen, skicka ett brev till:
	<#L#>-digest-subscribe@<#H#>

För att avsluta prenumerationen på "digest" versionen,
skicka ett brev till:
	<#L#>-digest-unsubscribe@<#H#>

För att skicka ett brev till listan, skicka brevet till:
	<#L#>@<#H#>

</text/get-bad#E/>
Tyvärr, det meddelandet finns inte i arkivet.

</text/help#E/>
Detta är ett almänt hjälp meddelande. Brevet som kom var inte
skickat till någon av kommando adresserna.

Detta är en lista på de kommando adresser som stöds:

Skicka brev till något av följande adresser för information
och "FAQn" för listan:
   <<#L#>-info@<#H#>>
   <<#L#>-faq@<#H#>>

</#dE/>
Liknande adresser finns för "digest" versionen av listan:
   <<#L#>-digest-subscribe@<#H#>>
   <<#L#>-digest-unsubscribe@<#H#>>

# ezmlm-make -i needed to add ezmlm-get line. If not, we can't do
# multi-get!
</#aE/>
För att få meddelande 123 till 145 (max 100 per förfrågan),
skicka ett brev till:
   <<#L#>-get.123_145@<#H#>>

För att få ett index med författare och titel för meddelande
123-456, skicka ett brev till:
   <<#L#>-index.123_456@<#H#>>

För att få alla meddelanden med samma titel som meddelande 12345,
skicka ett brev till:
   <<#L#>-thread.12345@<#H#>>

</#E/>
Meddelandena behöver inte innehålla något särskilt, det är
bara adressen som är viktig.

Du kan starta en prenumeration till en alternativ adress,
t ex "john@host.domain", addera bara ett bindesträck
och din adress (med '=', istället för '@') efter kommando
ordet. Dvs:
<<#L#>-subscribe-john=host.domain@<#H#>>

För att avsluta prenumerationen till denna adressen,
skicka ett brev till:
<<#L#>-unsubscribe-john=host.domain@<#H#>>

</text/mod-help#E/>
Tack för att du vill moderera <#L#>@<#H#> listan.

Kommandona är lite anorlunda mot andra listor,
men de är lätta att lära och använda.

Här är lite instruktioner angående de saker du kan behöva
göra som listägare/moderator.

Allmäna kommandon följer efter detta meddelande.

Fjärr prenumeration.
--------------------
Som moderator kan du prenumerera och avprenumerera vilken adress
som helst på listan. För att prenumerera "john@host.domain",
skriv bara ett bindesträck efter "kommando ordet", därefter
adressen med ett '=' tecken istället för '@'. I detta fallet skulle
du skickat ett brev till:
   <<#L#>-subscribe-john=host.domain@<#H#>>

Du kan på samma sätt ta bort en adress med ett meddelande till:
   <<#L#>-unsubscribe-john=host.domain@<#H#>>

</#dE/>
För "digest" versionen av listan:
   <<#L#>-digest-subscribe-john=host.domain@<#H#>>
   <<#L#>-digest-unsubscribe-john=host.domain@<#H#>>

</#E/>
Det är allt. Titel och innehåll spelar ingen roll!

</#rE/>
Ett bekräftelse brev kommer skickas för att vara säker
på att det verkligen var du som skickade brevet.
Svara bara på det brevet och det hela är klart.
</#RE/>
Jag kommer skicka ett bekräftelsebrev till prenumerantens adress,
i detta fallet <john@host.domain>. Allt prenumeranten behöver
göra är att svara på brevet.
</#E/>

Bekräftelserna är nödvändiga för att göra det svårt för
en tredje part till att lägga till/ta bort adresser till
listan.

Jag kommer underrätta prenumeranten när dennes status
har ändrats.

Prenumeration
--------------

Alla kan prenumerera/sluta prenumerera på listan genom
att skicka ett brev till:

<#L#>-subscribe@<#H#>
<#L#>-unsubscribe@<#H#>

</#dE/>
För "digest" versionen av listan:

<#L#>-digest-subscribe@<#H#>
<#L#>-digest-unsubscribe@<#H#>

</#E/>
Prenumeranten kommer få ett bekräftelse brev för
att vara säker på att personen har den adressen.
När det är klart blir personen borttagen ur listan.

</#sE/>
Eftersom denna listan är sluten, kommer jag skicka en andra
förfrågan till moderatorerna. Eftersom prenumeranten redan har
bekräftat att den vill vara med på listan, kan du som
moderator vara säker på att det är rätt adress. Ifall du vill
ha med personen på listan, svara på bekräftelse (CONFIRM)
meddelandet. Ifall du inte vill ha med personen, radera bara
meddelandet istället (eller kontakta personen för ytterligare
information).
</#SE/>
Prenumeration fungerar på samma sätt.
</#E/>

Användaren kan också:

   <<#L#>-subscribe-mary=host.domain@<#H#>>
   <<#L#>-unsubscribe-mary=host.domain@<#H#>>

för att få brev skickad till "mary@host.domain". Bara om hon kan
ta emot brev på den adressen, får hon bekräftelse meddelandet
och kan svara på det.

Din adress och identitet kommer att vara hemlig för prenumeranten
om du inte skickar brev direkt till denne.

</#rlE/>
För att få en lista på prenumeranter på <#L#>@<#H#>,
skicka ett brev till:
   <<#L#>-list@<#H#>>

För att få en "transaktionslog" för <#L#>@<#H#>,
skicka ett brev till:
   <<#L#>-log@<#H#>>

</#rldE/>
För "digest" prenumeranter:
   <<#L#>-digest-list@<#H#>>
och:
   <<#L#>-digest-log@<#H#>>

</#rnE/>
Du kan ändra textfilerna, som listan använder, på distans. För att
få en lista på filerna och instruktioner om hur du ändrar dem,
skicka ett e-mail till:
   <<#L#>-edit@<#H#>>

</#mE/>
Modererade utskick
------------------
När utskick är modererade, kommer ett brev att skickas till dig
med en kopia på utskick och instruktioner som berättar hur
utskicket skall godkännas för att komma med på listan. Det
brevet kommer att ha "MODERATE for ..." som titel.

För att acceptera ett utskick, skicka bara ett svar till 'Reply-To:'
adressen (sker vanligtvis med "svara" knappen). Du behöver inte
skicka med brevet du fick skickat till dig, det är bara adressen
som är viktig.

Ifall du vill avvisa utskicket, skicka ett brev till avsändar-
adressen ("From:" fältet), där rätt avvisningsadress är inskrivning.
"Svara alla" brukar använda den adressen. Om du vill skriva ett
meddelande till författaren, skriv den mellan två rader som börjar
med tre '%' tecken. Detta kommer att ske anonymt och bara skickas
till författaren.

Utskicket kommer att behandlas beroende på vilket svar som kommer
in först. Om en moderator redan har avvisat ett brev som du godkänner
så kommer brevet ändå att vara avvisat och vice versa.

Ifall ingen moderator svarar inom en viss tid (vanligtvis 5 dagar),
kommer brevet att returneras till författaren med en förklaring
om vad som hände.
</#E/>

Semestrar
---------
Ifall du temporärt har en annan adress, vidareskicka alla brev som
har korrekt "Mailing-List:" fält i brevhuvudet (eller alla brev som
har titeln "MODERATE for <#L#>@<#H#>"
eller "CONFIRM subscribe to <#L#>@<#H#>")
till den nya adressen. Du kan därefter moderera listan från den
adressen. Alternativt kan du vidareskicka brevet till någon annan
som modererar listan åt dig. Fråga listägaren först om det är OK.

Ifall du vill att allt skall godkännas automatiskt medan du är
borta, ställ iordning ditt e-mail system så den gör ett autosvar
på brev med ovan nämnda titlar.

</#rE/>
Ifall du försöker administrera listan från en adress som inte är din
egen, prenumeranten, inte du, kommer frågas efter en bekräftelse.
Därefter kommer en bekräftelseförfrågan skickas till moderatorerna.
Detta görs eftersom det är omöjligt att veta ifall det var du som
skickade originalfrågan.

Observera att originalförfrågan, inklusive din adress, skickas till
prenumeranten i detta fallet.
</#E/>

Lycka till!

PS. Kontakta listägaren (<#L#>-owner@<#H#>) ifall du
har några frågor eller stöter på några problem.

</text/mod-reject#E/>
Tyvärr, meddelandet (bifogat) accepterades inte av moderatorn.
Ifall moderatorn har bifogat några kommentarer, står de här nedan.
</text/mod-request#E/>
Det bifogade meddelandet skickades till <#L#>@<#H#> listan.
Ifall du vill godkänna den för vidare distribution skicka e-mail till:

!A

Vanligtvis händer detta automatiskt om du trycker på "svara" (reply)
knappen. Du kan kontrollera adressen att den börjar med:
"<#L#>-accept". Ifall det inte fungerar, kopiera adressen och
klistra in den i "Till" ("To:") fältet i ett nytt brev.
</#xE/>

Alternativt, tryck här:
	<mailto:<#A#>>
</#E/>

Föra att skicka tillbaka brevet till avsändaren, skicka ett
meddelande till:

!R

Vanligtvis är det enklare att trycka på "svara alla" ("reply-to-all")
knappen och ta bort alla adresser som inte börjar med:
"<#L#>-reject".
</#xE/>

Alternativt, tryck här:
	<mailto:<#R#>>
</#E/>

Du behöver inte kopiera brevet i ditt svar. Ifall du vill skicka
med en kommentar till författaren till ett brev du inte accepterat,
inkludera kommentaren, i svarsbrevet, mellan två rader som börjar
med tre procenttecken ('%').

%%% Start kommenter
%%% Slut kommentar.

Tack för din hjälp!

--- Nedan finner du utskicket.

</text/mod-sub#E/>
--- Du har blivit (av-)prenumererad av en moderator för
<#l#>@<#H#> listan.

Ifall du inte tycker om det, skicka ett klagomål, eller annan
kommentar, till listägaren (<#l#>-owner@<#H#>) så snart som
möjligt.

</text/mod-timeout#E/>
Tyvärr har <#L#> listans moderatorer inte
hanterat din postning, därför skickas den nu tillbaka till dig.
Ifall detta är fel, skicka om ditt meddelande till listan
eller kontakta listägaren (<#L#>-owner@<#H#>).

--- Bifogat är brevet du skickade.

</text/mod-sub-confirm#E/>
Vill du lägga till

!A

till <#l#> listan? Antingen kom detta brevet som svar på
att du vill lägga till prenumeranten till listan eller
så har prenumeranten redan bekräftat sin prenumeration.

För att bekräfta, skicka ett tomt brev till denna adress:

!R

Vanligtvis görs det genom "svara" ("reply") knappen.
Ifall det inte fungerar, kopiera adressen och klistra in den i
"To:" fältet i ett nytt meddelande.
</#xE/>

eller tryck här:
	<mailto:<#R#>>
</#E/>

Ifall du inte godkänner detta, ignorera detta meddelande.

Tack för din hjälp!

</text/mod-unsub-confirm#E/>
Någon önskar ta bort:

!A

från <#l#> listan. Ifall du håller med, skicka ett brev
till denna adress:

!R

Enklast gör du det genom att trycka på "svara" ("reply") knappen.
Ifall det inte fungerar, kopiera adressen och klistra in den i
"Till" ("To:") fältet i det nya meddelandet.
</#xE/>

eller tryck här:
	<mailto:<#R#>>
</#E/>

Ifall du inte håller med, ignorera detta brev.

Tack för din hjälp!

</text/sub-bad#E/>
Oops, det bekräftelsenummret verkar vara felaktigt.

Den vanligaste orsaken till felaktiga bekräftelsenummer är
att de blivit för gamla. De gäller i max 10 dagar. Var också
säker på att du använde hela bekräftelsenummret i ditt svar,
vissa program kan i vissa fel ta bort slutet på adresser när
de är långa.

Ett nytt bekräftelsenummer har skapats, för att bekräfta att
du vill ha med

!A

på <#l#> listan, skicka ett brev till denna adress:

!R
</#xE/>

eller tryck här:
	<mailto:<#R#>>
</#E/>

Var noga med att svarsadresser är riktig när du bekräftar
prenumerationen.

Ursäkta detta extra besvär.

	<#L#>-Owner <<#l#>-owner@<#H#>>

</text/sub-confirm#E/>
För att bekräfta att du vill ha

!A

adderad till <#l#> listan, skicka ett brev till denna adress:

!R

Enklast görs det genom att trycka på "svara" ("reply") knappen.
Ifall det inte fungerar, kopiera adressen och klistra in den i
"Till" ("To:") fältet i ett nytt brev.
</#xE/>

eller tryck här:
	<mailto:<#R#>>
</#E/>

Denna bekräftelse tjänar två syften. Dels säkerställer den att det går
att skicka brev till dig och dels skyddar den dig mot att andra försöker
prenumerera någon mot dess vilja.

</#qE/>
Det är fel på vissa e-mail program vilket gör att de inte kan hantera
långa adresser. Ifall du inte kan svara på denna förfrågan, skicka
istället ett meddelande till <<#L#>-request@<#H#>>
och skriva hela ovan nämnda adress i titel ("Subject:") raden.

</#sE/>
Denna lista är modererad. Så fort du har svarat på denna bekräftelse
kommer din förfrågan att skickas till moderatorerna för listan.
Du kommer att underrättas när din prenumeration är aktiverad.

</text/sub-nop#E/>
Jag kunde inte utföra din förfrågan.

!A

prenumererar redan på <#l#> listan när jag fick din förfrågan.
Adressen kommer vara kvar på listan.

</text/sub-ok#E/>
Uppmärksamma: Adressen

!A

har adderats till <#l#> listan.

Välkommen till <#l#>@<#H#>!

Var vänlig och spara detta meddelande så du minns vilken adress
som prenumererar på listan, ifall du senare vill avsluta din
prenumeration.

För att avsluta prenumerationen, skicka ett brev till:

    <<#l#>-unsubscribe-<#t#>@<#H#>>

</text/top/>
Detta är ett meddelande från ezmlm programmet som har hand om
<#l#>@<#H#> listan.

</#x/>
Ägaren till listan kan nås på:
<#l#>-owner@<#H#>.

</text/unsub-bad#E/>
Oops, det bekräftelsenummret verkar vara felaktigt.

Den vanligaste orsaken till felaktiga bekräftelsenummer är
att de blivit för gamla. De gäller i max 10 dagar. Var också
säker på att du använde hela bekräftelsenummret i ditt svar,
vissa program kan i vissa fel ta bort slutet på adresser när
de är långa.

Ett nytt bekräftelsenummer har skapats, för att bekräfta att
du vill ta bort

!A

från <#l#> listan, skicka ett brev till denna adress:

!R
</#xE/>

eller klicka här:
	<mailto:<#R#>>
</#E/>

Var vänligt att kontrollera svarsadressen noggrant så att den är
riktig innan du svarar på detta brev.

Ursäkta allt besvär.

	<#l#>-Owner <<#l#>-owner@<#H#>>

</text/unsub-confirm#E/>
För att bekräfta att du vill ta bort

!A

från <#l#> listan, skicka ett brev till denna adress:

!R

Vanligtvis gör man det med "Svara" ("Reply") knappen.
Ifall det inte fungerar, kopiera adressen nedan och klistra
in den i "Till" ("To:") fältet i ett nytt brev.
</#xE/>

eller tryck här:
	<mailto:<#R#>>
</#E/>

För att se vilken adress din prenumeration går till, undersök ett
meddelande från listan. Varje meddelande har din adress dold i
dess "return path", t ex mary@xdd.ff.com har får meddelanden
med "return-path" satt till:
<<#l#>-return-<nummer>-mary=xdd.ff.com@<#H#>>.

</#qE/>
Vissa email program är felaktiga och kan inte hantera långa adresser.
Ifall du inte kan svara på detta meddelande, skicka istället ett
meddelande till <<#L#>-request@<#H#>> och skriv hela ovan nämnda
adress i titel ("Subject:") raden.

</text/unsub-nop#E/>
Tyvärr kan inte din förfrågan utföras eftersom adressen:

!A

var inte med på <#l#> listan.

Ifall du har avslutat din prenumeration, men fortfarande får brev,
är du prenumererad under en annan adress än den du för närvarande
använder. Titta i brevhuvudet (header) efter:

'Return-Path: <<#l#>-return-1234-user=host.dom@<#H#>>'

Det visar din prenumerationsadress som "user@host.dom".
För att avsluta din prenumeration med den adressen, skicka
ett brev till:
<#l#>-unsubscribe-user=host.dom@<#H#>

Glöm inte att anpassa user=host.dom till din egen adress.

Ifall meddelandet har en "List-Unsubscribe:" fält i brevhuvudet,
kan du skicka ett meddelande till adressen i det fältet.
Det är redan anpassat för din adress.

I vissa email program måste du göra vissa inställningar för att
se "return path" fältet i brevhuvudet:

I Eudora 4.0, klicka på "Blah blah ..." knappen.
I PMMail, klicka på "Window->Show entire message/header". 

Ifall det inte fungerar kan vi tyvärr inte göra mer.
Vidaresänd ett brev från listan, tillsammans med ett meddelande
om vad du vill ha gjort och en lista som du tror att du kan ha
prenumererat under till listägaren:

    <<#l#>-owner@<#H#>>

som kan ta hand om det. Det kan dock dröja en liten stund innan du får
ett svar.

</text/unsub-ok#E/>
Observera: Jag har tagit bort adressen

!A

från <#l#> listan. Den adressen är inte längre en prenumerant.

</text/edit-do#nE/>
Var vänlig och editera följande textfil och skicka den till
denna address:

!R

Ditt mailprogram borde ha en svarsfunktiuon som använder
denna address automatiskt. Ifall det inte fungerar, kan du
kopiera addressen och klistra in den i "To:"/"Till:" fältet
på ett nytt medelande.
</#xE/>

eller klicka här:
        mailto:<#R#>
</#E/>

Jag kan ta bort citeringsmarkeringar (t ex "> ") som din
mailprogramvara lägger till texten så länge som du inte
ändrar start och slutraderna.

Start och slutraderna är rader som börjar med %%%. De får inte
ändras. Ifall ditt mailprogram lägger in tecken före dem så skall
de stå kvar.


</text/edit-list#En/>
<#L#>-edit.fil kommandot kan användas av en fjärradministratör
för att editera de textfiler som skapar de svaren jag skickar för
<#L#>@<#H#> listan.

Här följer en lista på de filer som kan ändras samt en
kort beskrivning om hur dess innehåll används. För att
ändra en fil, skicka ett brev till <#L#>-edit.filnamn där
du byter ut filnamn mot filens namn. Editeringsinstruktioner
skickas till dig ihop med textfilen.

Fil                 Användninsområde.

bottom              slutet på alla svar. Generell kommando information.
digest              'administrationsbiten' av en 'digest'.
faq                 Vanligt förekommande frågor på denna lista.
get_bad             i stället för medelanden som inte hittas i arkivet.
help                generell hjälp (mellan top och bottom).
info                list info. Första raden skall kunna visas separat.
mod_help            specifik hjälp för listmoderatorer.
mod_reject          sänds till avsändaren av avvisade medelanden.
mod_request         till medelandemoderatorerna tillsammans med medelandet.
mod_sub             till prenumeranter efter att en moderator bekräftat prenumerationen.
mod_sub_confirm     till prenumerationsmoderatorn för att bekräfta prenumerationer.
mod_timeout         till sändaren av ett medelande som ingen accepterat/avvisat.
mod_unsub_confirm   till en administratör för att bekräfta avprenumerationer.
sub_bad             till prenumeranten ifall bekräftelsen var felaktig.
sub_confirm         till prenumeranten för att bekräfta prenumerationer.
sub_nop             till prenumeranten efter en dubbel prenumeration.
sub_ok              till prenumeranten efter en lyckad prenumeration.
</#tnE/>
trailer             adderas till alla utskick innan de kommer till listan.
</#nE/>
top                 starten på alla svar. Generell kommando information.
unsub_bad           till prenumeranten ifall avprenumerationen misslyckades.
unsub_confirm       till prenumeranten för att bekräfta avprenumeration.
unsub_nop           till icke-prenumerant efter avprenumeration.
unsub_ok            till tidigare prenumeration efter avslutad prenumeration.

</text/edit-done#nE/>
Textfilen uppdaterades korrekt.
</text/info#E/>
Ingen information har antecknats om listan.
</text/faq#E/>
FAQ - vanligt förekommande frågor på <#l#>@<#H#> listan.

Inga har nedtecknats ännu.

