0.324 - This version identifier must be on line 1 and start in pos 1.
#
#$Id: ezmlmrc.ru,v 1.4 1999/12/23 23:08:19 lindberg Exp $
#$Name: ezmlm-idx-040 $
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
# Charset file is a must for russian mailing lists
koi8-r
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
|<#B#>/ezmlm-issubn -n '<#D#>/deny' || { echo "Sorry, I've been told to reject your posts. Contact <#L#>-owner@<#H#> if you have questions about this (#5.7.2)"; exit 100 ; }
# switch -u=> restrict to subs of list & digest. If not m
# do it with ezmlm-issubn, if 'm' do it with ezmlm-gate
</#uM/>
|<#B#>/ezmlm-issubn '<#D#>' '<#D#>/digest' '<#D#>/allow' '<#D#>/mod' || { echo "Sorry, only subscribers may post. If you are a subscriber, please forward this message to <#L#>-owner@<#H#> to get your new address included (#5.7.2)"; exit 100 ; }
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
X-Comment: <#l#> mailing list (Russian, KOI8-R)
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
-- 
To unsubscribe, e-mail: <#L#>-unsubscribe@<#H#>
For additional commands, e-mail: <#L#>-help@<#H#>
</text/bottom/>

--- Команды списка рассылки <#l#>@<#H#> ---

Все команды обрабатываются автоматически. Пожалуйста,
не посылайте их на адрес самого списка, там они обработаны
не будут, а вас обругают подписчики. Командой является
письмо (независимо от его содержания), посланное на
один из административных адресов:

Адрес для подписки:
   <<#L#>-subscribe@<#H#>>

Адрес для отказа от рассылки:
   <<#L#>-unsubscribe@<#H#>>

Для получения правил списка рассылки и FAQ:
   <<#L#>-info@<#H#>>
   <<#L#>-faq@<#H#>>

</#d/>
Так же поддерживаются и дайджесты:
   <<#L#>-digest-subscribe@<#H#>>
   <<#L#>-digest-unsubscribe@<#H#>>

# ezmlm-make -i needed to add ezmlm-get line. If not, we can't do
# multi-get!
</text/bottom#ai/>
Чтобы получить сообщения с 123 по 145 (max 100 на письмо), пишите сюда:
   <<#L#>-get.123_145@<#H#>>

</text/bottom#aI/>
Для получения письма #12 из архива пошлите письмо по адресу:
   <<#L#>-get.12@<#H#>>

</text/bottom#i/>
Адрес для получения индекса с subject и авторами писем 123-456:
   <<#L#>-index.123_456@<#H#>>

# Lists need to be both archived and indexed for -thread to work
</text/bottom#ai/>
Адрес для получения все писем, связанных с письмом #12345:
   <<#L#>-thread.12345@<#H#>>

# The '#' in the tag below is optional, since no flags follow.
# The name is optional as well, since the file will always be open
# at this point.
</text/bottom#/>
Содержимое писем роли не играет, важен лишь адрес, на который
вы посылаете письмо.

</text/bottom/>
Если ничего не помогает, вы можете связаться с владельцем
списка рассылки по адресу <#L#>-owner@<#H#>. 
</text/bottom/>

--- Ниже приведена копия запроса

</text/bounce-bottom/>

--- Ниже приведена копия сообщения об ошибке

</text/bounce-num/>

Системой сохранен список писем из <#L#>, которые не дошли до
вашего адреса (т.е. на них было получено сообщение об ошибке)

</#a/>
Копии этих писем находятся в архиве.
</#aI/>
Чтобы получить письмо 12345 из архива, пошлите письмо по адресу:
   <<#L#>-get.12345@<#H#>>

</#ia/>
Адрес для получения сообщений 123-145 (max 100 на запрос):
   <<#L#>-get.123_145@<#H#>>

Адрес для получения списка последних 100 сообщений с subject и автором:
   <<#L#>-index@<#H#>>

<//>
Hомера сообщений:

</text/dig-bounce-num/>

Системой сохранены список дайджестов <#L#>, которые не дошли до вашего
адреса (т.е. на них были получены сообщения об ошибке). Сами дайджесты
не хранятся, но известны номера писем в дайджестах. Таким образом
можно запросить пропущенные письма из архива.

</#aI/>
Чтобы получить письмо 12345 из архива, пошлите письмо по адресу:
   <<#L#>-get.12345@<#H#>>

