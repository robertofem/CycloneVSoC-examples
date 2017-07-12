/******************************************************************************
*
* Author: Roberto Fernandez Molanes (robertofem@gmail.com)
* University of Vigo
*
* 31 May 2017
*
* This is a complete example on moving data using the HPS DMAC PL330. 
* This example also shows how to switch on the cache memories L1 and L2
* and how to configure the ACP port to access cache memories from L3.
* 
* This example moves data between a source buffer in processor´s memory to 
* a destiny buffer in processor´s memory or in the FPGA:
*   - When the macro USE_FPGA is not defined the destiny is 
*     in processors memory.
*   - When the macro USE_FPGA is defined the destiny is a buffer
*     in the FPGA in the address 0xC0000000, the beginning of the HPS-FPGA
*     bridge. Therefore when selecting this option there should be a memory
*     in the FPGA with enough space to do the transfer. In the folder
*     https://github.com/robertofem/CycloneVSoC_Examples/tree/master/
*     DE1-SoC/FPGA_Hardware/FPGA_OCR_256K you can find a hardware project
*     implementing a 256KB On-Chip RAM memory in the FPGA for this purpose.
*
* To control the cache behaviour there is the macro SWITCH_ON_CACHE.
*   - When SWITCH_ON_CACHE is not defined the cache system L1 and L2 
*     are switched off. In this case the DMAC accesses the processor RAM
*     using the direct conection between L3 interconnect and SDRAM controller.
*     The reader can modify transfer pointers to access processor memories 
*     through ACP but this is slower because ACP will access L2 cache 
*     controller and L2 cache controller will finally access RAM to get the
*     data because cache is off.
*   - When SWITCH_ON_CACHE is defined the cache L1 and L2 are switched on 
*     with optimizations using Legup functions. In this case the DMAC
*     accesses the processor cached memory in a coherent way using the ACP.
*     This is very much faster than when the cache is off because: a) the 
*     processor instructions are executed faster, b) the data is in cache that
*     is faster than RAM. However for big buffers (bigger than cache size)
*     this strategy could be counterproductive because the DMA will be writing
*     /reading cache through APC but data is not there so L2 controller will
*     need to access external RAM anyway. In this case (not treated in this 
*     example) it is better to directly access to external RAM through the 
*     RAM Controller (like when cache is OFF). In this case, to maintain the 
*     coherency of the data the user should flush the cache after the transfer
*     when writing to the processor RAM or before the transfer when reading 
*     from processor RAM.
*
* This example was programmed modifying the "HPS DMA Example" from Altera. 
* File name: Altera-SoCFPGA-HardwareLib-DMA-CV-GNU.tar.
* Legup cache functions for Cyclone V SoC in arm_cache.c were slightly
* modified. Thats why we created the files arm_cache_modified.h and 
* arm_cache_modified.c. The modification is only in arm_cache.c: 
* L1_NORMAL_111_11 constant was changed from its original value 
* 0x00007c0e to 0x00017c0e. This change sets S bit in table 
* descriptors definyng normal memory region attributes, making 
* normal memory shareable (coherent) for ACP accesses.
* alt_dma_modified.c also contains modifications to the original 
* alt_dma.c from hwlib. The changes are changes in macros 
* ALT_DMA_CCR_OPT_SC_DEFAULT and ALT_DMA_CCR_OPT_DC_DEFAULT so the DMA
* does cacheable accesses. Othe change is the split of  
* alt_dma_memory_to_memory() into 2 functions 
* alt_dma_memory_to_memory_only_prepare_program() and alt_dma_channel_exec().
* This way the program and its execution can run separately. The program can
* be prepare during initializations only once and its execution time is later 
* reduced when doing the transfer.
******************************************************************************/
#ifndef soc_cv_av
    #define soc_cv_av
#endif

#include "hwlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include "alt_dma_modified.h"
#include "alt_globaltmr.h"
#include "socal/hps.h"
#include "socal/socal.h"
#include "socal/alt_acpidmap.h"
#include "alt_address_space.h"

