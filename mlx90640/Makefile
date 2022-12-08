ARCH= arm64
MVTOOL_PREFIX= aarch64-linux-gnu-
#MVTOOL_PREFIX= aarch64-linux-
CROSS_COMPILE= $(MVTOOL_PREFIX)
KDIR := /home/rk3399/rk3399_linux/kernel
TARGET= mlx90640
EXEC = $(TARGET)
obj-m :=$(TARGET).o
PWD :=$(shell pwd)
all:
	$(MAKE) -C  $(KDIR) -w  M=$(PWD) modules
clean:
