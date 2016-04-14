LD=$( head -n 1 conf-ld )
echo 'main="$1"; shift'
echo exec "$LD" '-o "$main" "$main".o -L. ${1+"$@"}'
