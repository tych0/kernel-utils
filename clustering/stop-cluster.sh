#!/bin/bash

set -e
set -x

n=3

sudo killall lxd || true

for i in $(seq 1 $n); do
  sudo rm -rf /tmp/lxd$i || true
done
