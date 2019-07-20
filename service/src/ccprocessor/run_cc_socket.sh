#!/usr/bin/env bash

if [ -d "/p" ]; then
  cd /p/src/ccprocessor
else
  cd /app/ccprocessor
fi

#while true; do
  python3 websocket.py 127.0.0.2 # >> /var/log/websocket.log 2>&1

#done
