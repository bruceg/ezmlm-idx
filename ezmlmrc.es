0.40 - This version identifier must be on line 1 and start in pos 1.
#
#$Id$
#
# ezmlmrc- Traducción de: Vicent Mas, Francesc Alted, Sonia Lorente, Cyndy DePoy
# #######   Servicom2000
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
|<#B#>/ezmlm-issubn -n '<#D#>/deny' || { echo "Sorry, I've been told to reject your posts. Contact <#L#>-owner@<#H#> if you have questions about this (#5.7.2)"; exit 100 ; }
# switch -u=> restrict to subs of list & digest. If not m
# do it with ezmlm-issubn, if 'm' do it with ezmlm-gate
</#uM/>
|<#B#>/ezmlm-issubn '<#D#>' '<#D#>/digest' '<#D#>/allow' '<#D#>/mod' || { echo "Sorry, only subscribers may post. If you are a subscriber, please forward this message to <#L#>-owner@<#H#> to get your new address included (#5.7.2)"; exit 100 ; }
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
contact <#L#>-help@<#H#>; run by ezmlm
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
Para dar de baja la suscripción, mande un mensaje a:

   <#L#>-unsubscribe@<#H#>

Para obtener el resto de direcciones-comando, mande un mensaje a:

   <#L#>-help@<#H#>

</text/bottom#E/>

--- Comandos administrativos para la lista <#l#> ---

Puedo gestionar automáticamente peticiones administrativas. Por
favor, no envíe este tipo de peticiones a la lista. Envíelas a la
dirección-comando adecuada:

Para obtener ayuda y una descripción de los comandos disponibles,
mande un mensaje a:

   <<#L#>-help@<#H#>>

Para suscribirse a la lista, mande un mensaje a:

   <<#L#>-subscribe@<#H#>>

Para eliminar su dirección de la lista, simplemente mande un
mensaje a la dirección que hay en la cabecera
``List-Unsubscribe'' de cualquier mensaje de la lista. Si usted
no ha cambiado su dirección desde que se suscribió, también puede
enviar un mensaje a:

   <<#L#>-unsubscribe@<#H#>>

</#dE/>
o, para los resúmenes, a:

   <<#L#>-unsubscribe@<#H#>>

</#E/>

Para añadir o eliminar direcciones, le enviaré un mensaje de
confirmación a esa dirección. Cuando lo reciba, pulse el botón
'Responder' para completar la transacción.

Si necesita contactar con el propietario de la lista, por favor,
mande un mensaje a:

    <<#L#>-owner@<#H#>>

Por favor, incluya una lista de mensajes REENVIADOS con TODAS LAS
CABECERAS intactas para que sea más fácil ayudarle.

--- Le adjunto una copia de la petición que he recibido.

</text/bounce-bottom#E/>

--- Le adjunto una copia del mensaje devuelto que he recibido.

</text/bounce-num#E/>

He guardado una lista de todos los mensajes de la lista de correo
<#L#> que han sido devueltos procedentes de su dirección.

</#aE/>

Una copia de estos mensajes puede estar en el archivo.

</#aE/>

Para recibir los mensajes desde el número 123 al 145 (con un
máximo de 100 por petición), escriba a:

   <<#L#>-get.123_145@<#H#>>

Para recibir una lista por "Asunto" y "Autor" de los últimos 100
mensajes, envíe un mensaje en blanco a:

   <<#L#>-index@<#H#>>

</#E/>
Aquí están los números de los mensajes:

</text/dig-bounce-num#E/>

He guardado una lista de los resúmenes de la lista de correo
<#L#> que han sido devueltos desde su dirección. Por cada uno de
los resúmenes perdidos, he anotado el número del primer mensaje
en el resumén.

</#aE/>

No archivo los resúmenes propiamente dichos, pero puede conseguir
los mensajes del archivo principal de la lista.

