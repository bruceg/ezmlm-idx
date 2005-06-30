dependon warn-auto.sh conf-qmail VERSION Makefile tests/*-*
formake '(cat warn-auto.sh; \'
formake 'echo VER=\"`head -n 1 VERSION`\"; \'
formake 'echo EZVER=\"`head -n 1 VERSION | sed -e "s/^.*\.//"`\"; \'
formake 'cat tests/*-* ) >ezmlm-test;'
formake 'chmod 755 ezmlm-test'
