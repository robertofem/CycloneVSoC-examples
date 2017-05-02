#Name of the module
obj-m := DMA_with_FPGA_module.o
#Files composing the module
DMA_with_FPGA_module-objs := DMA_with_FPGA.o PL330-functions.o alt_dma.o

#guest architecture
ARCH := arm

#compiler
CROSS_COMPILE := ~/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/sysroots/x86_64-linux/usr/bin/armv7ahf-vfp-neon-angstrom-linux-gnueabi/arm-angstrom-linux-gnueabi-

#path of the arm compiled kernel
ROOTDIR := ~/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git

MAKEARCH := $(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)

all: modules
modules:
	$(MAKEARCH) -C $(ROOTDIR) M=${shell pwd} modules

clean:
	$(MAKEARCH) -C $(ROOTDIR) M=${shell pwd} clean
