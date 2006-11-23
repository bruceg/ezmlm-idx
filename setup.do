dependon it man install conf-bin conf-etc conf-lang conf-lib conf-man
dependon BIN ETC LIB MAN
formake './install "`head -n 1 conf-bin`" < BIN'
formake './install "`head -n 1 conf-etc`" < ETC'
formake './install "`head -n 1 conf-man`" < MAN'
formake './install "`head -n 1 conf-lib`" < LIB'
formake 'rm -f "`head -n 1 conf-etc`"/default'
formake 'ln -sf "`head -n 1 conf-lang`" "`head -n 1 conf-etc`"/default'
