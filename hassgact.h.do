dependon trysgact.c compile load
( ./compile trysgact.c && ./load trysgact ) >/dev/null 2>&1 \
&& echo \#define HASSIGACTION 1
rm -f trysgact.o trysgact
formake '( ( ./compile trysgact.c && ./load trysgact ) >/dev/null 2>&1 \'
formake '&& echo \#define HASSIGACTION 1 || exit 0 ) > hassgact.h'
formake 'rm -f trysgact.o trysgact'