Para recuperar el grupo de mensajes del 123 al 145 (máximo 100
por petición), envíe un mensaje en blanco a:

   <<#L#>-get.123_145@<#H#>>

Para recibir una lista por "Asunto" y "Autor" de los últimos 100
mensajes, envíe un mensaje en blanco a:

   <<#L#>-index@<#H#>>

</#E/>
Estos son los números de mensaje de los resúmenes:

</text/bounce-probe#E/>

Parece que han sido devueltos algunos mensajes dirigidos a usted
de la lista de correo <#l#>. Le he enviado un mensaje de aviso,
pero también ha sido devuelto. Le adjunto una copia del mensaje
devuelto.

Esta es una prueba para comprobar si su dirección es accesible.
Si esta prueba me es devuelta, eliminaré su dirección de la lista
de correo <#l#>@<#H#>, sin más avisos. En ese caso, puede usted
volver a suscribirse mandando un mensaje a esta dirección:

   <<#l#>-subscribe@<#H#>>

</text/bounce-warn#E/>

Han sido devueltos algunos mensajes para usted de la lista de
correo <#l#>. Le adjunto una copia del primer mensaje devuelto
que recibí. Si también se devuelve este mensaje, le mandaré una
prueba. Si se devuelve la prueba, eliminaré su dirección de la
lista de correo <#l#> sin más avisos.

</text/digest#dE/>
Para suscribirse al resumen escriba a:

	<#L#>-digest-subscribe@<#H#>

Para cancelar su suscripción al resumen, escriba a:

	<#L#>-digest-unsubscribe@<#H#>

Para mandar un mensaje a la lista, escriba a:

	<#L#>@<#H#>

</text/get-bad#E/>
Lo siento, ese mensaje no está en el archivo.

</text/help#E/>

Este es un mensaje genérico de ayuda. El mensaje que recibí no
fue mandado a ninguna de mis direcciones-comando.

Aquí hay una lista de las direcciones comando disponibles:

Mande un correo a las siguientes direcciones-comando para obtener
información y FAQ de esta lista:

   <<#L#>-info@<#H#>>
   <<#L#>-faq@<#H#>>

</#dE/>
Para la lista de resúmenes existen direcciones-comando análogas:

   <<#L#>-digest-subscribe@<#H#>>
   <<#L#>-digest-unsubscribe@<#H#>>

# ezmlm-make -i needed to add ezmlm-get line. If not, we can't do
# multi-get!
</#aE/>

Para recibir los mensajes desde el número 123 al 145 (con un
máximo de 100 por petición), escriba a:

   <<#L#>-get.123_145@<#H#>>

Para obtener un índice con los campos "Asunto" y "Autor" para los
mensajes del 123 al 456, debe escribir a:

   <<#L#>-index.123_456@<#H#>>

Para recibir todos los mensajes con el mismo "Asunto" que el
mensaje 12345, mande un mensaje en blanco a:

   <<#L#>-thread.12345@<#H#>>

</#E/>

En realidad no es necesario que los mensajes estén en blanco,
pero si no lo están ignoraré su contenido. Sólo es importante la
DIRECCIÓN a la que se envía.

Usted puede suscribir una dirección alternativa, por ejemplo,
para "david@ordenador.dominio", simplemente añada un guión y su
dirección (con '=' en lugar de '@') después del comando:

   <<#L#>-subscribe-david=ordenador.dominio@<#H#>>

Para cancelar la suscripción de esta dirección, escriba a:

<<#L#>-unsubscribe-david=ordenador.dominio@<#H#>>

</text/mod-help#E/>
Gracias por acceder a moderar la lista <#L#>@<#H#>.

Mis comandos son algo distintos a los de otras listas de correo,
pero creo que los encontrará intuitivos y fáciles de utilizar.

Estas son algunas instrucciones para las tareas que debe realizar
como propietario y/o moderador de una lista de correo.

Al final del mensaje se incluyen algunas instrucciones generales
para la lista.

Suscripción remota
-------------------

