if test -r "$2=so"
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
    -*)
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
  done < "$2=so"
  dependon makeso $objs $libs
  formake ./makeso $1 $objs $dashl "$libscat"
else
  nosuchtarget
fi
