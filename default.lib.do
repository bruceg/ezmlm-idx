dependon('compile','load','trylib.c')
formake('( ( ./compile trylib.c && ./load trylib -l{base} ) >/dev/null 2>&1 && echo -l{base} || exit 0 ) >{target}')
formake('rm -f trylib.o trylib')
