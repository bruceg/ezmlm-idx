for f in ezmlm-mktab checktag.c issub.c logmsg.c opensql.c putsubs.c searchlog.c subscribe.c tagmsg.c
do
  ln sub_std/$f $f
  dependon $f
done