#include "arm_cache_modified.h" //to use modified Legup cache config functions

/******************MACROS TO CONTROL THE BEHAVIOUR OF THE EXAMPLE*************/
//#define USE_FPGA    //uncomment to copy to FPGA
#define SWITCH_ON_CACHE //uncomment to switch on cache and use ACP

#define DMA_TRANSFER_SIZE  4 //DMA transfer size in Bytes
//when cache on add 0x8000000 to processor address to access processor RAM
//through acp from L3 with DMAC
#ifdef SWITCH_ON_CACHE //cache is on
    #define DMA_TRANSFER_SRC_DMAC   ((uint8_t*)&Read_Buffer[0] + 0x80000000) //_DMAC: address for DMAC
    #define DMA_TRANSFER_SRC_UP     ((uint8_t*)&Read_Buffer[0]) //_UP: address for processor
    #ifdef USE_FPGA 
        #define DMA_TRANSFER_DST_DMAC   ((uint8_t*)0xC0000000) //beginning of HPS-FPGA bridge
        #define DMA_TRANSFER_DST_UP     ((uint8_t*)0xC0000000)
    #else
        #define DMA_TRANSFER_DST_DMAC   ((uint8_t*) &Write_Buffer[0] + 0x80000000)
        #define DMA_TRANSFER_DST_UP     ((uint8_t*) &Write_Buffer[0])
    #endif
#else 
    //cache is off. address to be used by DMAC are same as processor 
    //so direct access  to RAM from L3(ACP not used)
    #define DMA_TRANSFER_SRC_DMAC ((uint8_t*)&Read_Buffer[0])
    #define DMA_TRANSFER_SRC_UP ((uint8_t*)&Read_Buffer[0])
    #ifdef USE_FPGA 
        #define DMA_TRANSFER_DST_DMAC   ((uint8_t*)0xC0000000)
        #define DMA_TRANSFER_DST_UP     ((uint8_t*)0xC0000000) 
    #else
        #define DMA_TRANSFER_DST_DMAC   ((uint8_t*) &Write_Buffer[0])
        #define DMA_TRANSFER_DST_UP     ((uint8_t*) &Write_Buffer[0])
    #endif
#endif

/************MACRO TO SELECT THE CACHE CONFIGURATION WHEN CACHE IS ON*********/
//With this define select the level of cache optimization
/*Options for cache config (each config is added to the all previous ones):
0 no cache
(basic config and optimizations)
1 enable MMU
2 do 1 and initialize L2C
3 do 2 and enable SCU
4 do 3 and enable L1_I
5 do 4 and enable branch prediction
6 do 5 and enable L1_D
7 do 6 and enable L1 D side prefetch
8 do 7 and enable L2C
(special L2C-310 controller + Cortex A9 optimizations)
9 do 8 and enable L2 prefetch hint
10 do 9 and enable write full line zeros
11 do 10 and enable speculative linefills of L2 cache
12 do 11 and enable early BRESP
13 do 12 and store_buffer_limitation
*/
#define CACHE_CONFIGURATION 9 

/********************extra fuction declarations*******************************/
ALT_STATUS_CODE dmac_init(void);
ALT_STATUS_CODE dmac_uninit(void);
void cache_configuration(int cache_config);
void print_src_dst(uint8_t* src, uint8_t* dst, int size);

/***************************Program entrypoint********************************/
int main(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    
    //--Cache configuration--//
    #ifdef SWITCH_ON_CACHE 
        cache_configuration(CACHE_CONFIGURATION);
    #endif
    
	//--ACP configuration--//
	const uint32_t ARUSER = 0b11111; //coherent cacheable reads
	const uint32_t AWUSER = 0b11111; //coherent cacheable writes
	if(status == ALT_E_SUCCESS)
    {
		//Set output ID3 for dynamic reads and ID4 for dynamic writes
		status = alt_acp_id_map_dynamic_read_set(ALT_ACP_ID_OUT_DYNAM_ID_3);
		status = alt_acp_id_map_dynamic_write_set(ALT_ACP_ID_OUT_DYNAM_ID_4);
		//Configure the page and user write sideband signal options that are applied 
		//to all write transactions that have their input IDs dynamically mapped.
		status = alt_acp_id_map_dynamic_read_options_set(ALT_ACP_ID_MAP_PAGE_0, ARUSER);
		status = alt_acp_id_map_dynamic_write_options_set(ALT_ACP_ID_MAP_PAGE_0, AWUSER);
	}
	
	if(status == ALT_E_SUCCESS) printf("ACP ID Mapper configuration was successful\n\r");
	else
	{
		printf("ACP ID Mapper configuration was not successful\n\r");
		return 1;
	}
    
    //--DMAC initialization--// 
    status = dmac_init();
    if(status == ALT_E_SUCCESS) printf("PL330 DMAC initialization was successful\n\r");
	else
	{
		printf("PL330 DMAC initialization was not successful\n\r");
		return 1;
	}
        
    //--define some variables to do the transfer--//
	ALT_DMA_CHANNEL_t Dma_Channel; // DMA channel to be used
    ALT_DMA_CHANNEL_FAULT_t fault;
    //To store DMA microcode in processor memory uncomment the next 2 lines 
	//ALT_DMA_PROGRAM_t program; 
    //ALT_DMA_PROGRAM_t program_ptr = &program;
	//To store the DMA microcode in HPS On-Chip RAM uncomment the next line
    ALT_DMA_PROGRAM_t* program_ptr = (ALT_DMA_PROGRAM_t*) 0xFFFF0000; 
	uint8_t Read_Buffer[DMA_TRANSFER_SIZE]; //Source buffer for DMA transfer
    #ifndef USE_FPGA
    uint8_t Write_Buffer[DMA_TRANSFER_SIZE]; //Destiny buffer for DMA transfer
    #endif                                   //when #USE_FPGA is not defined
    
    
    
    //--Initialize Buffers with random data and print--//
	printf("INFO: Initializing data in transfer buffers\n\r");
	for(int i = 0; i < DMA_TRANSFER_SIZE; i++)
	{
		DMA_TRANSFER_DST_UP[i] = (uint8_t)rand();
		DMA_TRANSFER_SRC_UP[i] = (uint8_t)rand();
	}
    print_src_dst(DMA_TRANSFER_DST_UP, DMA_TRANSFER_SRC_UP, DMA_TRANSFER_SIZE);
    
    //--Allocate a DMA channel and do the transfer--//
    printf("INFO: Allocating DMA channel.\n\r");
    status = alt_dma_channel_alloc_any(&Dma_Channel);
    if(status == ALT_E_SUCCESS) printf("INFO: DMA channel allocation was successful\n\r");
	else
	{
		printf("INFO: DMA channel allocation was not successful\n\r");
		return 1;
	}
    
    //--Copy source buffer to destination buffer--//
	printf("INFO: Copying from 0x%08x to 0x%08x size = %d bytes.\n\r", 
    (int)DMA_TRANSFER_SRC_DMAC, (int)DMA_TRANSFER_DST_DMAC, (int)DMA_TRANSFER_SIZE);
	status = alt_dma_memory_to_memory(Dma_Channel, program_ptr, 
    DMA_TRANSFER_DST_DMAC, DMA_TRANSFER_SRC_DMAC, DMA_TRANSFER_SIZE, false, (ALT_DMA_EVENT_t)0);
	// Wait for transfer to complete
	if (status == ALT_E_SUCCESS)
	{
		printf("INFO: Waiting for DMA transfer to complete.\n\r");
		ALT_DMA_CHANNEL_STATE_t channel_state = ALT_DMA_CHANNEL_STATE_EXECUTING;
		while((status == ALT_E_SUCCESS) && (channel_state != ALT_DMA_CHANNEL_STATE_STOPPED))
		{
			status = alt_dma_channel_state_get(Dma_Channel, &channel_state);
			if(channel_state == ALT_DMA_CHANNEL_STATE_FAULTING)
			{
				 alt_dma_channel_fault_status_get(Dma_Channel, &fault);
				 printf("ERROR: DMA Channel Fault: %d\n\r", (int)fault);
				 return 1;
			}
		}
	}
    
    //-- Print and compare results--//
    print_src_dst(DMA_TRANSFER_DST_UP, DMA_TRANSFER_SRC_UP, DMA_TRANSFER_SIZE);
	if(0  != memcmp(DMA_TRANSFER_SRC_UP, DMA_TRANSFER_DST_UP, DMA_TRANSFER_SIZE))
	{
        printf("INFO: DMA Transfer failed!\n\r");
	}
    else
    {
        printf("INFO: DMA Transfer was successful!\n\r");
    }
    
    return 0;
}


/************************extra fuction definitions****************************/
// DMA Controller initialization
ALT_STATUS_CODE dmac_init(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    printf("INFO: Setting up DMA.\n\r");

    // Uninit DMA
    if(status == ALT_E_SUCCESS)
    {
        status = alt_dma_uninit();
    }

    // Configure everything as defaults.
    if (status == ALT_E_SUCCESS)
    {
        ALT_DMA_CFG_t dma_config;
        dma_config.manager_sec = ALT_DMA_SECURITY_DEFAULT;
        for (int i = 0; i < 8; ++i)
        {
            dma_config.irq_sec[i] = ALT_DMA_SECURITY_DEFAULT;
        }
        for (int i = 0; i < 32; ++i)
        {
            dma_config.periph_sec[i] = ALT_DMA_SECURITY_DEFAULT;
        }
        for (int i = 0; i < 4; ++i)
        {
            dma_config.periph_mux[i] = ALT_DMA_PERIPH_MUX_DEFAULT;
        }

        status = alt_dma_init(&dma_config);
    }


    return status;
}

// DMA Controller uninitialization
ALT_STATUS_CODE dmac_uninit(void)
{
    printf("INFO: System shutdown.\n\r");
    return ALT_E_SUCCESS;
}

// Cache configuration using Legup functions
void cache_configuration(int cache_config)
{
	if (cache_config<=0){ //0 no cache
		printf("\n\rCACHE CONFIG:0 No cache\n\r");
	}else if (cache_config<=1){ //1 enable MMU
		printf("\n\rCACHE CONFIG:1 Enable MMU\n\r");
		enable_MMU(); 
	}else if (cache_config<=2){//2 do 1 and initialize L2C
		printf("\n\rCACHE CONFIG:2 Enable MMU and init L2C\n\r");
		enable_MMU();
		initialize_L2C();
	}else if (cache_config<=3){ //3 do 2 and enable SCU
		printf("\n\rCACHE CONFIG:3 Enable MMU and SCU and init L2C\n\r");
		enable_MMU();
		initialize_L2C();
		enable_SCU();
	}else if (cache_config<=4){ //4 do 3 and enable L1_I
		printf("\n\rCACHE CONFIG:4 L1_I\n\r");
		enable_MMU();
		initialize_L2C();
		enable_SCU();
		enable_L1_I();
	}else if (cache_config<=5){ //5 do 4 and enable branch prediction
		printf("\n\rCACHE CONFIG:5 L1_I and branch prediction\n\r");
		enable_MMU();
		initialize_L2C();
		enable_SCU();
		enable_L1_I();
		enable_branch_prediction();
	}else if (cache_config<=6){ // 6 do 5 and enable L1_D
		printf("\n\rCACHE CONFIG:6 L1_D, L1_I and branch prediction\n\r");
		enable_MMU();
		initialize_L2C();
		enable_SCU();
		enable_L1_D();
		enable_L1_I();
		enable_branch_prediction();
	}else if (cache_config<=7){ // 7 do 6 and enable L1 D side prefetch
		printf("\n\rCACHE CONFIG:7 L1_D with side prefetch), L1_I with branch prediction\n\r");
		enable_MMU();
		initialize_L2C();
		enable_L1_D_side_prefetch();
		enable_SCU();
		enable_L1_D();
		enable_L1_I();
		enable_branch_prediction();
	}else if (cache_config<=8){ // 8 do 7 and enable L2C
		printf("\n\rCACHE CONFIG:8 L1_D side prefetch, L1_I with branch pre. and enable L2\n\r");
		enable_MMU();
		initialize_L2C();
		enable_L1_D_side_prefetch();
		enable_SCU();
		enable_caches(); //equivalent to enable L1_D,L1_I,branch prediction and L2
	}else if (cache_config<=9){ // 9 do 8 and enable L2 prefetch hint
		printf("\n\rCACHE CONFIG:9 basic config. + L2 prefetch hint\n\r");
		enable_MMU();
		initialize_L2C();
		enable_L1_D_side_prefetch();
		enable_L2_prefetch_hint();
		enable_SCU();
		enable_caches(); //equivalent to enable L1_D,L1_I,branch prediction and L2
	}else if (cache_config<=10){ // 10 do 9 and enable write full line zeros
		printf("\n\rCACHE CONFIG:10 basic config. + L2ph + wr full line 0s\n\r");
		enable_MMU();
		initialize_L2C();
		enable_L1_D_side_prefetch();
		enable_L2_prefetch_hint();
		enable_SCU();
		enable_write_full_line_zeros();
		enable_caches(); //equivalent to enable L1_D,L1_I,branch prediction and L2
	}else if (cache_config<=11){ // 11 do 10 and enable speculative linefills of L2 cache
		printf("\n\rCACHE CONFIG:11 basic config. + L2ph + wrfl0s + speculative linefills\n\r");
		enable_MMU();
		initialize_L2C();
		enable_L1_D_side_prefetch();
		enable_L2_prefetch_hint();
		enable_SCU();
		enable_L2_speculative_linefill(); //call SCU first
		enable_write_full_line_zeros();
		enable_caches(); //equivalent to enable L1_D,L1_I,branch prediction and L2
	}else if (cache_config<=12){ // 12 do 11 and enable early BRESP
		printf("\n\rCACHE CONFIG:12 basic config. + L2ph + wrfl0s + sl + eBRESP\n\r");
		enable_MMU();
		initialize_L2C();
		enable_L1_D_side_prefetch();
		enable_L2_prefetch_hint();
		enable_SCU();
		enable_L2_speculative_linefill(); //call SCU first
		enable_write_full_line_zeros();
		enable_early_BRESP();
		enable_caches(); //equivalent to enable L1_D,L1_I,branch prediction and L2
	}else if (cache_config<=13){ // 13 do 12 and store_buffer_limitation
		printf("\n\rCACHE CONFIG:13 basic config. + L2ph + wrfl0s + sl + eBRESP + buffer store limitation\n\r");
		enable_MMU();
		initialize_L2C();
		enable_L1_D_side_prefetch();
		enable_L2_prefetch_hint();
		enable_SCU();
		enable_L2_speculative_linefill(); //call SCU first
		enable_write_full_line_zeros();
		enable_early_BRESP();
		enable_store_buffer_device_limitation();
		enable_caches(); //equivalent to enable L1_D,L1_I,branch prediction and L2
	}
}

void print_src_dst(uint8_t* src, uint8_t* dst, int size)
{
  int i;
  uint8_t* char_ptr_scr = (uint8_t*) src;
  uint8_t* char_ptr_dst= (uint8_t*) dst;
  
  printf( "Source=[");
  for (i=0; i<size; i++)
  {
    printf( "%u", *char_ptr_scr);
    char_ptr_scr++;
    if (i<(size-1)) printf(",");
  }
  printf("]\n\r");
  
  printf( "Destiny=[");
  for (i=0; i<size; i++)
  {
    printf("%u", *char_ptr_dst);
    char_ptr_dst++;
    if (i<(size-1)) printf(",");
  }
  printf( "]\n\r");
}

