cmd_/home/roberto/CycloneVSoC_Examples/DE1-SoC/Linux_Modules/DMA_PL330_LKM/PL330-functions.o := ~/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/sysroots/x86_64-linux/usr/bin/armv7ahf-vfp-neon-angstrom-linux-gnueabi/arm-angstrom-linux-gnueabi-gcc -Wp,-MD,/home/roberto/CycloneVSoC_Examples/DE1-SoC/Linux_Modules/DMA_PL330_LKM/.PL330-functions.o.d  -nostdinc -isystem /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/sysroots/x86_64-linux/usr/lib/armv7ahf-vfp-neon-angstrom-linux-gnueabi/gcc/arm-angstrom-linux-gnueabi/4.8.3/include -I/home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/include/uapi -Iinclude/generated/uapi -include /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -O2 -fno-dwarf2-cfi-asm -fno-omit-frame-pointer -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a -msoft-float -Uarm -Wframe-larger-than=1024 -fno-stack-protector -Wno-unused-but-set-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -pg -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -DCC_HAVE_ASM_GOTO  -DMODULE  -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(PL330_functions)"  -D"KBUILD_MODNAME=KBUILD_STR(DMA_with_FPGA_module)" -c -o /home/roberto/CycloneVSoC_Examples/DE1-SoC/Linux_Modules/DMA_PL330_LKM/PL330-functions.o /home/roberto/CycloneVSoC_Examples/DE1-SoC/Linux_Modules/DMA_PL330_LKM/PL330-functions.c

source_/home/roberto/CycloneVSoC_Examples/DE1-SoC/Linux_Modules/DMA_PL330_LKM/PL330-functions.o := /home/roberto/CycloneVSoC_Examples/DE1-SoC/Linux_Modules/DMA_PL330_LKM/PL330-functions.c

deps_/home/roberto/CycloneVSoC_Examples/DE1-SoC/Linux_Modules/DMA_PL330_LKM/PL330-functions.o := \
  include/linux/kernel.h \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/atomic/sleep.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/ring/buffer.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/sysroots/x86_64-linux/usr/lib/armv7ahf-vfp-neon-angstrom-linux-gnueabi/gcc/arm-angstrom-linux-gnueabi/4.8.3/include/stdarg.h \
  include/linux/linkage.h \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
    $(wildcard include/config/kprobes.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  include/linux/compiler-gcc4.h \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  include/linux/stringify.h \
  include/linux/export.h \
    $(wildcard include/config/have/underscore/symbol/prefix.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/arch/arm/include/asm/linkage.h \
  include/linux/stddef.h \
  include/uapi/linux/stddef.h \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  include/uapi/linux/types.h \
  arch/arm/include/generated/asm/types.h \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/include/uapi/asm-generic/types.h \
  include/asm-generic/int-ll64.h \
  include/uapi/asm-generic/int-ll64.h \
  arch/arm/include/generated/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/uapi/asm-generic/bitsperlong.h \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/include/uapi/linux/posix_types.h \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/arch/arm/include/uapi/asm/posix_types.h \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/include/uapi/asm-generic/posix_types.h \
  include/linux/bitops.h \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/arch/arm/include/asm/bitops.h \
    $(wildcard include/config/smp.h) \
  include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
  include/linux/typecheck.h \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/arch/arm/include/asm/irqflags.h \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/arm/thumb.h) \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/arch/arm/include/uapi/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/arch/arm/include/asm/hwcap.h \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/arch/arm/include/uapi/asm/hwcap.h \
  include/asm-generic/bitops/non-atomic.h \
  include/asm-generic/bitops/fls64.h \
  include/asm-generic/bitops/sched.h \
  include/asm-generic/bitops/hweight.h \
  include/asm-generic/bitops/arch_hweight.h \
  include/asm-generic/bitops/const_hweight.h \
  include/asm-generic/bitops/lock.h \
  include/asm-generic/bitops/le.h \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/arch/arm/include/uapi/asm/byteorder.h \
  include/linux/byteorder/little_endian.h \
  include/uapi/linux/byteorder/little_endian.h \
  include/linux/swab.h \
  include/uapi/linux/swab.h \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/arch/arm/include/asm/swab.h \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/arch/arm/include/uapi/asm/swab.h \
  include/linux/byteorder/generic.h \
  include/asm-generic/bitops/ext2-atomic-setbit.h \
  include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  include/linux/printk.h \
    $(wildcard include/config/early/printk.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/dynamic/debug.h) \
  include/linux/init.h \
    $(wildcard include/config/broken/rodata.h) \
  include/linux/kern_levels.h \
  include/linux/dynamic_debug.h \
  include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
  include/uapi/linux/string.h \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/arch/arm/include/asm/string.h \
  include/linux/errno.h \
  include/uapi/linux/errno.h \
  arch/arm/include/generated/asm/errno.h \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/include/uapi/asm-generic/errno.h \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/include/uapi/asm-generic/errno-base.h \
  include/uapi/linux/kernel.h \
  /home/roberto/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git/include/uapi/linux/sysinfo.h \
  /home/roberto/CycloneVSoC_Examples/DE1-SoC/Linux_Modules/DMA_PL330_LKM/alt_dma.h \
  /home/roberto/CycloneVSoC_Examples/DE1-SoC/Linux_Modules/DMA_PL330_LKM/hwlib_socal_linux.h \
  /home/roberto/CycloneVSoC_Examples/DE1-SoC/Linux_Modules/DMA_PL330_LKM/alt_dma_common.h \
  /home/roberto/CycloneVSoC_Examples/DE1-SoC/Linux_Modules/DMA_PL330_LKM/alt_dma_periph_cv_av.h \
  /home/roberto/CycloneVSoC_Examples/DE1-SoC/Linux_Modules/DMA_PL330_LKM/alt_dma_program.h \
  /home/roberto/CycloneVSoC_Examples/DE1-SoC/Linux_Modules/DMA_PL330_LKM/PL330-functions.h \

/home/roberto/CycloneVSoC_Examples/DE1-SoC/Linux_Modules/DMA_PL330_LKM/PL330-functions.o: $(deps_/home/roberto/CycloneVSoC_Examples/DE1-SoC/Linux_Modules/DMA_PL330_LKM/PL330-functions.o)

$(deps_/home/roberto/CycloneVSoC_Examples/DE1-SoC/Linux_Modules/DMA_PL330_LKM/PL330-functions.o):
