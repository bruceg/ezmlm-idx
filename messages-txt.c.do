dependon make-messages-c conf-lang Makefile lang/*/text/messages
formake './make-messages-c < lang/$$(head -n 1 conf-lang)/text/messages >messages-txt.c'
