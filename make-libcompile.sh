echo 'source=$1; shift'
echo 'base=`echo "$source" | sed -e s:\\\\.c$::`'
echo exec "$CC $CCLO" '-I. -Ilib -o "$base".lo -c "$source" ${1+"$@"}'
