dependon make-messages-c conf-lang Makefile lang/*/messages
formake './make-messages-c < lang/$$(head -n 1 conf-lang)/messages >messages-txt.c'
