dependon compile trydrent.c direntry.h1 direntry.h2
./compile trydrent.c >/dev/null 2>&1 \
&& cat direntry.h2 || cat direntry.h1
rm -f trydrent.o
formake '( ./compile trydrent.c >/dev/null 2>&1 \'
formake '&& cat direntry.h2 || cat direntry.h1 ) > direntry.h'
formake 'rm -f trydrent.o'
