
# call from kernel build system

EXTRA_CFLAGS = -I$(M)/../include -I$(M)/../include/osa -DKERNEL_LEVEL -DDEBUG -O3
kernel_PATH=/home/rk3399/work/tina_r818/lichee/linux-4.9
memcache-objs := ../memcache_drv.o ../memcache_dev.o 
A15_TOOLCHAIN_PREFIX=aarch64-linux-gnu-
obj-m	:= memcache.o


PWD     := `pwd`
SUBDIRS := .. .
KER_MODULE := memcache

modules:
	@echo \# $(KER_MODULE): Building module
	$(MAKE) -C $(kernel_PATH) M=$(PWD) ARCH=arm64 CROSS_COMPILE=$(A15_TOOLCHAIN_PREFIX) modules

install:
	@echo \# $(KER_MODULE): Copying files to install directory
	$(MAKE) -C $(kernel_PATH) M=$(PWD) ARCH=arm64 CROSS_COMPILE=$(A15_TOOLCHAIN_PREFIX) modules_install
clean:
	@echo \# $(KER_MODULE): Deleting temporary files
	for i in $(SUBDIRS) ; do cd $$i; rm -rf *.ko *.o *~ core .depend .*.cmd *.mod.c .tmp_versions *.symvers *.order; cd -; done

depend:
	@echo \# $(KER_MODULE): Dependencies..