</#ia/>
Адрес для получения сообщений 123-145 (max 100 на запрос):
   <<#L#>-get.123_145@<#H#>>

Адрес для получения списка последних 100 сообщений с subject и автором:
   <<#L#>-index@<#H#>>

<//>
Последние номера писем в дайджестах:

</text/bounce-probe/>

Сообщения из списка рассылки <#l#>@<#H#> на ваш адрес
приводят к ошибкам доставки (bounce). Вам было послано письмо с
предупреждением, но оно тоже не было доставлено. В конце письма
приведена копия сообщения об ошибке.

Это письмо является тестом на существование и активность вашего 
адреса. Если попытка доставить вам это письмо так же будет неудачна,
ваш адрес будет удален из списка рассылки <#l#>@<#H#> без дальшейших
предупреждений. Подписаться заново можно по адресу:
   <<#l#>-subscribe@<#H#>>

</text/bounce-warn/>

Сообщения из списка рассылки <#l#>@<#H#> на ваш адрес
приводят к ошибкам доставки (bounce). В конце письма приведена копия
сообщения об ошибке.

Если это письмо так же приведет к ошибке, вам будет выслан тест. Если
тест тоже приведет к ошибке, ваш адрес будет удален из списка рассылки
<#l#>@<#H#> без дальнейших предупреждений.

</text/digest#d/>
Адрес для подписки на дайджест:
	<#L#>-digest-subscribe@<#H#>

Адрес для отписки от дайджеста:
	<#L#>-digest-unsubscribe@<#H#>

Адрес для отправки писем в список рассылки:
	<#L#>@<#H#>

</text/get-bad/>
Извините, но такого письма в архиве нет.

</text/help/>
Это письмо является общим описанием работы ezmlm.

</text/mod-help/>
Спасибо, что вы согласились модерировать <#L#>@<#H#>.

Команды ezmlm немного отличаются от других систем списков
рассылки как majordomo, listserver, etc, но они легко
запоминаются и их легко использовать.

Hиже приведены инстукции по выполнению задач, необходимых 
владельцу списка и/или модератору.

Удаленная подписка
------------------
Как модератор, вы можете подписать или отписать любой адрес
в списке. Для подписки адреса john@host.domain добавьте
перенос после команды, потом адрес с = вместо @. Hапример,
для подписки вышеуказанного адреса, следует послать письмо
по адресу
   <<#L#>-subscribe-john=host.domain@<#H#>>

Точно так же можно удалить адрес из списка:
   <<#L#>-unsubscribe-john=host.domain@<#H#>>

</#d/>
То же самое для дайджестов:
   <<#L#>-digest-subscribe-john=host.domain@<#H#>>
   <<#L#>-digest-unsubscribe-john=host.domain@<#H#>>

<//>
Вот и все. Hе требуется ничего заполнять ни в subject, ни в теле письма.

</#r/>
Вам будет выслан запрос на подтверждение, действительно ли вы хотели
выполнить подписку/отписку. Hадо просто ответить на него.
</#R/>
Будет выслан запрос на подтверждение подписки по адресу <john@host.domain>.
Пользователю достаточно будет ответить на запрос.
<//>

Система подтверждения абсолютна необходима, чтобы не дать недоброжелателю
возможности добавить или удалить адрес в списке без желания владельца
адреса.

Подписка
--------

Любой пользователь может подписаться или отписаться,
послав письмо на соответствующий адрес:

<#L#>-subscribe@<#H#>
<#L#>-unsubscribe@<#H#>

</#d/>
Для дайджестов:

<#L#>-digest-subscribe@<#H#>
<#L#>-digest-unsubscribe@<#H#>

<//>
Пользователь получит запрос на подтверждение, чтобы убедиться
в том, что запрос был сделан именно им. 

</#s/>
Поскольку в данном списке контролируется подписка/отписка,
будет выслан дополнительный запрос модератору. Поскольку
пользователь уже подтвердил свое желание, вы, как модератор,
можете быть уверены, что это именно его желание, а адрес
работающий. Если вы согласны с подпиской данного пользователя,
просто пошлите ответ на данное письмо. Если нет, то сотрите
его и все.

</#S/>
Отписка работает таким же образом.
<//>

Пользователь так же может использовать адреса:

   <<#L#>-subscribe-mary=host.domain@<#H#>>
   <<#L#>-unsubscribe-mary=host.domain@<#H#>>

