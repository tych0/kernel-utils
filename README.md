# kernel-utils

This is a collection of scripts I use to hack on kernel related stuff. I
publish them here mostly so I have somewhere to archive the scripts and little
test programs, but maybe they will be useful to someone else.

# testing kernels

I use Andy Lutomirski's very helpful [virtme](https://github.com/amluto/virtme)
tool to test kernels.

That automates most of the gory bits, except for getting a rootfs, for which I
abuse the docker hub to allow me to try multiple distros. I place my kernel
trees in `~/packages/linux$n`, and I have the below code in my .bashrc:

    VIRTME_ROOTFS=/tmp/virtme-rootfs
    function getrootfs() {
        arch=$1

        if [ -d "$VIRTME_ROOTFS/$arch" ]; then
            return
        fi

        mkdir -p "$VIRTME_ROOTFS"

        skopeo --insecure-policy copy docker://ubuntu:latest oci:/tmp/oci:ubuntu-$arch
        sudo umoci unpack --image /tmp/oci:ubuntu-$arch "$VIRTME_ROOTFS/$arch"

        # umoci makes things opaque to us
        sudo chmod 755 "$VIRTME_ROOTFS/$arch"
        actual_root="$VIRTME_ROOTFS/$arch/rootfs"
        sudo chown $USER:$USER "$actual_root"

        # ubuntu kernel images don't have a /lib/modules since they're not used to
        # having a kernel in them. let's fix that.
        sudo mkdir -p "$actual_root/lib/modules"

        # make somewhere for our home directory to be mounted
        sudo mkdir -p "$actual_root/home/tycho"
        sudo chown $USER:$USER "$actual_root/$HOME"

        # now install some extra packages
        sudo cp /etc/resolv.conf "$actual_root/etc/resolv.conf"
        sudo chroot "$actual_root" apt -y update
        sudo chroot "$actual_root" apt -y install iproute2 strace gcc make
    }

    function runkernel() {
        kdir=$1
        shift
        arch=$2
        shift

        if [ -z "$kdir" ]; then
            kdir=linux
        fi

        if [ -z "$arch" ]; then
            arch=x86_64
        fi

        getrootfs "$arch"
        actual_root="$VIRTME_ROOTFS/$arch/rootfs"

        virtme-run \
            --kdir "$HOME/packages/$kdir" \
            --root "$actual_root" --rw \
            --mods auto --arch "$arch" \
            --cwd "$actual_root/$PWD" \
            --term "" \
            --rwdir "$HOME=$HOME" \
            $@
    }

This allows me to do things like:

    runkernel
    runkernel linux2
    runkernel linux3 arm64
    runkernel linux4 arm64 --qmeu-opt ...

And I will end up in a rootfs for that arch with my home directory bind mounted
in, with the init shell's PWD set to where I started runkernel from in my home
directory. This can be useful for e.g. running the kernel's selftests from the
kernel tree you're trying to test.

The rootfses live in /tmp, because they're generally pretty small, so it's not
a huge deal to re-download them on boot (I don't boot much, I just suspend).
This also prevents them from getting too stale.