Como moderador, puede añadir o quitar cualquier dirección de la
lista. Para suscribir "david@ordenador.dominio", basta con poner
un guión después del comando, y después su dirección con '=' en
lugar de '@'. Por ejemplo, para suscribir esa dirección, mande
correo a:

   <<#L#>-subscribe-john=host.domain@<#H#>>

De la misma manera puede eliminar la dirección de la lista
mandando un mensaje a:

   <<#L#>-unsubscribe-john=host.domain@<#H#>>

</#dE/>
Para suscribirse o darse de baja de la lista de resúmenes:

   <<#L#>-digest-subscribe-john=host.domain@<#H#>>
   <<#L#>-digest-unsubscribe-john=host.domain@<#H#>>

</#E/>

Eso es todo. No es necesario poner "Asunto" o cuerpo principal en
el mensaje.

</#rE/>

Le mandaré una petición de confirmación, para asegurarme que
usted me envió la petición. Simplemente responda al mensaje, y se
cursará su petición. </#RE/> Mandaré una petición de confirmación
a la dirección del usuario, en este caso a
<david@ordenador.dominio>. El usuario simplemente debe responder
a este mensaje de confirmación. </#E/>

Las confirmaciones son necesarias para impedir, en la medida de
lo posible, que un tercero añada o quite una dirección de la
lista.

Notificaré al usuario cualquier cambio en el estado de su
suscripción.

Suscripción
------------

Cualquier usuario puede suscribirse o darse de baja mandando un
correo a:

   <#L#>-subscribe@<#H#>
   <#L#>-unsubscribe@<#H#>

</#dE/>
Para la lista de resúmenes:

   <#L#>-digest-subscribe@<#H#>
   <#L#>-digest-unsubscribe@<#H#>

</#E/>

El usuario recibirá una petición de confirmación para asegurarse
que él/ella posee la dirección de suscripción. Una vez
verificada, se procederá a dar de baja al usuario.

</#sE/>

Como esta lista está moderada para suscripciones, mandaré una
segunda petición de confirmación al moderador. Como el usuario ya
ha confirmado su deseo de estar en la lista, usted, como
moderador, puede estar seguro de que la dirección del suscriptor
es real. Si quiere aprobar la petición del usuario, simplemente
responda a mi mensaje de confirmación. En caso contrario, puede
simplemente borrar mi mensaje o contactar con el suscriptor para
pedir más información.

</#SE/>
Las suscripciones funcionan del mismo modo.
</#E/>

El usuario también puede utilizar:

   <<#L#>-subscribe-maria=ordenador.dominio@<#H#>>
   <<#L#>-unsubscribe-maria=ordenador.dominio@<#H#>>

para que le manden correo a: "maria@ordenador.dominio". Solo si
ella recibe correo en esta dirección, podrá recibir la petición
de confirmación y mandar una contestación.

Su dirección e identidad no serán reveladas al suscriptor, a no
ser que le mande correo directamente al usuario.

</#rlE/>

Para conseguir una lista de suscriptores para <#L#>@<#H#> envíe
un mensaje a:

   <<#L#>-list@<#H#>>

Para conseguir un log de la lista de transacciones para
<#L#>@<#H#> mande un mensaje a:

   <<#L#>-log@<#H#>>

</#rldE/>
Para suscriptores al resumen:

   <<#L#>-digest-list@<#H#>>

y:

   <<#L#>-digest-log@<#H#>>

</#rnE/>

Usted puede modificar remotamente los ficheros de texto que
componen las respuestas mandadas por la lista. Para conseguir una
lista de ficheros e instrucciones de edición, escriba a:

   <<#L#>-edit@<#H#>>

</#mE/>
Mensajes moderados.
-------------------

Cuando los mensajes están moderados, guardaré el mensaje enviado
y le mandaré una copia junto con instrucciones. El mensaje para
usted llevará "MODERATE for ..." como "Asunto".

