if test -r $1=x
then
  dependon $1=x
  libs=`grep '\.lib *$' "$1=x"`
  libscat=''
  for i in $libs
  do
    libscat="$libscat "'`'"cat $i"'`'
  done
  objs=`grep -v '\.lib *$' "$1=x"`
  dependon load $1.o $objs $libs
  directtarget
  formake ./load $1 $objs "$libscat"
  eval ./load $1 $objs $libscat
  exit 0
fi

if test -r $1=s
then
  dependon $1=s warn-auto.sh $1.sh
  formake cat warn-auto.sh $1.sh '>' $1
  formake chmod 755 $1
  cat warn-auto.sh $1.sh
  chmod 755 $3
  exit 0
fi

case "$1" in
  shar)
    dependon FILES `cat FILES`
    formake 'shar -m `cat FILES` > shar'
    formake 'chmod 400 shar'
    shar -m `cat FILES`
    chmod 400 $3
    ;;
  compile|load|makelib)
    dependon make-$1 warn-auto.sh systype
    formake "( cat warn-auto.sh; ./make-$1 "'"`cat systype`"'" ) > $1"
    formake "chmod 755 $1"
    cat warn-auto.sh
    ./make-$1 "`cat systype`"
    chmod 755 $3
    ;;
  make-compile|make-load|make-makelib)
    dependon $1.sh auto-ccld.sh
    formake "cat auto-ccld.sh $1.sh > $1"
    formake "chmod 755 $1"
    cat auto-ccld.sh $1.sh
    chmod 755 $3
    ;;
  systype)
    dependon find-systype trycpp.c
    formake './find-systype > systype'
    ./find-systype
    ;;
  find-systype)
    dependon find-systype.sh auto-ccld.sh
    formake 'cat auto-ccld.sh find-systype.sh > find-systype'
    formake 'chmod 755 find-systype'
    cat auto-ccld.sh find-systype.sh
    chmod 755 $3
    ;;
  auto-ccld.sh)
    dependon conf-cc conf-ld warn-auto.sh
    formake '( cat warn-auto.sh; \'
    formake 'echo CC=\'\''`head -1 conf-cc`\'\''; \'
    formake 'echo LD=\'\''`head -1 conf-ld`\'\'' \'
    formake ') > auto-ccld.sh'
    cat warn-auto.sh
    echo CC=\'`head -1 conf-cc`\'
    echo LD=\'`head -1 conf-ld`\'
    ;;
  *)
    nosuchtarget
    ;;
esac
