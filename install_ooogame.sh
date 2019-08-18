#!/bin/sh -e
if [ ! -d "dcf-game-infrastructure" ]; then
	git clone https://github.com/o-o-overflow/dcf-game-infrastructure.git
else
	cd dcf-game-infrastructure && git pull && cd -
fi
cd dcf-game-infrastructure && pip install -e . && cd -