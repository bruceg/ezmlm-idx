dependon tryulong32.c compile load uint32.h1 uint32.h2
( ./compile tryulong32.c && ./load tryulong32 && ./tryulong32 ) >/dev/null 2>&1 \
&& cat uint32.h2 || cat uint32.h1
rm -f tryulong32.o tryulong32
formake '( ( ./compile tryulong32.c && ./load tryulong32 && ./tryulong32 ) >/dev/null 2>&1 \'
formake '&& cat uint32.h2 || cat uint32.h1 ) > uint32.h'
formake 'rm -f tryulong32.o tryulong32'
