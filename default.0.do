for section in 1 2 3 4 5 6 7 8; do
  if test -r "$2.$section"
  then
    dependon "$2.$section"
    formake nroff -man "$2.$section" '>' $1
    exit 0
  fi
done
nosuchtarget
