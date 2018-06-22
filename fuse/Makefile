fuse_test: fuse_test.c
	gcc -D_FILE_OFFSET_BITS=64 -o fuse_test fuse_test.c -lfuse

.PHONY: mount
mount:
	mkdir -p test
	sudo mount -t fuse ./fuse_test test

.PHONY: clean
clean:
	-umount -l test
	-rm -rf test fuse_test
