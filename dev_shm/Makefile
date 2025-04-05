# Minimal Makefile for building the kernel module

# List your module object file(s) here. The name should match your source file name.
obj-m += myshm.o

# $(PWD) is the current directory (hw/). So $(PWD)/../include points to the include directory.
EXTRA_CFLAGS += -I$(PWD)/../include

# KDIR points to the kernel build directory.
KDIR := /lib/modules/$(shell uname -r)/build

# PWD is the current directory where your module source and Makefile reside.
PWD := $(shell pwd)

# The default target builds the module by invoking the kernel build system.
default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Clean target to remove generated files.
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean