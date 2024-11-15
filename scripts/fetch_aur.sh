#!/usr/bin/env bash

set -Eeuo pipefail

AUR_URL='https://aur.archlinux.org/'

fetch_pkg_list () {
  curl https://aur.archlinux.org/packages.gz | gzip -cd
}

fetch_pkg_list > packages.txt
