dependon it man install conf-bin conf-etc conf-lang conf-man BIN ETC MAN
formake './install "`head -n 1 conf-bin`" < BIN'
formake './install "`head -n 1 conf-etc`" < ETC'
formake './install "`head -n 1 conf-man`" < MAN'
formake 'rm -f "`head -n 1 conf-etc`"/default'
formake 'ln -sf "`head -n 1 conf-lang`" "`head -n 1 conf-etc`"/default'
