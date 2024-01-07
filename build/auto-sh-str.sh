echo -n "$1="
quote="'&'"
if [ -n "$2" ]; then
    quote="\${$2-'&'}"
fi
sed -e "1{" -e "s/'/'\\\\''/g" -e "s/.*/$quote/g" -e t -e "}" -e d
