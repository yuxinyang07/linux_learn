ARCH= arm64
MVTOOL_PREFIX= aarch64-linux-gnu-
#MVTOOL_PREFIX= aarch64-linux-
CROSS_COMPILE= $(MVTOOL_PREFIX)
#KDIR := /home/rk3399/rk3399_linux/kernel
KDIR := /home/rk3399/work/tina_r818/lichee/linux-4.9
TARGET= pin
EXEC = $(TARGET)
obj-m :=$(TARGET).o
PWD :=$(shell pwd)
all:
	$(MAKE) -C  $(KDIR) -w  M=$(PWD) modules
clean:
