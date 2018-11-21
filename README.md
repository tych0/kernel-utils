# kernel-utils

This is a collection of scripts I use to hack on kernel related stuff. I
publish them here mostly so I can sync them with all of my VMs, but perhaps
they will be useful to someone else.

# using libvirt

i am mostly lazy and use libvirt to check kernels. i should probably figure out
how to do this better, but:

    function vmcreate() {
      release=$2
      if [ -z "$release" ]; then
        release=bionic
      fi
      echo creating vm with $release
      uvt-kvm create --cpu 4 --memory 4000 --disk 30 --bridge virbr0 --packages avahi-daemon --password ubuntu --run-script-once ~/config/bin/vminit $1 release=$release
    }

	function vmsync() {
	  releases='(bionic)'
	  echo syncing vm release "release~$releases"
	  sudo uvt-simplestreams-libvirt sync --source http://cloud-images.ubuntu.com/daily "release~$releases" arch=amd64
	}

	function vmmount() {
	  mkdir -p /tmp/$1
	  sudo guestmount -o allow_other -i -a /var/lib/uvtool/libvirt/images/$1.qcow /tmp/$1
	}

    # needs CONFIG_DEBUG_INFO
	function ppdmesg() {
	  $HOME/packages/linux/scripts/decode_stacktrace.sh $HOME/packages/linux/vmlinux $HOME/packages/linux < $1
	}

with a vminit script that looks like:

    #!/bin/bash

    apt -y autoremove snapd lxd open-iscsi
    touch /etc/cloud/cloud-init.disabled

are all aliases I use regularly. Then I can do various things:

    vmcreate foo # create
	virsh console foo # attach to the serial console
	virsh destroy foo # stop the vm (not delete it)
	virsh start foo # start the vm
	uvt-kvm destroy foo # actually delete the vm (not just stop)

I keep my kernel trees in ~/packages/linux{1,2,3...}. I use the
make -jN bindeb-pkg target to create a debian package, and then I apply it
with the following applykernel script:

    #!/usr/bin/env bash

    set -ex

    # we don't use debug images in the guest, so let's not install them
    rm -f ~/packages/linux-image*dbg*deb || true

    # similarly for headers
    rm -f ~/packages/linux-headers*deb || true

    kernels=("$HOME/packages/*$(ls ~/packages/*deb -1t | head -n1 | tail -c 14)")
    if [ "${kernels}" == "$HOME/packages/*" ]; then
      echo "no kernels found" && exit 1
    fi
    echo "found kernels: ${kernels}"
    scp $kernels $1.local:/home/ubuntu
    needs_reboot=0
    ssh $1.local "DEBIAN_FRONTEND=noninteractive sudo -E dpkg -i *deb" && needs_reboot=1 || true
    ssh $1.local "rm -rf *deb"

    # as of bionic, we need to do some fiddling with netplan
    ssh $1.local "sudo sed -i -e 's/ens3/enp0s3/g' /etc/netplan/50-cloud-init.yaml"

    if [ "${needs_reboot}" -eq "1" ]; then
      ssh $1.local "sudo reboot" || true
      rm -rf ~/packages/*changes ~/packages/*deb ~/packages/*buildinfo
    fi


