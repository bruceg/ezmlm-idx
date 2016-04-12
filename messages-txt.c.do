dependon('make-messages-c','conf-lang','Makefile')
messages = [ 'lang/{}/messages'.format(lang) for lang in listdir('lang') ]
dependon(*[ m for m in messages if exists(m) ])
formake('./make-messages-c < lang/$$(head -n 1 conf-lang)/messages >messages-txt.c')
