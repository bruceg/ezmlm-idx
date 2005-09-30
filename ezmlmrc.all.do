ls -1 lang | while read lang; do
  dependon lang/${lang}/ezmlmrc
done
