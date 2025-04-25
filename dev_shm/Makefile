# Minimal Makefile for building the kernel module

# List your module object file(s) here. The name should match your source file name.
TARGET = myshm
obj-m += ${TARGET}.o

# $(PWD) is the current directory (hw/). So $(PWD)/../include points to the include directory.
EXTRA_CFLAGS += -I$(PWD)/../include

# KDIR points to the kernel build directory.
KDIR := /lib/modules/$(shell uname -r)/build

# PWD is the current directory where your module source and Makefile reside.
PWD := $(shell pwd)
OUT_DIR := $(PWD)/output

# The default target builds the module by invoking the kernel build system.
default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	@mkdir -p $(OUT_DIR)
	@mv .*.cmd *.ko *.mod* modules.order Module.symvers .$(TARGET).o.d $(TARGET).o $(OUT_DIR)
	@echo "Module built and moved to $(OUT_DIR)"

# Clean target to remove generated files.
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	@rm -rf $(OUT_DIR)