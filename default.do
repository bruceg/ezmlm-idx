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
      baselib=`echo $line | sed -e 's/^-l/lib/'`
      if test -e $baselib=l; then
        libs="$libs $baselib.a"
      fi
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

if test -r $1.template
then
  dependon $1.template VERSION fill-template
  formake "./fill-template $1"
  exit 0
fi

case "$1" in
  compile|libcompile|load|makelib|makeso)
    dependon make-$1 warn-auto.sh systype
    formake "( cat warn-auto.sh; ./make-$1 "'"`cat systype`"'" ) > $1"
    formake "chmod 755 $1"
    ;;
  make-*)
    dependon $1.sh auto-ccld.sh
    formake "cat auto-ccld.sh $1.sh > $1"
    formake "chmod 755 $1"
    ;;
  lang/*/ezmlmrc)
    lang=${1#lang/}
    lang=${lang%/ezmlmrc}
    dependon makelang ezmlmrc
    formake "./makelang $lang"
    ;;
  lang/*/text/messages)
    lang=${1#lang/}
    lang=${lang%%/*}
    dependon make-messages lang/$lang/messages
    formake "./make-messages < lang/$lang/messages > lang/$lang/text/messages"
    ;;
  auto_version.c)
    dependon auto-str VERSION
    formake './auto-str auto_version < VERSION > auto_version.c'
    ;;
  auto_bin.c|auto_lib.c|auto_etc.c)
    base=${1%.c}
    base=${base##*_}
    ubase=`echo $base | tr a-z A-Z`
    dependon auto-str conf-$base
    formake "./auto-str auto_${base} EZMLM_${ubase} <conf-${base} >$1"
    ;;
  auto_*.c)
    base=${1%.c}
    base=${base##*_}
    dependon auto-str conf-$base
    formake "./auto-str auto_${base} <conf-${base} >$1"
    ;;
  *)
    nosuchtarget
    ;;
esac
