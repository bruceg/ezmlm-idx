dependon conf-cc conf-ld conf-ldso conf-sub warn-auto.sh
formake '( cat warn-auto.sh; \'
formake 'sub=`head -n 1 conf-sub` ; \'
formake 'echo CC=\'\''`head -n 1 conf-cc`\'\''; \'
formake 'echo LD=\'\''`head -n 1 conf-ld`\'\''; \'
formake 'echo LDSO=\'\''`head -n 1 conf-ldso`\'\''; \'
formake ') > auto-ccld.sh'
