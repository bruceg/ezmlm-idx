dependon('build/make-messages-c.sh','conf-lang','Makefile')
messages = [ 'lang/{}/messages'.format(lang) for lang in listdir('lang') ]
dependon(*[ m for m in messages if exists(m) ])
formake('sh build/make-messages-c.sh < lang/$$(head -n 1 conf-lang)/messages >messages-txt.c')