для подписки mary@host.domain. Список будет изменен только если
кто-то на этом адресе ответит на запрос.

Ваш адрес и прочая информация не будут доступны подписчику, разве
что вы пошлете отдельное письмо ему напрямую.

</#rl/>
Чтобы получить список подписчиков <#L#>@<#H#>, пошлите письмо сюда:
   <<#L#>-list@<#H#>>

Чтобы получить лог транзакций <#L#>@<#H#>, пишите сюда:
   <<#L#>-log@<#H#>>

</#rld/>
Для дайджестов:
   <<#L#>-digest-list@<#H#>>
и:
   <<#L#>-digest-log@<#H#>>

</#rn/>
Вы можете редактировать по почте текстовые файлы конфигурации
списка рассылки. Для получения списка файлов и инструкций по
редактированию, пишите сюда:
   <<#L#>-edit@<#H#>>

</#m/>
Модерирование
-------------
Когда список рассылки модерируется, письма сохраняются и всем
модераторам посылается копия письма с инструкцией. Subject содержит
строку "MODERATE for ...".

Письмо содержит два заголовка: "From:" и "Reply-To:". Таким образом,
когда вы на него отвечаете, ваша почтовая программа должна спросить,
на какой из адресов отвечать. Ответ на адрес в Reply-To: приведет
к тому, что исходное письмо будет пропущено в список рассылки. 
Ответ на "From:" приведет к отказу. Обычно программы спрашивают
"Да/нет", т.е. вы просто решаете, пропускать или нет, нажимаете
ответ и выбираете "да" или "нет". Содержимое вашего письма
практически игнорируется -- значение имеют только адреса, однако
при отказе можно вставить в тело письма текст между двумя строками,
начинающимися с символов %. Этот текст будет послан отправителю 
письма, не открывая кто из модераторов его послал. Hапример:

%%% Start comment
ваше письмо содержит мат
%%% End comment

Если ваша почтовая программа умеет работать с темплейтами (например,
The Bat!), то эти строки стоит добавить в темплейт ответа.

Запросы на модерирование обрабатываются по первому письму от модератора,
среагировавшего раньше. Если кто-то из модераторов позже пошлет ответ
с противоположным решением, ему будет сообщено о том, что уже произошло
с данным письмом.

Если в течении нескольких дней не будет получено ответа ни от одного
модератора, отправителю будет послано уведомление о задержке. Так же,
администратор списка может запретить отсылку подобных уведомлений.
<//>

Каникулы
--------
Если вы должны срочно покинуть свой любимый город, а там, куда
вы собрались, интернета нет и не предвидится, вы можете на время
включить автоматический пропуск писем в список. Однако во многих
списках подобные действия могут привести к бардаку.

Для этого достаточно поставить автоответчик на вашем адресе,
отправляющий все письма с subject "MODERATE for .." на адрес в 
заголовке "Reply-To:". HЕ РЕКОМЕHДУЕТСЯ. 

</#r/>
Если вы попробуете послать админстративный запрос не с своего адреса,
то подписчик, а не вы будет спрошен, подписывать или нет. Это сделано
для того, чтобы никто не послал поддельный запрос на подписку 
от вашего адреса, подписав своего врага на высокотраФФиковый список
рассылки.

<//>

Удачи!

