dependon compile load trylib.c
formake "( ( ./compile trylib.c && ./load trylib -l$2 ) >/dev/null 2>&1 && echo -l$2 || exit 0 ) >$1"
formake 'rm -f trylib.o trylib'
