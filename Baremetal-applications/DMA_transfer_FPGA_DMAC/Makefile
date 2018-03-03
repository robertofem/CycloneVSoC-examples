#
# Copyright Altera 2013,2014
# All Rights Reserved.
#

SOCEDS_ROOT ?= $(SOCEDS_DEST_ROOT)
HWLIBS_ROOT = $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib

# look for SoC EDS 15.0 directory structure
ifneq (,$(wildcard $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib/include/soc_cv_av/socal))
SOCAL_ROOT := $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib/include/soc_cv_av
endif

# look for SoC EDS 14.0 directory structure
ifneq (,$(wildcard $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib/include/socal))
SOCAL_ROOT := $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib/include
endif

# this will cahnge for system to system.  just needed to make the mkimage wrapper.  If you are using it after uboot it is not needed.
MKI := mkimage

ifndef SOCAL_ROOT
$(error SOCAL_ROOT is undefined)
endif

EXAMPLE_SRC := dma_demo.c io.c arm_cache_modified.s alt_address_space.c fpga_dmac_api.c
C_SRC       := $(EXAMPLE_SRC)

LINKER_SCRIPT := cycloneV-dk-ram-modified.ld

MULTILIBFLAGS := -mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon
CFLAGS  := -g -O0 -Wall -Werror -std=c99 $(MULTILIBFLAGS) -I$(HWLIBS_ROOT)/include -I. -Imsgdma -Iqsys_headers -I$(SOCAL_ROOT)
LDFLAGS := -T$(LINKER_SCRIPT) $(MULTILIBFLAGS)

CROSS_COMPILE := arm-altera-eabi-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)g++
NM := $(CROSS_COMPILE)nm
OC := $(CROSS_COMPILE)objcopy
OD := $(CROSS_COMPILE)objdump
RM := rm -rf
CP := cp -f

ELF ?= $(basename $(firstword $(C_SRC))).axf
OBJ := $(patsubst %.c,%.o,$(C_SRC))
#BIN = $(basename $(firstword $(C_SRC))).bin
BIN = baremetalapp.bin

.PHONY: all
all: $(BIN)

.PHONY: clean
clean:
	$(RM) $(ELF) $(OBJ) $(BIN)
	$(RM) *.map
	$(RM) *.objdump

%.c: $(HWLIBS_ROOT)/src/hwmgr/%.c
	$(CP) $< $@

$(OBJ): %.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@
	$(NM) $@ > $@.map
	$(OD) -d $@ > $@.objdump

$(BIN): $(ELF)
	$(OC) -O binary $(ELF) $(BIN)
	$(MKI) -A arm -O u-boot -T standalone -C none -a 0x00100040 -e 0x00100040 -n "baremetalapp" -d $(BIN)  $(BIN).img
