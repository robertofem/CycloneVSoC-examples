#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__
/*
This file indicates where On-Chip RAM is connected:
-On chip RAM connected to 32 bits LIGHTWEIGHT bus
-On chip RAM connected to 32 bits HPS to FPGA High Performance Bridge
-On chip RAM connected to 64 bits HPS to FPGA High Performance Bridge
-On chip RAM connected to 128 bits HPS to FPGA High Performance Bridge
*/

//Select one of the 4 situations
//#define ON_CHIP_RAM_ON_LIGHTWEIGHT
#define ON_CHIP_RAM_ON_HFBRIDGE32
//#define ON_CHIP_RAM_ON_HFBRIDGE64
//#define ON_CHIP_RAM_ON_HFBRIDGE128

//import some files from hardware library
#ifndef soc_cv_av
  #define soc_cv_av
#endif
#include "hwlib/hwlib.h"
#include "hwlib/soc_cv_av/socal/socal.h"
#include "hwlib/soc_cv_av/socal/hps.h"

//define some variable types
typedef uint32_t	uint32_soc;
typedef uint64_t	uint64_soc;
typedef uint64_t  	uint128_soc;

//define some macros depending on the situation
#ifdef ON_CHIP_RAM_ON_LIGHTWEIGHT
	//import some constants from hps_0.h
	#include "hps_0_LW.h"
	#define ON_CHIP_MEMORY_BASE ONCHIP_MEMORY2_0_BASE
	#define ON_CHIP_MEMORY_SPAN ONCHIP_MEMORY2_0_SPAN //size in bytes of memory
	#define ONCHIP_MEMORY_END ONCHIP_MEMORY2_0_END
	//define variable type depending on on-chip RAM data width
	#define UINT_SOC uint32_soc
	//constants to write and read memory
	#define BYTES_DATA_BUS 4
	#define MEMORY_SIZE_IN_WORDS (ON_CHIP_MEMORY_SPAN/BYTES_DATA_BUS) //number of 32bit words
	//macros to define hardware directions
 	#define MMAP_BASE ( ALT_STM_OFST )
	#define MMAP_SPAN ( 0x04000000 ) //64MB (the whole space for regs in FPGA)
	#define MMAP_MASK ( MMAP_SPAN - 1 )
	#define BRIDGE_ADRESS_MAP_START ALT_LWFPGASLVS_OFST
#endif //ON_CHIP_RAM_ON_LIGHTWEIGHT

#ifdef ON_CHIP_RAM_ON_HFBRIDGE32
	//import some constants from hps_0.h
	#include "hps_0_HPS-FPGA-32.h"
	//#include "hps_0_LW.h"
	#define ON_CHIP_MEMORY_BASE ONCHIP_MEMORY2_0_BASE
	#define ON_CHIP_MEMORY_SPAN ONCHIP_MEMORY2_0_SPAN //size in bytes of memory
	#define ONCHIP_MEMORY_END ONCHIP_MEMORY2_0_END
	//define variable type depending on on-chip RAM data width
	#define UINT_SOC uint32_soc
	//constants to write and read memory
	#define BYTES_DATA_BUS 4
	#define MEMORY_SIZE_IN_WORDS (ON_CHIP_MEMORY_SPAN/BYTES_DATA_BUS) //number of 32bit words
	//macros to define hardware directions
 	#define MMAP_BASE 0xC0000000 //default start for HPS-FPGA High performance bridge
	#define MMAP_SPAN ON_CHIP_MEMORY_SPAN //Map just the memory size
	#define MMAP_MASK ( MMAP_SPAN - 1 )
	#define BRIDGE_ADRESS_MAP_START MMAP_BASE
#endif //ON_CHIP_RAM_ON_HFBRIDGE32

#ifdef ON_CHIP_RAM_ON_HFBRIDGE64
	//import some constants from hps_0.h
	#include "hps_0_HPS-FPGA-64.h"
	//#include "hps_0_LW.h"
	#define ON_CHIP_MEMORY_BASE ONCHIP_MEMORY2_0_BASE
	#define ON_CHIP_MEMORY_SPAN ONCHIP_MEMORY2_0_SPAN //size in bytes of memory
	#define ONCHIP_MEMORY_END ONCHIP_MEMORY2_0_END
	//define variable type depending on on-chip RAM data width
	#define UINT_SOC uint64_soc
	//constants to write and read memory
	#define BYTES_DATA_BUS 8
	#define MEMORY_SIZE_IN_WORDS (ON_CHIP_MEMORY_SPAN/BYTES_DATA_BUS) //number of 64bit words
	//macros to define hardware directions
 	#define MMAP_BASE 0xC0000000 //default start for HPS-FPGA High performance bridge
	#define MMAP_SPAN ON_CHIP_MEMORY_SPAN //Map just the memory size
	#define MMAP_MASK ( MMAP_SPAN - 1 )
	#define BRIDGE_ADRESS_MAP_START MMAP_BASE
#endif //ON_CHIP_RAM_ON_HFBRIDGE64


#ifdef ON_CHIP_RAM_ON_HFBRIDGE128
	//import some constants from hps_0.h
	#include "hps_0_HPS-FPGA-128.h"
	//#include "hps_0_LW.h"
	#define ON_CHIP_MEMORY_BASE ONCHIP_MEMORY2_0_BASE
	#define ON_CHIP_MEMORY_SPAN ONCHIP_MEMORY2_0_SPAN //size in bytes of memory
	#define ONCHIP_MEMORY_END ONCHIP_MEMORY2_0_END
	//define variable type depending on on-chip RAM data width
	#define UINT_SOC uint128_soc
	//constants to write and read memory
	#define BYTES_DATA_BUS 8
	#define MEMORY_SIZE_IN_WORDS (ON_CHIP_MEMORY_SPAN/BYTES_DATA_BUS) //number of 128bit words
	//macros to define hardware directions
 	#define MMAP_BASE 0xC0000000 //default start for HPS-FPGA High performance bridge
	#define MMAP_SPAN ON_CHIP_MEMORY_SPAN //Map just the memory size
	#define MMAP_MASK ( MMAP_SPAN - 1 )
	#define BRIDGE_ADRESS_MAP_START MMAP_BASE
#endif //ON_CHIP_RAM_ON_HFBRIDGE128


#define REP_TESTS 100
#define CLK_REP_TESTS 1000

#endif //__CONFIGURATION_H__
