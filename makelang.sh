# $Id$
test x"$1" = x && {
  echo "usage: $0 language"
  exit 1
}

test -f lang/"$1".text -a -f lang/"$1".sed || {
  echo "$0: language \"$1\" does not appear to exist."
  exit 1
}

(
  echo `sed -e 's/^.*-//' -e q VERSION` - This version identifier must be on line 1 and start in pos 1.
  sed -f lang/"$1".sed ezmlmrc.template
  cat lang/"$1".text
) > ezmlmrc."$1"
