dependon conf-sub
formake 'sub=`head -n 1 conf-sub` ; head -n 1 sub_$$sub/conf-sqlld > sql.tmp'
formake 'mv sql.tmp sql.lib'
