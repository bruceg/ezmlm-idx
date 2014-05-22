# [should have a bin/sh line and EZPATH added above by make]
#
# script to accept files in DIR/mod/pending. Use as:
# ezmlm-accept DIR file1 [file2 ...]
# where ``DIR'' is the list directory and ``file1'' is a file to be
# accepted. ``ezmlm-accept DIR DIR/mod/pending/*'' will accept all
# pending files. Files that are successfully sent to the list are
# deled. See man page for details.

EZSEND="${EZPATH}/ezmlm-send"
FATAL='ezmlm-accept: fatal:'
if [ ! -x "$EZSEND" ]; then
  echo "$FATAL please edit script to the correct ezmlm-send path"
  exit 100;
fi

DIR="$1"

if [ -z "$1" ] || [ -z "$2" ]; then
  echo "$FATAL usage: ezmlm-accept DIR file1 [file2 ...]"
  exit 100;
fi

while [ -n "$2" ]; do
  if [ -x "$2" ]; then
    $EZSEND $DIR < "$2" && rm -f "$2"
  fi
  shift
done
exit 0;
