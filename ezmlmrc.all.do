ls -1 text | while read lang; do
  dependon ezmlmrc.$lang
done
