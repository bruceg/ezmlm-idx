dependon it man install conf-bin conf-man BIN MAN
formake './install "`head -1 conf-bin`" < BIN'
formake './install "`head -1 conf-man`" < MAN'
./install "`head -1 conf-bin`" < BIN
./install "`head -1 conf-man`" < MAN
