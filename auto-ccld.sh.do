dependon conf-cc conf-cclo conf-ld conf-ldso warn-auto.sh
formake '( cat warn-auto.sh; \'
formake 'echo CC=\'\''`head -n 1 conf-cc`\'\''; \'
formake 'echo CCLO=\'\''`head -n 1 conf-cclo`\'\''; \'
formake 'echo LD=\'\''`head -n 1 conf-ld`\'\''; \'
formake 'echo LDSO=\'\''`head -n 1 conf-ldso`\'\''; \'
formake ') > auto-ccld.sh'
