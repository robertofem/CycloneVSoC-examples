################################################
#
# Makefile to Manage QuartusII/QSys Design
#
# Copyright Altera (c) 2013
# All Rights Reserved
#
################################################

SHELL := /bin/bash

################################################
# Tools

CAT := cat
CD := cd
CHMOD := chmod
CP := cp -rf
ECHO := echo
DATE := date
HEAD := head
MKDIR := mkdir -p
MV := mv
RM := rm -rf
SED := sed
TAR := tar
TOUCH := touch

# Helpful Macros
SPACE := $(empty) $(empty)

################################################


################################################
.PHONY: default
default: help
################################################

################################################
.PHONY: all
all: sof rbf preloader uboot dtb
################################################

################################################
# Target Stamping

ACDS_VERSION := $(shell quartus_cmd -version 2>/dev/null | grep Version | $(HEAD) -n1 | $(SED) -e 's,^Version[ \t]*\([0-9.]*\).*,\1,g' 2>/dev/null)

define get_stamp_dir
stamp$(if $(ACDS_VERSION),/$(ACDS_VERSION))
endef

define get_stamp_target
$(get_stamp_dir)$(if $1,/$1.stamp,$(error ERROR: Arg 1 missing to $0 function))
endef

define stamp_target
@$(MKDIR) $(@D)
@$(TOUCH) $@
endef

.PHONY: clean
clean:
	@$(ECHO) "Cleaning stamp files (which will trigger rebuild)"
	@$(RM) $(get_stamp_dir)
	@$(ECHO) " TIP: Use 'make scrub_clean' to get a deeper clean"
################################################


################################################
# Archiving & Cleaning your QuartusII/QSys Project

AR_TIMESTAMP := $(if $(ACDS_VERSION),$(subst .,_,$(ACDS_VERSION))_)$(subst $(SPACE),,$(shell $(DATE) +%m%d%Y_%k%M%S))

AR_DIR := tgz
AR_FILE := $(AR_DIR)/$(basename $(firstword $(wildcard *.qpf)))_$(AR_TIMESTAMP).tar.gz

PRELOADER_DIR := software/preloader

