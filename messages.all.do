for messages in lang/*/messages; do
  dependon ${messages%/messages}/text/messages
done
