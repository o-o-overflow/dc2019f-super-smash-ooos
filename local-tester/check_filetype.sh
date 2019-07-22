#!/bin/bash -e


BASESRC="/app"
if [ ! -z "$DEBUG" ]; then
  BASESRC="../service/src"
fi

echo "[+] Testing wasm is still a WebAssembly file"
#file $BASESRC/sendcc.wasm | grep -q "WebAssembly"
hd /tmp/app/sendcc.wasm |head -10 > /tmp/app/sendcc.top
hd ${BASESRC}/sendcc.wasm | head -10 > /tmp/app/mod_sendcc.top

if ! cmp -s /tmp/app/sendcc.top /tmp/app/mod_sendcc.top; then
  exit 1
fi
rm /tmp/app/sendcc.top
rm /tmp/app/mod_sendcc.top


echo "[+] Testing support files invariance"
#for fn in ${BASESRC}/*.js; do

find ${BASESRC} -type f| grep -v "sendcc.wasm" | while read fn; do
  other_fp="/tmp/app${fn:${#BASESRC}}"

  if [[ -f ${fn} && -f ${other_fp} ]]; then
      if ! cmp -s "${fn}" "${other_fp}" ; then
          echo "[:(] invariant files changed $fn $other_fp"
          exit 2
      fi
  else
      echo "[:(] File does not exist, $fn or $other_fp"
      exit 3
  fi

done

echo "[+] Done testing"

