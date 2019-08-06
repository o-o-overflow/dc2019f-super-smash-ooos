#!/bin/bash -e

git remote add template git@github.com:o-o-overflow/dc2019f-challenge-template 2>/dev/null || true
git fetch template
git rebase template/master
