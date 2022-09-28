KVERS=$(shell uname -r)


# kernel modules

obj-m +=globlefifo_dev.o globlefifetest.o

EXTRA_CFLAGS=-g -O0

build: kernel_modules


kernel_modules:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules

clean:
	make -C /lib/modules/$(KVERS)build M=$(CURDIR) clean
