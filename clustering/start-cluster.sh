#!/bin/bash

set -x
set -e

LXD="${GOPATH}/bin/lxd"
LXC="${GOPATH}/bin/lxc"

n=3

for i in $(seq 1 $n); do
  LXD_DIR=/tmp/lxd$i screen -d -m -S lxd$i sudo -E "${LXD}" --group sudo
done

for i in $(seq 1 $n); do
  LXD_DIR=/tmp/lxd$i "${LXD}" waitready
  LXD_DIR=/tmp/lxd$i "${LXC}" config set core.trust_password foo
  addr=127.0.0.1:844$(($i + 2))  # start at 8443 and count up
  LXD_DIR=/tmp/lxd$i "${LXC}" config set core.https_address $addr

  "${LXC}" remote rm c$i || true
  "${LXC}" remote add c$i $addr --accept-certificate --password=foo
done
