dependon auto-str conf-bin
formake './auto-str auto_bin `head -1 conf-bin` > auto_bin.c'
./auto-str auto_bin `head -1 conf-bin`
