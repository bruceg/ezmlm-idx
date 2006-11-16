if test -r $1=x
then
  libs=""
  libscat=""
  dashl=""
  objs=""
  while read line
  do
    case $line in
    *.lib)
      libs="$libs $line"
      libscat="$libscat "'`'"cat $line"'`'
      ;;
    -l*)
      dashl="$dashl $line"
      libs="$libs `echo $line | sed -e 's/^-l/lib/' -e 's/$/.a/'`"
      ;;
    *)
      objs="$objs $line"
      ;;
    esac
  done < "$1=x"
  dependon load $1.o $objs $libs
  directtarget
  formake ./load $1 $objs $dashl "$libscat"
  exit 0
fi

if test -r $1=s
then
  dependon warn-auto.sh $1.sh
  formake cat warn-auto.sh $1.sh '>' $1
  formake chmod 755 $1
  exit 0
fi

case "$1" in
  shar)
    dependon FILES `cat FILES`
    formake 'shar -m `cat FILES` > shar'
    formake 'chmod 400 shar'
    ;;
  compile|load|makelib|makeso)
    dependon make-$1 warn-auto.sh systype
    formake "( cat warn-auto.sh; ./make-$1 "'"`cat systype`"'" ) > $1"
    formake "chmod 755 $1"
    ;;
  make-compile|make-load|make-makelib|make-makeso)
    dependon $1.sh auto-ccld.sh
    formake "cat auto-ccld.sh $1.sh > $1"
    formake "chmod 755 $1"
    ;;
  systype)
    dependon find-systype trycpp.c
    formake './find-systype > systype'
    ;;
  find-systype)
    dependon find-systype.sh auto-ccld.sh
    formake 'cat auto-ccld.sh find-systype.sh > find-systype'
    formake 'chmod 755 find-systype'
    ;;
  auto-ccld.sh)
    dependon conf-cc conf-ld conf-ldso conf-sub warn-auto.sh
    formake '( cat warn-auto.sh; \'
    formake 'sub=`head -n 1 conf-sub` ; \'
    formake 'echo CC=\'\''`head -n 1 conf-cc`\'\''; \'
    formake 'echo LD=\'\''`head -n 1 conf-ld`\'\''; \'
    formake 'echo LDSO=\'\''`head -n 1 conf-ldso`\'\''; \'
    formake ') > auto-ccld.sh'
    ;;
  ezmlm-mktab)
    dependon conf-sub
    formake "rm -f $1"
    formake 'sub=`head -n 1 conf-sub` ; ln sub_$$sub/'$1 $1
    formake "touch $1"
    ;;
  lang/*/ezmlmrc)
    lang=${1#lang/}
    lang=${lang%/ezmlmrc}
    dependon makelang VERSION ezmlmrc.template lang/$lang/sed
    formake "./makelang $lang"
    ;;
  *)
    nosuchtarget
    ;;
esac