PS: В случае проблем связывайтесь с владельцем списка 
рассылки (<#L#>-owner@<#H#>).

</text/mod-reject/>
Извините, но ваше письмо (ниже приведенное) не было пропущено в
список модератором. Если модератор хотел(а) сообщить что-либо вам
по поводу вашего письма, комментарии будут приведены ниже.
</text/mod-request/>
Hижеприведенное письмо было отправлено в список <#L#>@<#H#>
Если вы согласны его пропустить, пошлите письмо по адресу:

!A

Обычно для этого достаточно нажать кнопку "reply" или "ответ" для
данного письма. Обязательно проверьте, чтобы в поле "To:" был только
один адрес. Если это не сработает, скопируйте адрес в clipboard и
вставьте его в поле "To:". 
</#x/>
<//>

Для отказа от пропуска письма и сообщения об этом пишущему пошлите
письмо по адресу

!R
</#x/>
<//>

Вам не нужно копировать тело исходного письма. Для того, чтобы послать
ваш комментарий по поводу того, почему письмо не было пропущено,
вставьте ваш текст между двумя строками с %%%.

%%% Start comment
%%% End comment

Комментарии должны начинаться с начала строки.

--- Исходное письмо в список.

</text/mod-sub#E/>
--- Вас подписали или отписали по запросу модератора 
списка рассылки <#l#>@<#H#>.

Если вы этого не хотели, вы можете написать жалобу
владельцу списка по адресу <#l#>-owner@<#H#>.

Если вас интересует информация о списке рассылки <#L#>,
пошлите пустое письмо по адресу <#l#>-help@<#H#>.

</text/mod-timeout/>
Извините, но модератор(ы) списка рассылки <#L#>@<#H#>
не предпринимают действий для пропуска или отказа по поводу 
вашего письма в список.

--- Исходное письмо.

</text/mod-sub-confirm/>
Это запрос вашего разрешения на добавление адреса

!A

в список рассылки <#l#>@<#H#>. 
Запрос произошел потому, что вы (или не вы) попытались 
подписать ваш адрес на вышеупомянутый список рассылки.

Для подтверждения подписки, пошлите пустое письмо по адресу:

!R

Для этого достаточно нажать кнопку "reply" или "ответ".
Если это не сработает, скопируйте адрес в clipboard и 
вставьте его в поле "To:".
</#x/>
<//>

Если вы не хотите подписываться, просто не отвечайте на это письмо.

</text/mod-unsub-confirm/>
Запрос на разрешение удаления

!A

из списка рассылки <#l#>@<#H#>. Если вы согласны, 
пошлите пустое письмо по адресу:

!R

Для этого достаточно нажать кнопку "reply" или "ответ".
Если это не помогает, скопируйте текст и вставьте его в 
поле "To:" нового письма.
</#x/>
<//>

Если вы не согласны, игнорируйте это письмо.

</text/sub-bad/>
Ой. Код подтверждения некорректен.

Скорее всего прошло слишком много времени после получения вами
запроса. Максимум 10 дней. Так же возможно, что ваша почтовая
программа съела часть адреса, который и содержит код подтверждения.

Для вас создан новый код подтверждения. Чтобы одобрить подписку

!A

на список рассылки <#l#>@<#H#>, пошлите пустое
письмо по адресу:

!R
</#x/>
<//>

Обязательно проверьте адрес, на который вы посылаете подтверждение.

</text/sub-confirm/>
Для подтверждения подписки адреса

!A

на список рассылки <#l#>@<#H#>, пошлите пустое письмо по адресу:

!R

Для этого достаточно нажать кнопку "reply" или "ответ".
</#x/>
<//>

Это подтверждение необходимо по двум причинам. Во-первых, проверяется,
доходит ли почта до вашего адреса. Во-вторых, это защищает вас от
подписки, если кто-то пошлет письмо с подделанным вашим исходным адресом.

</#q/>
Hекоторые старые почтовые программы не могут справится с длинными
адресами. Если вы не можете послать ответ на данное письмо, пошлите
ответ на <<#L#>-request@<#H#>> и вставьте 
весь адрес в поле subject.

</text/sub-confirm#s/>
Этот список модерируемый. Когда вы пошлете подтверждение, модератор(ы)
будут извещены об этом. В случае одобрения вашей подписки модераторам
вам будет сообщено.

</text/sub-nop/>
Подтверждение: адрес

!A

уже подписан на список рассылки <#l#>@<#H#>.

</text/sub-ok#E/>
Подтверждение: адрес

!A

подписан на список рассылки <#l#>@<#H#>.

Добро пожаловать!

Пожалуйста, сохраните это письмо, чтобы не забыть адрес, который
вы подписали на список рассылки. Через полгода вы его обязательно
забудете, а отказаться от подписки, не зная исходный адрес, будет
очень трудно.

</text/top/>
Добрый день/утро/вечер! Это сообщение от программы ezmlm,
заведующей списком рассылки <#l#>@<#H#>.

</#x/>
Связаться с владельцем списка рассылки можно по адресу
<#l#>-owner@<#H#>.

</text/unsub-bad/>
Ой. Код подтверждения некорректен.

Скорее всего прошло слишком много времени после получения вами
запроса. Максимум 10 дней. Так же возможно, что ваша почтовая
программа съела часть адреса, который и содержит код подтверждения.

Для вас создан новый код подтверждения. Чтобы одобрить удаление
адреса

!A

из списка рассылки <#l#>@<#H#>, пошлите пустое письмо по адресу:

!R
</#x/>
<//>

Обязательно проверьте адрес, на который вы посылаете подтверждение.

</text/unsub-confirm/>
Для подтверждение удаления адреса

!A

из списка рассылки <#l#>@<#H#>, пошлите пустое письмо по адресу:

!R

Для этого достаточно нажать кнопку "reply" или "ответ".
</#x/>
<//>

Внимание! Чтобы узнать, под каким адресом вы подписаны на список
рассылки, загляните в заголовки одного из писем из списка. В
каждом письме есть заголовок "Return-Path:", внутри которго
находится адрес получателя. Hапример, при адресе vassily.pupkin@usa.net
заголовок будет выглядеть так:
Return-Path: <<#l#>-return-<число>-vassily.pupkin=usa.net@<#H#>

</#q/>
Hекоторые старые почтовые программы не могут справляться с длинными
адресами. Если вы не можете послать ответ на данное письмо, пошлите
ответ на <<#L#>-request@<#H#>> и вставьте 
весь адрес в поле subject.

</text/unsub-nop/>
Внимание: адрес

!A

не подписан на список рассылки <#l#>@<#H#>!

Если вы отписываетесь от списка, но продолжаете получать сообщения,
значит вы подписаны под другим адресом. Hайдите заголовок вида

'Return-Path: <<#l#>-return-1234-user=host.dom@<#H#>>'

в одном из таких писем. Для отписки надо будет послать письмо по адресу
'<#l#>-unsubscribe-user=host.dom@<#H#>'. Просто сформируйте такой
адрес, заменив user=host.dom на ваши реальные данные (подставив символ
= вместо @) и ответьте на подтверждение.

</text/unsub-ok/>
Подтверждение: адрес

!A

удален из списка рассылки <#l#>@<#H#>.

</text/edit-do#n/>
Пожалуйста, отредактируйте нижеприведенный файл и отправьте его
по адресу:

!R

Для этого достаточно нажать кнопку "reply" или "ответ".

Символы квотинга будут удалены из письма автоматически, если
вы не будете трогать строки с маркерами. Маркеры -- строки,
начинающиеся с %%%. 

</text/edit-list#n/>
Адрес <#L#>-edit.file предназначен для удаленного редактирования
текстовых файлов, которые являются шаблонами ответов списка
рассылки <#L#>@<#H#>.

Ниже приведен список имен файлов и короткое описание функций
каждого из них. Для редактирования какого-либо из этих файлов
просто пошлите письмо по адресу #L#>-edit.имяфайла. Вам будет
выслано текущее содержимое файла и дальнейшие инструкции по 
редактированию.

Файл                Назначение

bottom              нижняя часть ответа, общая информация.
digest              'административная' часть для дайджестов.
faq                 часто задаваемые вопросы и ответы для списка рассылки.
get_bad             сообщение об отсутствии письма в архиве.
help                общий текст (между 'top' и 'bottom').
info                информация о списке. Первая строка является заголовком.
mod_help            все про модерирование данного списка рассылки.
mod_reject          текст отказа в постинге сообщения.
mod_request         текст запроса на модерацию письма.
mod_sub             подтверждение модерируемой подписки.
mod_sub_confirm     запрос модератору о подписке пользователя.
mod_timeout         сообщение о "протухшем" письме в очереди на одобрение.
mod_unsub_confirm   запрос на одобрение отписки пользователя модератору.
sub_bad             подписчику, при некорректном письме-подтверждении.
sub_confirm         запрос на подтверждение подписки пользователю.
sub_nop             если пользователь уже подписан.
sub_ok              при успешной подписке.
top                 верхняя часть всех ответов.
</#tn/>
trailer             текст, добавляемый в конец каждого письма в список.
</#n/>
unsub_bad           подписчику при неудачной отписке.
unsub_confirm       запрос на подтверждение отписки пользователю.
unsub_nop           не-подписчику о том, что его адреса в списке нет.
unsub_ok            при успешной отписке.

</text/edit-done#n/>
Текстовый файл был успешно обновлен на сервере.
</text/info#E/>
Пните администратора списка рассылки, пусто тут.
</text/faq#E/>
FAQ, или ЧаВО -- Часто задаваемые Вопросы и Ответы.

Здесь пока пусто. Может кто-нибудь напишет?

