# container-utils

This is a collection of scripts I use to hack on container related stuff. I
publish them here mostly so I can sync them with all of my VMs, but perhaps
they will be useful to someone else.

## usage

    # initial setup (though it is idempotent, so you can run it on each e.g.
    # each boot)
    ./lxd-cr-setup

    # try to c/r for the first time
    ./lxd-cr

    # whoops, if it failed due to a criu bug. hack hack hack, make install criu
    # and then
    ./lxd-cr

    # whoops, it failed due to a liblxc bug. hack hack hack, make install
    # liblxc, then
    ./reload-liblxc
    ./lxd-cr

## CRIU test suite

To run the criu test suite, you need the stuff documented on their wiki, as
well as:

CONFIG_FANOTIFY=y
