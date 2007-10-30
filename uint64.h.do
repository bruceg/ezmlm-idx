dependon tryulong64.c compile load uint64.h1 uint64.h2
formake '( ( ./compile tryulong64.c && ./load tryulong64 && ./tryulong64 ) >/dev/null 2>&1 \'
formake '&& cat uint64.h2 || cat uint64.h1 ) > uint64.h'
formake 'rm -f tryulong64.o tryulong64'
