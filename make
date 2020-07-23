#!/bin/bash

set -o errexit -o nounset -o pipefail

case "$1" in
  build)
    docker build -t fnf/shocker .
    ;;
  run)
    docker run -v $(pwd):/root --privileged -it fnf/shocker bash
    ;;
  compile)
    gcc shocker.c -o shocker
    ;;
  *)
    echo "Invalid command: $1"
    ;;
esac


