#Name of the module
obj-m := alloc_dmable_buffer.o
#Files composing the module
alloc_dmable_buffer-objs :=  alloc_dmable_buffer_LKM.o alt_address_space.o

#guest architecture
ARCH := arm

#compiler
CROSS_COMPILE := ~/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/sysroots/x86_64-linux/usr/bin/armv7ahf-vfp-neon-angstrom-linux-gnueabi/arm-angstrom-linux-gnueabi-

#path to the compiled kernel
ROOTDIR := ~/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git

MAKEARCH := $(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)

all: modules
modules:
	$(MAKEARCH) -C $(ROOTDIR) M=${shell pwd} modules
clean:
	$(MAKEARCH) -C $(ROOTDIR) M=${shell pwd} clean
