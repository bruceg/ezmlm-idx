#!/bin/bash
# $Id$
trap 'rm -r ezmlm-0.53' EXIT
tar -xzf ../ezmlm-0.53.tar.gz
diff -u -X REPLACED -x '*.[ao]' -x '*~' -x '*.do' ezmlm-0.53 . \
| sed	\
	-e "s/^--- ezmlm-0.53\\//--- /" \
	-e "s/^+++ .\\//+++ /" \
	-e '/^Only in /d' >idx.patch
diffstat idx.patch
