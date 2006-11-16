echo 'source=$1; shift'
echo 'base=`echo "$source" | sed -e s:\\\\.c$::`'
echo exec "$CC" '-I. -o "$base".o -c "$source" ${1+"$@"}'
