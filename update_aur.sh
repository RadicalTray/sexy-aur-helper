#!/usr/bin/env bash

set -Eeuo pipefail

if [ -z "$XDG_CACHE_HOME" ]; then
  echo '$XDG_CACHE_HOME is empty!'
  exit
fi

CACHE_DIR=$XDG_CACHE_HOME/sexy_aur_helper

if [ ! -d "$CACHE_DIR" ]; then
  echo "Creating $CACHE_DIR"
  mkdir "$CACHE_DIR"
fi

cd $CACHE_DIR

AUR_PKGS="$(pacman -Qemq)"

for pkg in $AUR_PKGS; do
  echo $pkg
done
