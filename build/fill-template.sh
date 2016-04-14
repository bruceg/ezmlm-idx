VERSION=$( sed -e 's/^.*-//' -e q VERSION )
sed -e "s/@VERSION@/$VERSION/" <$1.template >$1
