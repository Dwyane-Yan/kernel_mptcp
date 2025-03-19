CONFIG_MODULE_SIG=n
obj-m += kernel_mptcp.o
KERNELBUILD :=/lib/modules/$(shell uname -r)/build
default:
	make -C $(KERNELBUILD) M=$(shell pwd) modules
clean:
	rm -rf *.o *.ko *.mod.c .*.cmd *.marker *.order *.symvers .tmp_versions *.mod
