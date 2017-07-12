//
// arm_cache.h
//
// header file for assembly functions to initialize and enable various cache
// functionality
//


#ifndef _ARM_CACHE_H_
#define _ARM_CACHE_H_


// Initialize and enable all caches, and turn on useful optimizations
// This is equivalent to calling the following functions:
//	* enable_MMU()
//	* initialize_L2C()
//	* enable_L1_D_side_prefetch()
//	* enable_L2_hint()
//	* enable_SCU()
//	* enable_caches()
void enable_all_caches(void);

// Enable the Memory Management Unit
// This involves:
// 	* turning off branch prediction, I cache, and D cache
// 	* invalidating I- and D-, and unified TLBs
// 	* invalidating L1 I and D caches
// 	* clearing the branch predictor array
// 	* generating the translation tables (only L1 currently)
// 	* setting the TTBCR and TTBR0 registers for the translation table
// 	* setting domain access control
// 	* enabling the MMU
// Note: All caches and branch prediction will be off after calling enable_MMU()
void enable_MMU(void);

// Initialize the L2 cache controller (ARM L2C-310)
void initialize_L2C(void);

// Enable L1 D-Side Prefetch
void enable_L1_D_side_prefetch(void);

// Enables exclusive caching, where data is only in the L1 or L2 cache,
// but never both
// Note: This enables the L2 cache controller
void enable_exclusive_caching(void);

// Enables the Snoop Control Unit
void enable_SCU(void);

// Enable the L1 data cache
// Note: the MMU must be turned on first
void enable_L1_D(void);

// Enable the L1 instruction cache
void enable_L1_I(void);

// Enable branch prediction
void enable_branch_prediction(void);

// Enable the L2 cache
// Note: initialize_L2C() must be called first to initialize the L2 cache controller
void enable_L2(void);

// Enable L1 D cache, L1 I cache, L2 cache, and branch prediction.
// Equivalent to calling enable_L1_D(), enable_L1_I(), enable_branch_predition()
// and enable_L2()
void enable_caches(void);

// invalidate and flush L1 caches and branch predictor
// TODO: Invalidate L2 as well
void invalidate_and_flush(void);

///////////////////////////////////////
// L2C-310 + Cortex A9 Optimizations //
///////////////////////////////////////

// Enable Early BRESP in L2C-310
// See section 2.5.5 of L2 Cache Controller L2C-310 TRM
void enable_early_BRESP(void);

// Enable sending hints to the L2 cache
void enable_L2_prefetch_hint(void);

// Enable writing a full line of zeros
// Note: This operation turns on the L2 cache controller
// See section 2.5.5 of L2 Cache Controller L2C-310 TRM
void enable_write_full_line_zeros(void);

// Enable speculative linefills of the L2 cache
// Note: enable_SCU() must be called first
void enable_L2_speculative_linefill(void);

// Enable Store Buffer Device Limitation in L2C-310
// See section 2.5.5 of L2 Cache Controller L2C-310 TRM
void enable_store_buffer_device_limitation(void);

#endif


