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
	  uvt-kvm create --cpu 4 --memory 4000 --disk 30 --bridge virbr0 --packages avahi-daemon --password ubuntu $1 release=$release
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

are all aliases I use regularly. Then I can do various things:

    vmcreate foo # create
	virsh console foo # attach to the serial console
	virsh destroy foo # stop the vm (not delete it)
	virsh start foo # start the vm
	uvt-kvm destroy foo # actually delete the vm (not just stop)
