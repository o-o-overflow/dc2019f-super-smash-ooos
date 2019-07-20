#! /usr/bin/env bash

if [ -d "/p" ]; then
  cd /p/src/
else
  cd /app
fi

su - oooser -c $(pwd)/ccprocessor/run_cc_socket.sh &

while true; do

    #/usr/bin/timeout 5m npm start
    npm start

done