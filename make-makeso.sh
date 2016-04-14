LD=$( head -n 1 conf-ld )
LDSO=$( head -n 1 conf-ldso )
echo exec "$LD $LDSO" -L. -o '${1+"$@"}'
