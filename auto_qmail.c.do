dependon auto-str conf-qmail
formake './auto-str auto_qmail `head -1 conf-qmail` > auto_qmail.c'
./auto-str auto_qmail `head -1 conf-qmail`