AR_REGEX := \
	Makefile ip readme.txt ds5 \
	altera_avalon* *.qpf *.qsf *.sdc *.v *.sv *.vhd *.qsys *.tcl *.stp *.sed quartus.ini \
	*.sof *.rbf *.sopcinfo *.jdi output_files \
	hps_isw_handoff */*.svd */synthesis/*.svd *.dts *.dtb *.xml \
	$(PRELOADER_DIR)/preloader-mkpimage.bin \
	$(PRELOADER_DIR)/uboot-socfpga/u-boot.img \
	$(PRELOADER_DIR)/uboot-socfpga/spl/u-boot-spl \
	$(PRELOADER_DIR)/uboot-socfpga/u-boot \
	$(PRELOADER_DIR)/preloader.ds \
	$(PRELOADER_DIR)/uboot.ds

AR_FILTER_OUT = %_tb.qsys
AR_FILES = $(filter-out $(AR_FILTER_OUT),$(wildcard $(AR_REGEX)))

CLEAN_FILES += $(filter-out $(AR_DIR) $(AR_FILES),$(wildcard *))
CLEAN_FILES += $(wildcard .qsys_edit)

HELP_TARGETS += tgz
tgz.HELP := Create a tarball with the barebones source files that comprise this design

.PHONY: tarball tgz
tarball tgz: $(AR_FILE)

$(AR_FILE):
	@$(MKDIR) $(@D)
	@$(if $(wildcard $(@D)/*.tar.gz),$(MKDIR) $(@D)/.archive;$(MV) $(@D)/*.tar.gz $(@D)/.archive)
	@$(ECHO) "Generating $@..."
	@$(TAR) -czf $@ $(AR_FILES)

HELP_TARGETS += scrub_clean
scrub_clean.HELP := Restore design to its barebones state

.PHONY: scrub scrub_clean
scrub scrub_clean:
	$(if $(strip $(CLEAN_FILES)),$(RM) $(CLEAN_FILES),@$(ECHO) "You're already as clean as it gets!") 

.PHONY: tgz_scrub_clean
tgz_scrub_clean:
	$(MAKE) tgz AR_FILE=$(AR_FILE)
	$(MAKE) -s scrub_clean
	$(TAR) -xzf $(AR_FILE)
################################################


################################################
# Build QuartusII/QSys Project
#

#############
# QSys
QSYS_FILE := $(firstword $(wildcard *top*.qsys) $(wildcard *main*.qsys) $(wildcard *.qsys))
QSYS_DEPS := $(wildcard *.qsys)
QSYS_BASE := $(basename $(QSYS_FILE))
QSYS_QIP := $(QSYS_BASE)/synthesis/$(QSYS_BASE).qip
QSYS_SOPCINFO := $(QSYS_BASE).sopcinfo
QSYS_STAMP := $(call get_stamp_target,qsys)

# Under cygwin, ensure TMP env variable is not a cygwin style path
# before calling ip-generate
ifneq ($(shell which cygwin1.dll 2>/dev/null),)
ifneq ($(shell which cygpath 2>/dev/null),)
SET_QSYS_GENERATE_ENV = TMP="$(shell cygpath -m "$(TMP)")"
endif
endif

.PHONY: qsys_compile
qsys_compile: $(QSYS_STAMP)

$(QSYS_QIP) : $(QSYS_STAMP)

$(QSYS_STAMP): $(QSYS_DEPS)
	@$(RM) $(QSYS_QIP) $(QSYS_SOPCINFO) $(QSYS_BASE)
	$(SET_QSYS_GENERATE_ENV) qsys-generate $(QSYS_FILE) --synthesis=VERILOG $(QSYS_GENERATE_ARGS)
	$(stamp_target)


HELP_TARGETS += qsys_edit
qsys_edit.HELP := Launch QSys GUI

.PHONY: qsys_edit
qsys_edit:
	qsys-edit $(QSYS_FILE) &



#############
# Quartus II

QUARTUS_QPF := $(firstword $(wildcard *.qpf))
QUARTUS_QSF := $(patsubst %.qpf,%.qsf,$(QUARTUS_QPF))
QUARTUS_BASE := $(basename $(QUARTUS_QPF))
QUARTUS_HDL_SOURCE := $(wildcard *.v *.sv *.vhd)
QUARTUS_MISC_SOURCE := $(wildcard *.stp *.sdc)

QUARTUS_PIN_ASSIGNMENTS_STAMP := $(call get_stamp_target,quartus_pin_assignments)
QUARTUS_DEPS = $(QUARTUS_QPF) $(QUARTUS_QSF) $(QUARTUS_HDL_SOURCE) $(QUARTUS_MISC_SOURCE) $(QSYS_STAMP) $(QSYS_QIP) $(QUARTUS_PIN_ASSIGNMENTS_STAMP)

QUARTUS_SOF := output_files/$(patsubst %.qpf,%.sof,$(QUARTUS_QPF))
QUARTUS_STAMP := $(call get_stamp_target,quartus)


.PHONY: quartus_compile
quartus_compile: $(QUARTUS_STAMP)

$(QUARTUS_SOF): $(QUARTUS_STAMP)

$(QUARTUS_PIN_ASSIGNMENTS_STAMP): $(QSYS_STAMP)
	@for pin_assign_tcl_file in $$(find $(QSYS_BASE) -name '*_pin_assignments.tcl'); do \
		quartus_map $(QUARTUS_QPF); \
		echo "Applying $${pin_assign_tcl_file} to $(QUARTUS_QPF)..."; \
		quartus_sta -t $${pin_assign_tcl_file} $(QUARTUS_QPF); \
	done
	$(stamp_target)

$(QUARTUS_STAMP): $(QUARTUS_DEPS)
	$(RM) $(QUARTUS_SOF) *.sof output_files
	quartus_stp $(QUARTUS_BASE)
	quartus_sh --flow compile $(QUARTUS_QPF)
	$(stamp_target)

HELP_TARGETS += quartus_edit
quartus_edit.HELP := Launch Quartus II GUI

.PHONY: quartus_edit
quartus_edit:
	quartus $(QUARTUS_QPF) &

HELP_TARGETS += sof
sof.HELP := QSys generate & Quartus compile this design

BATCH_TARGETS += sof

.PHONY: sof
sof: $(QUARTUS_SOF)


QUARTUS_RBF := $(patsubst %.sof,%.rbf,$(QUARTUS_SOF))
#
# This converts the sof into compressed, unencrypted 
# raw binary format corresponding to MSEL value of 8 
# in the FPGAMGRREGS_STAT register. If you read the 
# the whole register, it should be 0x50.
#
# CVSoC DevBoard SW1 MSEL should be set to up,down,up,down,up,up
#

$(QUARTUS_RBF): %.rbf: %.sof
	quartus_cpf -c -o bitstream_compression=on $< $@

.PHONY: rbf
rbf: $(QUARTUS_RBF)

.PHONY: create_rbf
create_rbf:
	quartus_cpf -c -o bitstream_compression=on $(QUARTUS_SOF) $(QUARTUS_RBF)

################################################


################################################
# QSYS/Quartus Project Generation
#  - we don't run this generation step automatically on purpose because they 
#    it will destroy any changes/customizations that you've made to your qsys 
#    or your quartus project
#
QSYS_QSYS_GEN := $(firstword $(wildcard create_*_qsys.tcl))
QUARTUS_QSF_QPF_GEN := $(firstword $(wildcard create_*_quartus.tcl))

.PHONY: quartus_generate_qsf_qpf
ifneq ($(QUARTUS_QSF_QPF_GEN),)
quartus_generate_qsf_qpf: $(QUARTUS_QSF_QPF_GEN)
	$(RM) $(QUARTUS_QSF) $(QUARTUS_QPF)
	quartus_sh --script=$<
else
quartus_generate_qsf_qpf:
	@$(ECHO) "Make target '$@' is not supported for this design"
endif

.PHONY: qsys_generate_qsys
ifneq ($(QSYS_QSYS_GEN),)
qsys_generate_qsys: $(QSYS_QSYS_GEN)
	$(RM) $(QSYS_FILE)
	qsys-script --script=$<
else
qsys_generate_qsys:
	@$(ECHO) "Make target '$@' is not supported for this design"
endif
################################################


################################################
# Quartus Programming
QUARTUS_PGM_STAMP := $(call get_stamp_target,quartus_pgm)

# set these for your board
# BOARD_CABLE =

# FPGA Board Device Index. Default to 2 since this is the most
#  common setting for dev board
# For SoCKIT board, this should be set to 1
BOARD_DEVICE_INDEX ?= 2

define quartus_pgm_sof
jtagconfig
quartus_pgm --mode=jtag $(if $(BOARD_CABLE),--cable="$(BOARD_CABLE)") --operation=p\;$1$(if $(BOARD_DEVICE_INDEX),"@$(BOARD_DEVICE_INDEX)")
jtagconfig $(if $(BOARD_CABLE),-c "$(BOARD_CABLE)") -n
endef

.PHONY: pgm
pgm: $(QUARTUS_PGM_STAMP)

$(QUARTUS_PGM_STAMP): $(QUARTUS_SOF)
	$(call quartus_pgm_sof,$<)
	$(stamp_target)

HELP_TARGETS += program_fpga
program_fpga.HELP := Quartus program sof to your attached dev board

.PHONY: program_fpga
program_fpga:
	$(call quartus_pgm_sof,$(QUARTUS_SOF))


# HPS Device Index. Default to 1 since this is the most
#  common setting for dev board
BOARD_HPS_DEVICE_INDEX ?= 1

define quartus_hps_pgm_qspi
jtagconfig
quartus_hps $(if $(BOARD_CABLE),--cable="$(BOARD_CABLE)") $(if $(BOARD_HPS_DEVICE_INDEX),--device=$(BOARD_HPS_DEVICE_INDEX)) --operation=PV $1
endef

HELP_TARGETS += program_qspi
program_qspi.HELP := Flash program preloader into QSPI Flash

.PHONY: program_qspi
program_qspi: $(PRELOADER_DIR)/preloader-mkpimage.bin
	$(call quartus_hps_pgm_qspi,$<)

################################################


################################################
# Preloader

QSYS_HPS_INST_NAME := hps_0

SBT.CREATE_SETTINGS := bsp-create-settings
SBT.GENERATE := bsp-generate-files

HELP_TARGETS += preloader
preloader.HELP := Build Preloader BSP for this design into $(PRELOADER_DIR) directory

PRELOADER_ID := hps_isw_handoff/$(QSYS_BASE)_$(QSYS_HPS_INST_NAME)/id
PRELOADER_DEPS := $(PRELOADER_ID)
PRELOADER_STAMP := $(call get_stamp_target,preloader)

$(PRELOADER_ID): $(QUARTUS_STAMP)

.PHONY: preloader
preloader: $(PRELOADER_STAMP)

# Create and build preloader with watchdog disabled.
# This is useful for board bring up and troubleshooting.
$(PRELOADER_STAMP): $(PRELOADER_DEPS)

	$(RM) $(PRELOADER_DIR)
	$(MKDIR) $(PRELOADER_DIR)

	$(SBT.CREATE_SETTINGS) \
		--type spl \
		--bsp-dir $(PRELOADER_DIR) \
		--preloader-settings-dir "../../hps_isw_handoff/$(QSYS_BASE)_$(QSYS_HPS_INST_NAME)" \
		--settings $(PRELOADER_DIR)/settings.bsp \
		--set spl.boot.WATCHDOG_ENABLE false

	$(MAKE) -C $(PRELOADER_DIR)

	$(stamp_target)


UBOOT_STAMP := $(call get_stamp_target,uboot)

$(UBOOT_STAMP):  $(PRELOADER_STAMP)
	$(MAKE) -C $(PRELOADER_DIR) uboot
	$(stamp_target)

HELP_TARGETS += uboot
uboot.HELP := Build U-Boot into $(PRELOADER_DIR) directory

.PHONY: uboot
uboot: $(UBOOT_STAMP)

################################################


################################################
# Device Tree Source (dts)
DEVICE_TREE_SOURCE := $(patsubst %.sopcinfo,%.dts,$(QSYS_SOPCINFO))

DTS.SOPC2DTS := sopc2dts

DTS.BOARDINFO := $(wildcard $(QSYS_BASE)_board_info.xml)
DTS.CLOCKINFO := hps_clock_info.xml

$(DEVICE_TREE_SOURCE): %.dts: %.sopcinfo $(DTS.BOARDINFO) $(DTS.CLOCKINFO)
ifeq ($(DTS.BOARDINFO),)
	$(warning WARNING: DTS BoardInfo file was not specified or found)
endif
	$(DTS.SOPC2DTS) --input $< --output $@ $(if $(DTS.BOARDINFO),--board $(DTS.BOARDINFO)) $(if $(DTS.CLOCKINFO),--board $(DTS.CLOCKINFO)) --bridge-removal all

HELP_TARGETS += dts
dts.HELP := Generate a device tree for this qsys design

.PHONY: dts
dts: $(DEVICE_TREE_SOURCE)


# Device Tree Blob (dtb)
DEVICE_TREE_BLOB := $(patsubst %.dts,%.dtb,$(DEVICE_TREE_SOURCE))

# device tree compiler
DTS.DTC := dtc

$(DEVICE_TREE_BLOB): %.dtb: %.dts
	$(DTS.DTC) -I dts -O dtb -o $@ $<

HELP_TARGETS += dtb
dtb.HELP := Generate a device tree blob for this qsys design

.PHONY: dtb
dtb: $(DEVICE_TREE_BLOB)

################################################
# Running Batch Jobs
ifneq ($(BATCH_TARGETS),)

BATCH_DIR := $(if $(TMP),$(TMP)/)batch/$(AR_TIMESTAMP)

.PHONY: $(patsubst %,batch-%,$(BATCH_TARGETS))
$(patsubst %,batch-%,$(BATCH_TARGETS)): batch-%: $(AR_FILE)
	@$(RM) $(BATCH_DIR)
	@$(MKDIR) $(BATCH_DIR)
	$(CP) $< $(BATCH_DIR) 
	$(CD) $(BATCH_DIR) && $(TAR) -xzf $(notdir $<) && $(CHMOD) -R 755 *
	$(MAKE) -C $(BATCH_DIR) $*
endif # BATCH_TARGETS != <empty>
################################################


################################################
# Help system

HELP_TARGETS += help
help.HELP := Displays this info (i.e. the available targets)

.PHONY: help
help: help-init help-targets help-fini

HELP_TARGETS_X := $(patsubst %,help-%,$(sort $(HELP_TARGETS)))
.PHONY: $(HELP_TARGETS_X)
help-targets: $(HELP_TARGETS_X)
$(HELP_TARGETS_X): help-%:
	@$(ECHO) "*********************"
	@$(ECHO) "* Target: $*"
	@$(ECHO) "*   $($*.HELP)"

.PHONY: help-init
help-init:
	@$(ECHO) "*****************************************"
	@$(ECHO) "*                                       *"
	@$(ECHO) "* Manage QuartusII/QSys design          *"
	@$(ECHO) "*                                       *"
	@$(ECHO) "*     Copyright (c) 2013                *"
	@$(ECHO) "*     All Rights Reserved               *"
	@$(ECHO) "*                                       *"
	@$(ECHO) "*****************************************"
	@$(ECHO) ""

.PHONY: help-fini
help-fini:
	@$(ECHO) "*********************"

################################################
