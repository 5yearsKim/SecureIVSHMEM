TARGET = ivshmem_uio_hook

KDIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
OUT_DIR = $(PWD)/output

obj-m += $(TARGET).o

all:
	make -C $(KDIR) M=$(PWD) modules
	@mkdir -p $(OUT_DIR)
	@mv .*.cmd *.ko *.mod* modules.order Module.symvers $(TARGET).o $(OUT_DIR)

clean:
	make -C $(KDIR) M=$(PWD) clean
	@rm -rf $(OUT_DIR)
