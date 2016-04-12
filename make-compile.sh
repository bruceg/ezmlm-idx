echo 'source=$1; shift'
echo 'base=`echo "$source" | sed -e s:\\\\.c$::`'
echo exec "$CC" '-I. -Ilib -o "$base".o -c "$source" ${1+"$@"}'
