ARCH=arm64
#MVTOOL_PREFIX= /home/rk3399/work/SSR920/gcc-sigmastar-9.1.0-2020.07-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc
MVTOOL_PREFIX=/home/rk3399/work/suvan_test/build/toolchain-sunxi-glibc/toolchain/bin/aarch64-openwrt-linux-gcc
CROSS_COMPILE= $(MVTOOL_PREFIX)
ENV:=ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
#KDIR := /home/rk3399/work/SSR920/kernel
KDIR := /home/rk3399/work/tina_r818/lichee/linux-4.9
TARGET=sysfs
#TARGET= chardev_test
EXEC = $(TARGET)
ccflags-y = -DDEBUG -g 
obj-m :=$(TARGET).o
PWD :=$(shell pwd)
all:
	$(MAKE) $(ENV)  -C $(KDIR)  M=$(PWD) modules
clean:
	rm *o  
