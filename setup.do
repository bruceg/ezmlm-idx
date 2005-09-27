dependon it man install conf-bin conf-etc conf-man BIN ETC MAN
formake './install "`head -n 1 conf-bin`" < BIN'
formake './install "`head -n 1 conf-etc`" < ETC'
formake './install "`head -n 1 conf-man`" < MAN'
