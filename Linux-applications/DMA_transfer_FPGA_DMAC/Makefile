#
TARGET = DMA_transfer_FPGA_DMAC

#Compile with the toolchain from Angstrom 2013.12 compilation
#CROSS_COMPILE := ~/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/sysroots/x86_64-linux/usr/bin/armv7ahf-vfp-neon-angstrom-linux-gnueabi/arm-angstrom-linux-gnueabi-
#Compile from SOC EDS toolchain (from Altera Embedded Command Shell )
CROSS_COMPILE := arm-linux-gnueabihf-

CFLAGS = -g -Wall  -I ${SOCEDS_DEST_ROOT}/ip/altera/hps/altera_hps/hwlib/include
LDFLAGS =  -g -Wall
CC = $(CROSS_COMPILE)gcc
ARCH= arm

build: $(TARGET)

$(TARGET): DMA_transfer_FPGA_DMAC.o fpga_dmac_api.o
	$(CC) $(LDFLAGS)   $^ -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGET) *.a *.o *~