Para aceptar el mensaje, simplemente responda a la dirección que
figura en el campo 'Responder a:' , que ya he configurado con la
dirección correcta de aceptación. No necesita incluir el mensaje
en sí. De hecho, ignoraré el contenido siempre y cuando la
dirección a la que escriba sea correcta.

Si quiere rechazar el mensaje, mande un correo a la dirección
'De:', que ya he configurado con la dirección correcta de
"rechazo". Eso normalmente se hace con 'Responder a todos', y
borrando después todas las direcciones salvo la dirección
"rechazada". Puede añadir un comentario al remitente insertando
dicho comentario entre dos líneas que empiecen con tres '%'. Solo
mandaré este comentario al remitente con el mensaje rechazado.
Una vez más, no revelaré su identidad.

Procesaré el mensaje en función de la primera respuesta que
recibo. Le avisaré si me manda una petición para aceptar un
mensaje que, Previamente, había sido rechazado o viceversa.

Si no recibo respuestas del moderador dentro de un cierto periodo
de tiempo (normalmente 5 días), devolveré el mensaje al remitente
con una explicación de lo que ha pasado. Su administrador también
puede configurar la lista para que estos mensajes "ignorados"
simplemente sean borrados sin notificación, en lugar de ser
devueltos al remitente.

</#E/>

Vacaciones
----------

