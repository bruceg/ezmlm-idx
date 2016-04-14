dependon('compile','load','sys/trylib.c')
formake('( ( ./compile sys/trylib.c && ./load sys/trylib -l{base} ) >/dev/null 2>&1 && echo -l{base} || exit 0 ) >{target}')
formake('rm -f sys/trylib.o sys/trylib')
