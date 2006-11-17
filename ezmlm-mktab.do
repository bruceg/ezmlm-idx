dependon conf-sub
formake "rm -f $1"
formake 'sub=`head -n 1 conf-sub` ; ln sub_$$sub/'$1 $1
formake "touch $1"
