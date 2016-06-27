#!/bin/bash

set -ex

sudo apt install -y nfs-kernel-server

grep -q /home/ubuntu /etc/exports ||
    (
        cat <<EOF | sudo tee -a /etc/exports
/home/ubuntu *(rw,sync,no_subtree_check)
EOF
    )
sudo exportfs -a

echo "Written on host" >> /home/ubuntu/afile.txt
