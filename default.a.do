if test -r "$2=l"
then
  dependon "$2=l"
  dependon makelib `cat "$2=l"`
  directtarget
  formake ./makelib $1 `cat "$2=l"`
  ./makelib $1 `cat "$2=l"`
else
  nosuchtarget
fi
