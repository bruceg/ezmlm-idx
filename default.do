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
  lang/*/ezmlmrc)
    lang=${1#lang/}
    lang=${lang%/ezmlmrc}
    dependon makelang VERSION ezmlmrc.template lang/$lang/sed
    formake "./makelang $lang"
    ;;
  lib/auto_version.c)
    dependon auto-str VERSION
    formake './auto-str auto_version < VERSION > lib/auto_version.c'
    ;;
  lib/auto_bin.c|lib/auto_lib.c|lib/auto_etc.c)
    base=${1%.c}
    base=${base##*_}
    ubase=`echo $base | tr a-z A-Z`
    dependon auto-str conf-$base
    formake "./auto-str auto_${base} EZMLM_${ubase} <conf-${base} >$1"
    ;;
  lib/auto_*.c)
    base=${1%.c}
    base=${base##*_}
    dependon auto-str conf-$base
    formake "./auto-str auto_${base} <conf-${base} >$1"
    ;;
  *)
    nosuchtarget
    ;;
esac
