@==================================================================
@==================================================================
@
@ arm_caches.s
@
@ Functions for setting up the caches on the ARM Cortex A9 MP Core
@ found in the Altera Cyclone V
@
@ Adapted from:
@ Cortex-A9 Embedded example		 - startup.s
@	C:\altera\13.1\embedded\ds-5\examples\Bare-metal_examples\DS-5Examples\startup_Cortex\
@ Cortex-A9 MP-Core Embedded exampl	 - startup.s
@	C:\altera\13.1\embedded\ds-5\examples\Bare-metal_examples\DS-5Examples\startup_Cortex-A9MPCore\
@ Hardware Library for Altera SoCs	 - alt_cache.c
@	C:\altera\13.1\embedded\ip\altera\hps\altera_hps\hwlib\src\hwmgr\
@
@ Bain Syrowik
@ bain.syrowik@mail.utoronto.ca
@ May 30, 2014
@
@==================================================================
@==================================================================

@-----------------CHANGES TO THE ORIGINAL FILE--------------------@
@arm_caches_modified.s was done from arm_caches.s from Legup people
@L1_NORMAL_111_11 constant was changed from its original value 
@ 0x00007c0e to 0x00017c0e. This change sets S bit in table 
@ descriptors definyng normal memory region attributes, making 
@ normal memory shareable (coherent) for ACP accesses. 
@-----------------CHANGES TO THE ORIGINAL FILE--------------------@

@==================================================================
@==================================================================
@ Constant Definitions
@==================================================================
@==================================================================

@ This is the memory-mapped address of the L2 cache controller (L2C-310) on the
@ Altera Cyclone V SoC
.equ L2CC_PL310, 0xfffef000



@==================================================================
@==================================================================
@ Data Section
@
@ This section includes the following data structures:
@	* Level 1 Translation Table
@==================================================================
@==================================================================

.data

L1_TTB:
	.align 14		@ L1 Translation table must be aligned to 16kB boundary
	.skip  0x00004000	@ L1 Translation table takes up 16kB (4k entries X 4bytes each)




@==================================================================
@==================================================================
@ Text Section
@
@ This section includes the following functions:
@	* enable_MMU()
@	* initialize_L2C()
@	* enable_L1_D_side_prefetch()
@	* enable_L2_prefetch_hint()
@	* enable_exclusive_caching()
@	* enable_SCU()
@	* enable_L2_speculative_linefill()
@	* enable_L1_D()
@	* enable_L1_I()
@	* enable_branch_prediction()
@	* enable_L2()
@	* enable_caches()
@==================================================================
@==================================================================

.text

@==================================================================
@==================================================================
@ Enable Everything
@==================================================================
@==================================================================




	//.align
	//.global main
	
//main:




	.align
	.global enable_all_caches
	.type enable_all_caches #function
enable_all_caches:
	PUSH	{lr}
	BL	enable_MMU
	BL	initialize_L2C
	BL	enable_L1_D_side_prefetch
	BL	enable_L2_prefetch_hint
	BL	enable_SCU
	BL	enable_caches
	POP	{pc}


@==================================================================
@==================================================================
@ Enable the Memory Management Unit
@==================================================================
@==================================================================

	.align
	.global enable_MMU
	.type enable_MMU #function
enable_MMU:
	PUSH	{r4, r5, r7, r9, r10, r11, lr}

@------------------------------------------------------------------
@ Disable L2CC in case it was left enabled from an earlier run
@ This does not need to be done from a cold reset
@------------------------------------------------------------------

	LDR     r0, =L2CC_PL310		@VA = PA
	@ disable L2  via control register = 0x0
	LDR     r1, =0x0
	STR     r1, [r0,#0x100]		@ control register at offset 0x100

@------------------------------------------------------------------
@ Disable caches, MMU and branch prediction in case they were left enabled from an earlier run
@ This does not need to be done from a cold reset
@------------------------------------------------------------------

	MRC     p15, 0, r0, c1, c0, 0       @ Read CP15 System Control register
	BIC     r0, r0, #(0x1 << 12)        @ Clear I bit 12 to disable I Cache
	BIC     r0, r0, #(0x1 <<  2)        @ Clear C bit  2 to disable D Cache
	BIC     r0, r0, #0x1                @ Clear M bit  0 to disable MMU
	BIC     r0, r0, #(0x1 << 11)        @ Clear Z bit 11 to disable branch prediction
	MCR     p15, 0, r0, c1, c0, 0       @ Write value back to CP15 System Control register


@------------------------------------------------------------------
@ Invalidate Data and Instruction TLBs and branch predictor
@------------------------------------------------------------------

	MOV     r0,#0
	MCR     p15, 0, r0, c8, c7, 0      @ I-TLB and D-TLB invalidation
	MCR     p15, 0, r0, c7, c5, 6      @ BPIALL - Invalidate entire branch predictor array


@------------------------------------------------------------------
@ Cache Invalidation code for Cortex-A9
@------------------------------------------------------------------

	@ Invalidate L1 Instruction Cache

	MRC     p15, 1, r0, c0, c0, 1      @ Read Cache Level ID Register (CLIDR)
	TST     r0, #0x3                   @ Harvard Cache?
	MOV     r0, #0                     @ SBZ
	MCRNE   p15, 0, r0, c7, c5, 0      @ ICIALLU - Invalidate instruction cache and flush branch target cache

	@ Invalidate Data/Unified Caches

	MRC     p15, 1, r0, c0, c0, 1      @ Read CLIDR
	ANDS    r3, r0, #0x07000000        @ Extract coherency level
	MOV     r3, r3, LSR #23            @ Total cache levels << 1
	BEQ     Finished                   @ If 0, no need to clean

	MOV     r10, #0                    @ R10 holds current cache level << 1
Loop1:  ADD     r2, r10, r10, LSR #1       @ R2 holds cache "Set" position
	MOV     r1, r0, LSR r2             @ Bottom 3 bits are the Cache-type for this level
	AND     r1, r1, #7                 @ Isolate those lower 3 bits
	CMP     r1, #2
	BLT     Skip                       @ No cache or only instruction cache at this level

	MCR     p15, 2, r10, c0, c0, 0     @ Write the Cache Size selection register
	ISB                                @ ISB to sync the change to the CacheSizeID reg
	MRC     p15, 1, r1, c0, c0, 0      @ Reads current Cache Size ID register
	AND     r2, r1, #7                 @ Extract the line length field
	ADD     r2, r2, #4                 @ Add 4 for the line length offset (log2 16 bytes)
	LDR     r4, =0x3FF
	ANDS    r4, r4, r1, LSR #3         @ R4 is the max number on the way size (right aligned)
	CLZ     r5, r4                     @ R5 is the bit position of the way size increment
	LDR     r7, =0x7FFF
	ANDS    r7, r7, r1, LSR #13        @ R7 is the max number of the index size (right aligned)

Loop2:  MOV     r9, r4                     @ R9 working copy of the max way size (right aligned)

Loop3:  ORR     r11, r10, r9, LSL r5       @ Factor in the Way number and cache number into R11
	ORR     r11, r11, r7, LSL r2       @ Factor in the Set number
	MCR     p15, 0, r11, c7, c6, 2     @ Invalidate by Set/Way
	SUBS    r9, r9, #1                 @ Decrement the Way number
	BGE     Loop3
	SUBS    r7, r7, #1                 @ Decrement the Set number
	BGE     Loop2
Skip:   ADD     r10, r10, #2               @ increment the cache number
	CMP     r3, r10
	BGT     Loop1

Finished:

@------------------------------------------------------------------
@ Clear Branch Prediction Array
@------------------------------------------------------------------
	MOV     r0, #0
	MCR     p15, 0, r0, c7, c5, 6      @ BPIALL - Invalidate entire branch predictor array


@------------------------------------------------------------------=
@ Cortex-A9 MMU Configuration
@ Set translation table base
@------------------------------------------------------------------=


	@ Cortex-A9 supports two translation tables
	@ Configure translation table base (TTB) control register cp15,c2
	@ to a value of all zeros, indicates we are using TTB register 0.

	MOV     r0,#0x0
	MCR     p15, 0, r0, c2, c0, 2

	@ write the address of our page table base to TTB register 0
	LDR     r0, =L1_TTB
	MOV     r1, #0x08	@ RGN=b01  (outer cacheable write-back cached, write allocate)
				@ S=0      (translation table walk to non-shared memory)
	ORR     r1,r1,#0x40	@ IRGN=b01 (inner cacheability for the translation table walk is Write-back Write-allocate)

	ORR     r0,r0,r1                    
	MCR     p15, 0, r0, c2, c0, 0


@------------------------------------------------------------------=
@ PAGE TABLE generation
@
@ Generate the page tables
@ Build a flat translation table for the whole address space.
@ ie: Create 4096 1MB sections from 0x000xxxxx to 0xFFFxxxxx
@
@ 31                 20 19  18  17  16 15  14   12 11 10  9  8     5   4    3 2   1 0
@ |section base address| 0  0  |nG| S |AP2|  TEX  |  AP | P | Domain | XN | C B | 1 0|
@
@ Bits[31:20]   - Top 12 bits of VA is pointer into table
@ nG[17]=0      - Non global, enables matching against ASID in the TLB when set.
@ S[16]=0       - Indicates normal memory is shared when set.
@ AP2[15]=0
@ AP[11:10]=11  - Configure for full read/write access in all modes
@ TEX[14:12]=
@ CB[3:2]=
@
@ IMPP[9]=0     - Ignored
@ Domain[5:8]=0 - Set all pages to use domain 0
@ XN[4]=0       - Execute never disabled
@ Bits[1:0]=10  - Indicate entry is a 1MB section
@------------------------------------------------------------------=

@ templates for device and normal memory regions
@ The suffix denotes _TEX[2:0]_CB
.equ L1_DEVICE_010_00,	0x00002c02	@ non-shareable device memory

.equ L1_NORMAL_000_10,	0x00000c0a	@ outer and inner write-through, no write-allocate
.equ L1_NORMAL_000_11,	0x00000c0e	@ outer and inner write-back, no write-allocate
.equ L1_NORMAL_001_11,	0x00001c0e	@ outer and inner write-back, write-allocate
.equ L1_NORMAL_101_01,	0x00005c06	@ outer and inner write-back, write-allocate
.equ L1_NORMAL_110_10,	0x00006c0a	@ outer and inner write-through, no write-allocate
.equ L1_NORMAL_111_11,	0x00017c0e	@ outer and inner write-back, no write-allocate
.equ L1_NORMAL_111_01,	0x00007c06	@ outer and inner write-back, outer no write-allocate, inner write allocate
.equ L1_NORMAL_101_11,	0x00005c0e	@ outer and inner write-back, outer write-allocate, inner no write-allocate
.equ L1_NORMAL_110_11,	0x00006c0e	@ outer write-through, inner write-back, no write-allocate
.equ L1_NORMAL_111_10,	0x00007c0a	@ outer and inner write-back, no write-allocate
// NOTE: 000_11 doesn't seem to work for some reason.
// 111_11 and 110_11 give best performance for CHStone + mandelbrot + dhrystone benchmarks
// -> it is important that the inner cache is write-back

	LDR     r0, =L1_TTB
	LDR     r1,=0xfff                   @ loop counter

	@ r0 contains the address of the translation table base
	@ r1 is loop counter
	@ r2 will be the level1 descriptor (bits 19:0 of each table entry)

	@ use loop counter to create 4096 individual table entries.
	@ this writes from address TTB_Base +
	@ offset 0x3FFC down to offset 0x0 in word steps (4 bytes)

// TODO: Make more accurate separation between cacheable and device memory
init_ttb_1:
	CMP	r1, #0x800
	LDRMI	r2, =L1_NORMAL_111_11	    @ set lower half of memory to normal mode
	LDRPL	r2, =L1_DEVICE_010_00	    @ set upper half of memory to device memory
	ORR     r3, r2, r1, LSL#20          @ R3 now contains full level1 descriptor to write
	STR     r3, [r0, r1, LSL#2]         @ Str table entry at TTB base + loopcount*4
	SUBS    r1, r1, #1                  @ Decrement loop counter
	BPL     init_ttb_1


@------------------------------------------------------------------=
@ Setup domain control register - Set all domains to master mode
@------------------------------------------------------------------=

	MRC     p15, 0, r0, c3, c0, 0      @ Read Domain Access Control Register
	LDR     r0, =0xFFFFFFFF            @ Initialize every domain entry to 0b11 (master)
	MCR     p15, 0, r0, c3, c0, 0      @ Write Domain Access Control Register



/*
@------------------------------------------------------------------=
@ Enable NEON/VFP
@------------------------------------------------------------------=
	MRC     p15, 0, r0, c1, c0, 2      @ Read Coprocessor Access Control Register (CPACR)
	ORR     r0, r0, #(0xF << 20)       @ Enable access to CP 10 & 11
	MCR     p15, 0, r0, c1, c0, 2      @ Write Coprocessor Access Control Register (CPACR)
	ISB

	@ Switch on the VFP and NEON hardware
	MOV     r0, #0x40000000
	VMSR    FPEXC, r0                  @ Write FPEXC register, EN bit set
*/






@------------------------------------------------------------------=
@ Enable MMU
@ Leave the caches disabled
@------------------------------------------------------------------=

	MRC     p15, 0, r0, c1, c0, 0      @ Read CP15 System Control register
	BIC     r0, r0, #(0x1 << 12)       @ Clear I bit 12 to disable I Cache
	BIC     r0, r0, #(0x1 <<  2)       @ Clear C bit  2 to disable D Cache
	BIC     r0, r0, #0x2               @ Clear A bit  1 to disable strict alignment fault checking
	ORR     r0, r0, #0x1               @ Set M bit 0 to enable MMU before scatter loading
	MCR     p15, 0, r0, c1, c0, 0      @ Write CP15 System Control register

@ Now the MMU is enabled, virtual to physical address translations will occur. This will affect the next
@ instruction fetch.
@
@ The two instructions currently in the ARM pipeline will have been fetched before the MMU was enabled.
@ The branch back to main is safe because the Virtual Address (VA) is the same as the Physical Address (PA)
@ (flat mapping) of this code that enables the MMU and performs the branch

	POP	{r4, r5, r7, r9, r10, r11, pc}	@ pop the lr into the pc




@==================================================================
@==================================================================
@ Initialization for L2 Cache Controller
@==================================================================
@==================================================================
	.align
	.global initialize_L2C
	.type   initialize_L2C, %function
initialize_L2C: 
@------------------------------------------------------------------
@ L2CC (PL310) Initialization
@------------------------------------------------------------------
	@ In this example PL310 PA = VA. The memory was marked as Device memory
	@ in previous stages when defining CORE0 private address space
	LDR     r0, =L2CC_PL310

	@ Disable L2 Cache controller just in case it is already on
	LDR     r1, =0x0
	STR     r1, [r0,#0x100]

	@ Set aux cntrl
	@ Way size = 64KB
	LDR     r1, =0x00060000
	ORR	r1, r1, #(1 << 29)	@ Instruction prefetch enable
	ORR	r1, r1, #(1 << 28)	@ Data prefetch enable
	STR     r1, [r0,#0x104]		@ auxilary control reg at offset 0x104

	@ Set tag RAM latency
	@ 1 cycle RAM write access latency
	@ 1 cycle RAM read access latency
	@ 1 cycle RAM setup latency
	LDR     r1, =0x00000000
	STR     r1, [r0,#0x108]		@ tag ram control reg at offset 0x108

	@ Set Data RAM latency
	@ 1 cycle RAM write access latency
	@ 2 cycles RAM read access latency
	@ 1 cycle RAM setup latency
	//LDR     r1, =0x00000010	@ Altera uses read latency 1, but 0 seems to work
	LDR     r1, =0x00000000
	STR     r1, [r0,#0x10C]		@ data ram control reg at offset 0x108

	@Cache maintenance - invalidate by way (0xff) - base offset 0x77C
	LDR     r1, =0xFF
	STR     r1, [r0,#0x77C]		@ invalidate by way register at offset 0x77C
poll_invalidate:
	LDR     r1, [r0,#0x77C]		@ invalidate by way register at offset 0x77C
	TST     r1, #1
	BNE     poll_invalidate

//*
	@ Enable Event Counter Control Register. Reset counter 0 and 1 values
	LDR     r1, =0x007
	STR     r1, [r0,#0x200]

	@ Counter 1. Count Drhit event
	//LDR     r1, =0x008
	LDR     r1, =0b00001000 // DRHIT
	STR     r1, [r0,#0x204]

	@ Counter 0. Count Dwhit event
	//LDR     r1, =0x010
	LDR     r1, =0b00001100 // DRREQ
	STR     r1, [r0,#0x208]
//*/

	@ Ensure L2 remains disabled for the time being
	LDR     r1, =0x0
	STR     r1, [r0,#0x100]

	BX	lr



@==================================================================
@==================================================================
@ Enable caches and branch prediction
@==================================================================
@==================================================================
	.align
	.global enable_caches
	.type enable_caches #function
enable_caches:
	MRC     p15, 0, r0, c1, c0, 0      @ Read System Control Register
	ORR     r0, r0, #(0x1 << 12)       @ Set I bit 12 to enable I Cache
	ORR     r0, r0, #(0x1 << 2)        @ Set C bit  2 to enable D Cache
	ORR     r0, r0, #(0x1 << 11)       @ Set Z bit 11 to enable branch prediction
	MCR     p15, 0, r0, c1, c0, 0      @ Write System Control Register

	@ enable L2 cache
	LDR     r0, =L2CC_PL310
	LDR     r1, =0x1
	STR     r1, [r0,#0x100]

	BX		lr


@==================================================================
@==================================================================
@ Enable L1 D cache
@==================================================================
@==================================================================
	.align
	.global enable_L1_D
	.type enable_L1_D #function
enable_L1_D:
	MRC     p15, 0, r0, c1, c0, 0      @ Read System Control Register
	ORR     r0, r0, #(0x1 << 2)        @ Set C bit  2 to enable D Cache
	MCR     p15, 0, r0, c1, c0, 0      @ Write System Control Register

	BX		lr


@==================================================================
@==================================================================
@ Enable L1 I cache
@==================================================================
@==================================================================
	.align
	.global enable_L1_I
	.type enable_L1_I #function
enable_L1_I:
	MRC     p15, 0, r0, c1, c0, 0      @ Read System Control Register
	ORR     r0, r0, #(0x1 << 12)       @ Set I bit 12 to enable I Cache
	MCR     p15, 0, r0, c1, c0, 0      @ Write System Control Register

	BX		lr


@==================================================================
@==================================================================
@ Enable Branch Prediction
@==================================================================
@==================================================================
	.align
	.global enable_branch_prediction
	.type enable_branch_prediction #function
enable_branch_prediction:
	MRC     p15, 0, r0, c1, c0, 0      @ Read System Control Register
	ORR     r0, r0, #(0x1 << 11)       @ Set Z bit 11 to enable branch prediction
	MCR     p15, 0, r0, c1, c0, 0      @ Write System Control Register

	BX		lr


@==================================================================
@==================================================================
@ Enable L2 Cache
@==================================================================
@==================================================================
	.align
	.global enable_L2
	.type enable_L2 #function
enable_L2:
	LDR     r0, =L2CC_PL310
	LDR     r1, =0x1
	STR     r1, [r0,#0x100]

	BX		lr


@==================================================================
@==================================================================
@ Enable Store buffer device limitation
@==================================================================
@==================================================================
	.align
	.global enable_store_buffer_device_limitation
	.type enable_store_buffer_device_limitation #function
enable_store_buffer_device_limitation:
	@ disable L2C-310
	LDR     r0, =L2CC_PL310
	LDR     r1, =0x0
	STR     r1, [r0,#0x100]

	LDR     r1, [r0,#0x104]		@ auxilary control reg at offset 0x104
	ORR     r1, r1, #(0x1 << 11)    @ set bit 11 in aux. control reg to enable store buffer device limitation
	STR     r1, [r0,#0x104]		@ auxilary control reg at offset 0x104

	@ enable L2C-310
	LDR     r1, =0x1
	STR     r1, [r0,#0x100]

	BX		lr


@==================================================================
@==================================================================
@ Enable Early BRESP
@==================================================================
@==================================================================
	.align
	.global enable_early_BRESP
	.type enable_early_BRESP #function
enable_early_BRESP:
	@ disable L2C-310
	LDR     r0, =L2CC_PL310
	LDR     r1, =0x0
	STR     r1, [r0,#0x100]

	LDR     r1, [r0,#0x104]		@ auxilary control reg at offset 0x104
	ORR     r1, r1, #(0x1 << 30)    @ set bit 30 in aux. control reg to enable early BRESP
	STR     r1, [r0,#0x104]		@ auxilary control reg at offset 0x104

	@ enable L2C-310
	LDR     r1, =0x1
	STR     r1, [r0,#0x100]

	BX		lr


@==================================================================
@==================================================================
@ Enable L2 to write a full line of 0
@==================================================================
@==================================================================
	.align
	.global enable_write_full_line_zeros
	.type enable_write_full_line_zeros #function
enable_write_full_line_zeros:
	@ disable L2C-310
	LDR     r0, =L2CC_PL310
	LDR     r1, =0x0
	STR     r1, [r0,#0x100]

	LDR     r1, [r0,#0x104]		@ auxilary control reg at offset 0x104
	ORR     r1, r1, #(0x1 << 30)    @ set bit 0 in aux. control reg to enable write full line of 0's
	STR     r1, [r0,#0x104]		@ auxilary control reg at offset 0x104

	@ enable L2C-310
	LDR     r1, =0x1
	STR     r1, [r0,#0x100]

	MRC     p15, 0, r0, c1, c0, 1      @ Read processor's Auxiliary Control Register
	ORR     r0, r0, #(0x1 << 3)        @ enable write full line of zeros
	MCR     p15, 0, r0, c1, c0, 1      @ Write processor's Auxiliary Control Register

	BX		lr


@==================================================================
@==================================================================
@ Enable L1 D-side prefetch (A9 specific)
@==================================================================
@==================================================================
	.align
	.global enable_L1_D_side_prefetch
	.type enable_L1_D_side_prefetch, %function
enable_L1_D_side_prefetch:
	MRC     p15, 0, r0, c1, c0, 1      @ Read Auxiliary Control Register
	ORR     r0, r0, #(0x1 << 2)        @ Set DP bit 2 to enable L1 Dside prefetch
	MCR     p15, 0, r0, c1, c0, 1      @ Write Auxiliary Control Register

	BX      lr


@==================================================================
@==================================================================
@ Enable L2 prefetch hint
@ Note: The Preload Engine can also be programmed to improve L2 hit
@ rates.  This may be too much work though.
@==================================================================
@==================================================================
	.align
	.global enable_L2_prefetch_hint
	.type enable_L2_prefetch_hint, %function
enable_L2_prefetch_hint:
	MRC     p15, 0, r0, c1, c0, 1      @ Read Auxiliary Control Register
	ORR     r0, r0, #(0x1 << 1)        @ Set bit 1 to enable L2 prefetch hint
	MCR     p15, 0, r0, c1, c0, 1      @ Write Auxiliary Control Register

	BX      lr


@==================================================================
@==================================================================
@ Enable Exclusive Caching
@==================================================================
@==================================================================
	.align
	.global enable_exclusive_caching
	.type enable_exclusive_caching, %function
enable_exclusive_caching:
	@ disable L2C-310
	LDR     r0, =L2CC_PL310
	LDR     r1, =0x0
	STR     r1, [r0,#0x100]

	LDR     r1, [r0,#0x104]		@ auxilary control reg at offset 0x104
	ORR     r1, r1, #(0x1 << 12)    @ set bit 12 in aux. control reg to enable exclusive caching
	STR     r1, [r0,#0x104]		@ auxilary control reg at offset 0x104

	@ enable L2C-310
	LDR     r1, =0x1
	STR     r1, [r0,#0x100]

	MRC     p15, 0, r0, c1, c0, 1      @ Read Auxiliary Control Register
	ORR     r0, r0, #(0x1 << 7)        @ Set EXCL bit 7 to enable Exclusive caching
	MCR     p15, 0, r0, c1, c0, 1      @ Write Auxiliary Control Register

	BX      lr


@==================================================================
@==================================================================
@  SCU Enable
@==================================================================
@==================================================================
	.align
	.global  enable_SCU
	.type    enable_SCU, %function
	@ void enable_SCU(void)
	@ Enables the SCU
enable_SCU:
	MRC     p15, 4, r0, c15, c0, 0     @ Read periph base address
	@ SCU offset from base of private peripheral space = 0x000

	LDR     r1, [r0, #0x0]             @ Read the SCU Control Register
	ORR     r1, r1, #0x1               @ Set bit 0 (The Enable bit)
	STR     r1, [r0, #0x0]             @ Write back modifed value

	BX      lr


@==================================================================
@==================================================================
@  Enable L2 Speculative Linefill in SCU
@==================================================================
@==================================================================
	.align
	.global  enable_L2_speculative_linefill
	.type    enable_L2_speculative_linefill, %function
	@ void enable_L2_speculative_linefill(void)
	@ Enables the SCU
enable_L2_speculative_linefill:
	MRC     p15, 4, r0, c15, c0, 0     @ Read periph base address
	@ SCU offset from base of private peripheral space = 0x000

	LDR     r1, [r0, #0x0]             @ Read the SCU Control Register
	ORR     r1, r1, #0x8               @ Set bit 3 (Speculative L2 linefill)
	STR     r1, [r0, #0x0]             @ Write back modifed value

	BX      lr


@==================================================================
@==================================================================
@  Invalidate L1 caches and branch predictor
@==================================================================
@==================================================================
/* TODO: Clean and invalidate L2 cache as well.
See section 3.10.10 in ARM L2C-310 TRM.
Sequence:
 	* Clean L1
	* DSB
 	* Clean & Invalidate L2
	* SYNC
	* Clean * Invalidate L1
	* DSB
*/

	.align
	.global invalidate_and_flush
	.type   invalidate_and_flush, %function
invalidate_and_flush:
	PUSH	{r4, r5, r7, r9, r10, r11, lr}
/*
@------------------------------------------------------------------
@ Disable Caches and Branch Prediction
@------------------------------------------------------------------
	MRC     p15, 0, r0, c1, c0, 0      @ Read System Control Register
	BIC     r0, r0, #(0x1 << 12)       @ clear I bit 12 to disable I Cache
	BIC     r0, r0, #(0x1 << 2)        @ clear C bit  2 to disable D Cache
	BIC     r0, r0, #(0x1 << 11)       @ clear Z bit 11 to disable branch prediction
	MCR     p15, 0, r0, c1, c0, 0      @ Write System Control Register
*/

@------------------------------------------------------------------
@ Invalidate branch predictor
@------------------------------------------------------------------
	MOV     r0,#0
	MCR     p15, 0, r0, c7, c5, 6      @ BPIALL - Invalidate entire branch predictor array

@------------------------------------------------------------------
@ Invalidate L1 Instruction Cache
@------------------------------------------------------------------
	MRC     p15, 1, r0, c0, c0, 1      @ Read Cache Level ID Register (CLIDR)
	MOV     r0, #0                     @ SBZ
	MCR     p15, 0, r0, c7, c5, 0      @ ICIALLU - Invalidate instruction cache and flush branch target cache
	
@------------------------------------------------------------------
@ Invalidate Data Cache
@------------------------------------------------------------------
	MRC     p15, 1, r0, c0, c0, 1      @ Read CLIDR
	ANDS    r3, r0, #0x07000000        @ Extract coherency level
	MOV     r3, r3, LSR #23            @ Total cache levels << 1
	BEQ     5f                         @ If 0, no need to clean

	MOV     r10, #0                    @ R10 holds current cache level << 1
1:      ADD     r2, r10, r10, LSR #1       @ R2 holds cache "Set" position
	MOV     r1, r0, LSR r2             @ Bottom 3 bits are the Cache-type for this level
	AND     r1, r1, #7                 @ Isolate those lower 3 bits
	CMP     r1, #2
	BLT     4f                         @ No cache or only instruction cache at this level

	MCR     p15, 2, r10, c0, c0, 0     @ Write the Cache Size selection register
	ISB                                @ ISB to sync the change to the CacheSizeID reg
	MRC     p15, 1, r1, c0, c0, 0      @ Reads current Cache Size ID register
	AND     r2, r1, #7                 @ Extract the line length field
	ADD     r2, r2, #4                 @ Add 4 for the line length offset (log2 16 bytes)
	LDR     r4, =0x3FF
	ANDS    r4, r4, r1, LSR #3         @ R4 is the max number on the way size (right aligned)
	CLZ     r5, r4                     @ R5 is the bit position of the way size increment
	LDR     r7, =0x7FFF
	ANDS    r7, r7, r1, LSR #13        @ R7 is the max number of the index size (right aligned)

2:      MOV     r9, r4                     @ R9 working copy of the max way size (right aligned)

3:      ORR     r11, r10, r9, LSL r5       @ Factor in the Way number and cache number into R11
	ORR     r11, r11, r7, LSL r2       @ Factor in the Set number
	MCR     p15, 0, r11, c7, c6, 2     @ Invalidate by Set/Way
	SUBS    r9, r9, #1                 @ Decrement the Way number
	BGE     3b
	SUBS    r7, r7, #1                 @ Decrement the Set number
	BGE     2b
4:      ADD     r10, r10, #2               @ increment the cache number
	CMP     r3, r10
	BGT     1b
5:

	DSB	sy


/*
@------------------------------------------------------------------
@ Re-Enable Caches and Branch Prediction
@------------------------------------------------------------------
//TODO: It is only valid to re-enable features that were already on...
//Requires more work.
// However, it seems unnecessary to turn stuff off and back on.
	MRC     p15, 0, r0, c1, c0, 0      @ Read System Control Register
	ORR     r0, r0, #(0x1 << 12)       @ Set I bit 12 to enable I Cache
	ORR     r0, r0, #(0x1 << 2)        @ Set C bit  2 to enable D Cache
	ORR     r0, r0, #(0x1 << 11)       @ Set Z bit 11 to enable branch prediction
	MCR     p15, 0, r0, c1, c0, 0      @ Write System Control Register
*/

	POP	{r4, r5, r7, r9, r10, r11, pc}	@ pop the lr into the pc


.end

@==================================================================
@  End of file
@==================================================================

