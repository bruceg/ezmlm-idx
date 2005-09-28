dependon conf-lang ezmlmrc.all
formake 'rm -f ezmlmrc'
formake 'ln -s ezmlmrc."`head -n 1 conf-lang`" ezmlmrc'
