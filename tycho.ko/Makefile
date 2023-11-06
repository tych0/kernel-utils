obj-m += tycho.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	insmod tycho.ko

clean:
	rmmod tycho
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
