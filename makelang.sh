test x"$1" = x && {
  echo "usage: $0 language"
  exit 1
}

test -d lang/"$1" -a -d lang/"$1"/text || {
  echo "$0: language \"$1\" does not appear to exist."
  exit 1
}

cp ezmlmrc lang/$1/ezmlmrc
