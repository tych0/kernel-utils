CFLAGS=-D_FILE_OFFSET_BITS=64 -Werror -I/usr/include/fuse3
LDLIBS=-lfuse3

build: fuse main

.PHONY: check
check: fuse main
	modprobe fuse
	mkdir -p mount
	unshare -Urpm --fork --wd=$(CURDIR) ./main

clean:
	-rm -rf fuse main mount
