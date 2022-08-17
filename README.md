# kernel-utils

This is a collection of scripts I use to hack on kernel related stuff. I
publish them here mostly so I have somewhere to archive the scripts and little
test programs, but maybe they will be useful to someone else.

# testing kernels

I use Andy Lutomirski's very helpful [virtme](https://github.com/amluto/virtme)
tool to test kernels.

That automates most of the gory bits, except for getting a rootfs, for which I
abuse the docker hub to allow me to try multiple distros. I place my kernel
trees in `~/packages/linux$n`, and I have the `bin/` dir here in my `$PATH`.

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
