ARCH=arm64
CURRENT_PATH=$(shell pwd)
MVTOOL_PREFIX=aarch64-linux-gnu-gcc
CROSS_COMPILE= $(MVTOOL_PREFIX)
ENV:=ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
#KDIR := /home/rk3399/work/SSR920/kernel
KDIR := /home/rk3399/work/tina_r818/lichee/linux-4.9
TARGET=dma_test
EXEC = $(TARGET)
ccflags-y = -DDEBUG -g 
obj-m :=$(TARGET).o
PWD :=$(shell pwd)
all:
	$(MAKE) $(ENV)  -C $(KDIR)  M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(CURRENT_PATH) clean 
