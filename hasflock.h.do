dependon tryflock.c compile load
formake '( ( ./compile tryflock.c && ./load tryflock ) >/dev/null 2>&1 \'
formake '&& echo \#define HASFLOCK 1 || exit 0 ) > hasflock.h'
formake 'rm -f tryflock.o tryflock'
