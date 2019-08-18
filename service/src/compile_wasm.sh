#!/usr/bin/env bash

EMSDK_ENV="/tmp/emsdk/emsdk_env.sh"

if [[ -f ${EMSDK_ENV} ]]; then
  echo "Must have ${EMSDK_ENV} before emcc will work"
  exit 99
fi

source $EMSDK_ENV

emcc -O0 --bind -o sendcc.js wasm/main.cpp wasm/iso-8583/*.c -DSOCKK=4444 -s EXPORTED_FUNCTIONS='["_main"]' -s EXTRA_EXPORTED_RUNTIME_METHODS="['addFunction']" -s RESERVED_FUNCTION_POINTERS=2

sed 's/!flags.binary/false/g' -i sendcc.js

cp sendcc.* includes/

