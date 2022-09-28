KERNELINCLUDE ?= /lib/modules/$(shell  uname -r)/build


oops-objs :=ramdisk.o

KBUILD_CFLAGS += -g -O0

obj-m := ramdisk.o


all:
	$(MAKE) -C  $(KERNELINCLUDE)   SUBDIRS=$(PWD)  modules;

install: 
	$(MAKE) -C  $(KERNELINCLUDE)   SUBDIRS=$(PWD)  modules_install;

clean:
	$(MAKE) -C  $(KERNELINCLUDE)   SUBDIRS=$(PWD)  clean;
	rm -f *.ko
    
