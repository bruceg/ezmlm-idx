if test -r $2=m
then
  dependon $2=m $2.s
  directtarget
  formake as -o $1 $2.s
  exit 0
fi
depend -$2=m

directtarget
dependon compile
dependcc $2.c
formake ./compile $2.c