Si está temporalmente en otra dirección, simplemente reenvíe
todos los mensajes que tienen el encabezamiento 'Mailing-List:'
(o todos los que tienen "Asunto" empezando con 'MODERATE for
<#L#>@<#H#>' o con 'CONFIRM subscribe to <#L#>@<#H#>') a la nueva
dirección. Así podrá leer la lista desde la nueva dirección. Como
alternativa, puede reenviar los mensajes a un amigo para que él o
ella los lea en su lugar. Por favor, antes de hacerlo consiga el
permiso del propietario de la lista.

Si desea aprobar automáticamente todas las peticiones mientras
esté fuera, configure su programa de correo para responder
automáticamente a mensajes que tienen un "Asunto" que reúna los
criterios anteriormente expuestos.

</#rE/>

Si intenta administrar la lista remotamente, desde una dirección
que no es la suya, el suscriptor cuya dirección está utilizando,
y no usted, recibirá la petición de confirmación. Entonces, se
mandará una petición de confirmación a todos los moderadores.
Hago esto porque no tengo manera de saber que usted realmente ha
mandado la petición original.

En este caso, ¡Recuerde que se manda su petición original (y su
dirección) al suscriptor!

</#E/>

¡Buena suerte!

PD: Por favor, póngase en contacto con el propietario de la lista
(<#L#>-owner@<#H#>) si tiene preguntas o problemas.

</text/mod-reject#E/>

Lo siento, el mensaje que le adjunto no fue aceptado por el
moderador. Si el moderador ha hecho algún comentario, aparecerá
en la parte inferior.

</text/mod-request#E/>

El mensaje adjunto fue mandado a la lista de correo <#L#>@<#H#>.

Si desea aprobar su distribucisn a todos los suscriptores, por
favor, escriba a:

!A

Normalmente esto ocurre al pulsar el botón "responder". Usted
puede comprobar la dirección para asegurarse de que empieza por
"<#L#>-accept" . Si esto no funciona, simplemente copie la
dirección y péguela en el campo "Para:" de un nuevo mensaje.
</#xE/>

Como alternativa haga clic aquí:

	mailto:<#A#>
</#E/>

Para rechazar el mensaje y hacer que sea devuelto al remitente,
por favor, mande un mensaje a:

!R

Normalmente, lo más fácil es hacer clic en el botón "Responder a
todos" y luego quitar todas las direcciones menos la que empieza
con "<#L#>-reject".
</#xE/>

Como alternativa, haga clic aquí:
	mailto:<#R#>
</#E/>

No es necesario copiar el mensaje en su respuesta para aceptarlo
o rechazarlo. Si desea mandar un comentario al remitente de un
mensaje rechazado, por favor, inclúyalo entre dos líneas que
empiezan con tres signos de porcentaje ('%'):

%%% Inicio del comentario
%%% Fin del comentario

¡Gracias por su ayuda!

--- Se adjunta el mensaje enviado.

</text/mod-sub#E/>

--- Le he suscrito o dado de baja por petición del moderador de
la lista de correo <#l#>@<#H#>.

Si no está de acuerdo con esta acción, por favor, mande una queja
u otros comentarios al propietario de la lista
(<#l#>-owner@<#H#>) tan pronto como le sea posible.

</text/mod-timeout#E/>

Lo siento, los moderadores de la lista <#L#> no han procesado su
mensaje. Por esa razón, se lo devuelvo. Si cree que esto es un
error, por favor, vuelva a mandar el mensaje o póngase en
contacto con el moderador de la lista directamente.

--- Le adjunto el mensaje que mandó.

</text/mod-sub-confirm#E/>
Respetuosamente pido permiso para añadir

!A

a los suscriptores de la lista de correo <#l#>. Esta petición o
procede de usted o ha sido ya verificada por el suscriptor.

Para confirmar, por favor, envíe un mensaje en blanco a esta
dirección:

!R

Normalmente esto ocurre al pulsar el botón "Responder". Si esto
no funciona, simplemente copie la dirección y péguela en el campo
"Para:" de un nuevo mensaje. </#xE/>

o haga clic aquí:

	mailto:<#R#>
</#E/>

Si no está de acuerdo, simplemente ignore este mensaje. 

¡Gracias por su ayuda!

</text/mod-unsub-confirm#E/>
Se ha hecho una petición para eliminar

!A

de la lista de correo <#l#>. Si está de acuerdo, por favor, envíe
un mensaje en blanco a esta dirección:

!R

Normalmente, esto ocurre al pulsar el botón "Responder". Si esto
no funciona, simplemente copie la dirección y péguela en el campo
"Para:" de un nuevo mensaje. 
</#xE/>

o haga clic aquí:

	mailto:<#R#>
</#E/>

Si no está de acuerdo, simplemente ignore este mensaje. 

¡Gracias por su ayuda!

</text/sub-bad#E/>
¡Vaya!, parece que el número de confirmación no es válido. 

La principal causa por la que los números se invalidan es su
expiración. Yo tengo que recibir confirmación de cada petición en
un plazo de diez días. Además, asegúrese de que el número de
confirmación completo estaba incluido en la respuesta que me
mandó. Algunos programas de correo tienen la (mala) costumbre de
cortar parte de la dirección de respuesta, que puede ser muy
larga.

He configurado un nuevo número de confirmación. Para confirmar
que le gustaría que

!A

fuese añadida a la lista de correo <#l#>, por favor, mande un
mensaje en blanco a esta dirección:

!R
</#xE/>

o haga clic aquí:

	mailto:<#R#>
</#E/>

De nuevo, compruebe cuidadosamente la dirección de la respuesta
para asegurarse que esté todo incluido antes de confirmar su
suscripción.

Perdone las molestias.

	<#L#>-Propietario <<#l#>-owner@<#H#>>

</text/sub-confirm#E/>
Para confirmar que le gustaría que

!A

fuese añadido a la lista de correo <#l#>, por favor, envíe un
mensaje en blanco a esta dirección:

!R

Normalmente esto ocurre al pulsar el botón "Responder". Si eso no
funciona, es suficiente copiar la dirección y pegarla en el campo
"Para:" de un nuevo mensaje. 
</#xE/>

o haga clic aquí:

	mailto:<#R#>
</#E/>

Esta confirmación cumple dos propósitos. Primero, verifica que
puedo mandarle correo. Segundo, le protege en el caso de que
alguien intente falsificar una petición de suscripción en su
nombre.

</#qE/>

Algunos programas de correo no pueden manejar direcciones largas.
Si no puede responder a esta petición, envíe un mensaje a
<<#L#>-request@<#H#>> y ponga la dirección entera escrita arriba
en la línea de "Asunto:".

</#sE/>

Esta lista está moderada. Una vez que haya enviado esta
confirmación, se mandará la petición al moderador de la lista.
Cuando su suscripción haya sido activada, se lo notificaré.

</text/sub-nop#E/>
No he conseguido cursar su petición: La dirección

!A

ya estaba en la lista de correo <#l#> cuando recibí su petición,
y usted sigue siendo suscriptor.

</text/sub-ok#E/>
Acuse de recibo: He añadido la dirección

!A

A la lista de correo <#l#>.

¡Bienvenido a <#l#>@<#H#>!

Por favor guarde este mensaje para que sepa bajo que dirección
está suscrito, por si luego quiere cancelar su suscripción o
cambiar la dirección de la misma.

Para cancelar su suscripción mande un mensaje a:

    <<#l#>-unsubscribe-<#t#>@<#H#>>

</text/top/>

¡Hola! Soy el programa ezmlm. Me ocupo de la lista de correo
<#l#>@<#H#>.

</#x/>

Estoy trabajando para mi propietario, a quien se puede localizar
en <#l#>-owner@<#H#>.

</text/unsub-bad#E/>
¡Vaya!, parece que ese número de confirmación es inválido. 

La principal razón por la que los números de confirmación se
invalidan es la expiración. Debo recibir confirmación de cada
petición en un plazo de diez días. Además, asegúrese que el
número completo de confirmación estaba incluido en la respuesta
que me mandó. Tenga en cuenta que algunos programas de correo
tienen la (mala) costumbre de cortar parte de la dirección de
respuesta, que puede ser muy larga.

He configurado un nuevo número de confirmación. Para confirmar
que le gustaría que

!A

fuese dado de baja en la lista de correo <#l#>, por favor, mande
un mensaje en blanco a esta dirección:

!R
</#xE/>

o haga clic aquí:

	mailto:<#R#>
</#E/>

De nuevo, compruebe la dirección de respuesta cuidadosamente para
asegurarse que esté todo incluido antes de confirmar esta acción.

Perdone las molestias.

	<#l#>-Owner <<#l#>-owner@<#H#>>

</text/unsub-confirm#E/>
Para confirmar que le gustaría que

!A

sea dado de baja de la lista de correo <#l#>, por favor, mande un
mensaje en blanco a esta dirección:

!R

Normalmente esto ocurre al pulsar el botón "Responder". Si no
funciona, simplemente copie la dirección y péguela en el campo
"Para:" de un nuevo mensaje. 
</#xE/>

o haga clic aquí:

	mailto:<#R#>
</#E/>

No he comprobado si su dirección está actualmente en la lista de
correo. Para ver que dirección utilizó para suscribirse, mire los
mensajes que está recibiendo de la lista de correo. Cada mensaje
tiene su dirección oculta dentro de la ruta de retorno; por
ejemplo, maria@xdd.ff.com recibe mensajes con la ruta de retorno:
<<#l#>-return-<número>-maria=xdd.ff.com@<#H#>>.

</#qE/>

Algunos programas de correo no pueden manejar direcciones largas.
Si no puede responder a esta petición, envíe un mensaje a
<<#L#>-request@<#H#>> y escriba la dirección completa en la línea
de "Asunto:".

</text/unsub-nop#E/>
Lo siento, no he podido cursar su petición porque la dirección

!A

no estaba en la lista de correo <#l#> cuando recibí su petición y
no es suscriptor de esta lista.

Si se da de baja, pero sigue recibiendo correo, es que está
suscrito con una dirección distinta a la que usa actualmente. Por
favor, busque en las cabeceras el texto:

'Return-Path: <<#l#>-return-1234-user=host.dom@<#H#>>'

La dirección para dar de baja a este usuario sería:
'<#l#>-unsubscribe-user=host.dom@<#H#>'.

Simplemente escriba a esa dirección, tras modificarla con la
verdadera dirección de suscripción.

Si el mensaje tiene una cabecera ``List-Unsubscribe:'' puede
usted mandar un mensaje a la dirección de esa cabecera. La
cabecera ya contiene la petición de suscripción.

En algunos programas de correo, necesitará hacer visibles los
encabezamientos para ver el campo de retorno:

Con Eudora 4.0, haga clic en el botón "Blah blah ...". Con
PMMail, haga clic en "Window->Show entire message/header".

Si esto tampoco da resultado, siento decirle que no le puedo
ayudar. Por favor, reenvíe el mensaje junto con una nota sobre lo
que está intentando hacer y una lista de direcciones bajo las
cuales puede estar suscrito, a mi propietario:

    <#l#>-owner@<#H#>

que se ocupará de todo. Mi propietario es un poco más lento que
yo; por favor, tenga paciencia.

</text/unsub-ok#E/>
Acuse de recibo: He dado de baja la dirección

!A

de la lista de correo <#l#>. Esta dirección ya no está suscrita.

</text/edit-do#nE/>

Por favor edite el siguiente fichero de texto y envíelo a esta
dirección:

!R

Su programa de correo debería tener la opción "Responder" que
utiliza esta dirección automáticamente.

Puedo quitar las comillas que su programa añade al texto, siempre
y cuando usted no modifique las líneas marcadoras (las que
empiezan con '%%%'). Estas líneas no deben ser modificadas (solo
son tolerados caracteres añadidos por su programa de correo al
principio de la línea).

</text/edit-list#nE/>

El comando <#L#>-edit.file puede ser utilizado por un
administrador remoto para modificar los ficheros de texto que
componen la mayoría de las respuestas de la lista de correo
<#L#>@<#H#>.

Lo que sigue es un listado de los ficheros de respuesta y una
corta indicación de cuando se utilizan sus contenidos. Para
modificar un fichero, simplemente envíe un correo a
<#L#>-edit.fichero, sustituyendo 'fichero' por el nombre del
fichero. Las instrucciones para las modificaciones se envían con
el fichero.

File                Use

bottom        final de todas las respuestas. Información general
              sobre comandos.	      
digest        sección administrativa de resúmenes. 
faq           preguntas frecuentes propias de esta lista.
get_bad       en lugar de mensajes no encontrados en el archivo.
help          ayuda general (entre 'top' y 'bottom').
info          información sobre la lista. La primera línea debe
              tener significado por sí misma. 
mod_help      ayuda específica para moderadores.
mod_reject    al remitente del mensaje rechazado.
mod_request   a los moderadores de mensajes junto a los mensajes.
mod_sub       al suscriptor después de que el moderador confirme
              la suscripción.
mod_sub_confirm  al moderador para pedir confirmación de 
                 suscripción.
mod_timeout   al remitente de correo caducado.  
mod_unsub_confirm  al administrador remoto para pedir confirmación
                   de baja.
sub_bad       al suscriptor si la confirmación no fue correcta.
sub_confirm   al suscriptor para pedir confirmación de 
              suscripción.
sub_nop       al suscriptor después de re-suscribirse.
sub_ok        al suscriptor después de la suscripción. 
top           el principio de todas las respuestas.
</#tnE/>
trailer       añadido a todos los mensajes enviados de la lista.
</#nE/>
unsub_bad     al suscriptor si la confirmación de baja fue 
              incorrecta.
unsub_confirm al suscriptor para pedir confirmación de 
              cancelación.
unsub_nop     al no suscriptor después de darse de baja.
unsub_ok      al ex suscriptor después de darse de baja.

</text/edit-done#nE/>
El fichero de texto fue actualizado correctamente.
</text/info#E/>
No se ha proporcionado información para esta lista.
</text/faq#E/>
FAQ - Preguntas más comunes para la lista <#l#>@<#H#>.

Ninguno disponible todavía. 


