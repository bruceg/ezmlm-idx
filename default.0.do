if test -r "$2=0"
then
  dependon "$2=0"
  dependon `cat "$2=0"`
  nroff -man `cat "$2=0"`
  formake nroff -man `cat "$2=0"` '>' $1
else
  nosuchtarget
fi
