#!/bin/bash

set -ex

series=xenial

#juju add-machine -n 3

# maas:
# juju add-machine $1
# juju add-machine $2
# juju add-machine $3

function get_internal_ip
{
    juju_host=$1
    juju ssh $juju_host ip -4 -o addr show | grep -v '\blo\b' | grep -v 'br0' | cut -d ' ' -f 9
}

function setup_nfs_client
{
    juju_host=$1
    host_name=host-$1
    host_ip=`get_internal_ip $1`
    nfs_host=$2
    nfshost_ip=$3

    juju ssh $juju_host sudo usermod -a -G lxd ubuntu || true
    juju ssh $juju_host sudo apt install -y zfsutils-linux lxd
    juju ssh $juju_host sudo lxd init --auto --storage-backend zfs \
         --storage-create-loop 10 --storage-pool lxd && juju ssh $juju_host sudo dpkg-reconfigure -p medium lxd || true
    
    juju ssh $juju_host lxc config set core.trust_password foo
    juju ssh $juju_host lxc config set core.https_address 0.0.0.0:8443

    juju ssh $nfs_host lxc remote remove $host_name || true
    juju ssh $nfs_host lxc remote add $host_name $host_ip --accept-certificate --password=foo
    if [ "$(lxc image list $host_name: | grep -c $series)" -eq "0" ]; then
        juju ssh $nfs_host lxc image copy ubuntu:$series $host_name: --alias $series --auto-update
    fi


    juju ssh $juju_host -- sudo apt update
    juju ssh $juju_host -- sudo apt install -y nfs-common
    juju ssh $juju_host -- sudo mkdir /nfs    
    juju ssh $juju_host -- sudo mount -t nfs $nfshost_ip:/home/ubuntu /nfs

}

IP_1=`get_internal_ip $1`

juju scp ./nfshost.sh $1:/home/ubuntu/nfshost.sh

juju ssh $1 /home/ubuntu/nfshost.sh


setup_nfs_client $2 $1 $IP_1
setup_nfs_client $3 $1 $IP_1


function check_nfs_on_container
{
    container=$1
    nfs_host=$2
    container_host=$3
    juju ssh $nfs_host lxc exec $container_host:$container -- cat /nfs/afile.txt
    juju ssh $nfs_host lxc exec $container_host:$container -- "echo '\nadded on $container' >> /nfs/afile.txt"
}

juju ssh $1 lxc launch host-$2:$series host-$2:container
juju ssh $1 lxc config device add container nfs disk source=/nfs path=/nfs

check_nfs_on_container container $1 host-$2
