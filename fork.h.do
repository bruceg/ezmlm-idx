dependon compile load tryvfork.c fork.h1 fork.h2
formake '( ( ./compile tryvfork.c && ./load tryvfork ) >/dev/null 2>&1 \'
formake '&& cat fork.h2 || cat fork.h1 ) > fork.h'
formake 'rm -f tryvfork.o tryvfork'
