/******************************************************************************
*
* Author: Roberto Fernandez Molanes (robertofem@gmail.com)
* University of Vigo
*
* 28 March 2018
*
* This is a simple example showing how to use a DMA in the FPGA. It is intended
* to be used with the hardware project DMA_FPGA, also available in the
* repository.
* The component for doing DMA is the basic (non scatter-gather) DMA Controller
* available in Qsys. The files fpga_dmac_api.h and fpga_dmac_api.c contain
* macros and functions to control this DMA controller.
*
* In this particular example the DMA controller reads a buffer in processor
* memory and copies to a FPGA On-Chip RAM (FPGA-OCR) using the FPGA-to-HPS port.
* Since the FPGA-to-HPS port access L3, FPGA has access to ACP and L3-SDRAMC
* port, so coherent and no-coherent access can be performed. In this example
* this can be controlled with a macro.
* At hardware level the DMAC controller should be connected to HPS using the
* read port and to FPGA-OCR using the write port, so data can flow in the
* intended direction (this DMA component is not bidirectional and there are
* write and read ports).
* Using a macro it is possible in this example to change the direction of
* the transfer and write into HPS memories the data in FPGA-OCR.
* In this case the hardware project must bu modified to connect the read port
* of the DMA controller to the FPGA-OCR and the write port to the HPS.

* The macros to control the behaviour of the example:
* - SWITCH_ON_CACHE: if it is defined the cache is enabled and the DMAC accesses
*   the buffer in processor memory through ACP port. Access is coherent to
*   cache. If it is not defined  the cache is disabled and access is through
*   L3-SDRAMC port. Notice that the combination of cache enablement and
*   port selected ensures coherency of data without cache flushing. In the case
*   of having the cache switched on and FPGA writing to L3-SDRAM port the
*   processor may read data in cache instead the last version living in SDRAM
*   only. In this case processor should flush the cache before reading this data.
*   If the operation is reading the processor should flush cache after writing
*   data to memory so the FPGA reads the latest version of it.
* - DMA_TRANSFER_SIZE: size of the transfer in Bytes.
* - WRITE_OPERATION: if defined it means that a write operation takes place
*   (read FPGA-OCR and write the HPS memories). If not defined (default),
*   the read operation is done.
******************************************************************************/

/******************MACROS TO CONTROL THE BEHAVIOUR OF THE EXAMPLE*************/
#define SWITCH_ON_CACHE //uncomment to switch on cache and use ACP
#define DMA_TRANSFER_SIZE  256//DMA transfer size in Bytes
//#define WRITE_OPERATION
/*****************************************************************************/
#ifndef soc_cv_av
    #define soc_cv_av
#endif

#include "hwlib.h"
#include <stdbool.h>
#include "socal/hps.h"
#include "socal/socal.h"
#include "socal/alt_acpidmap.h"
#include "alt_address_space.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "arm_cache_modified.h" //to use modified Legup cache config functions
#include "fpga_dmac_api.h"

//The address of FPGA-DMAC is the HPS-FPGA bridge + Qsys address assigned to it
#define FPGA_DMAC_QSYS_ADDRESS 0x10000
#define FPGA_DMAC_ADDRESS ((uint8_t*)0xC0000000+FPGA_DMAC_QSYS_ADDRESS)
//Address of the On-Chip RAM in the FPGA, as seen by processor
#define FPGA_OCR_QSYS_ADDRESS_UP 0x0
#define FPGA_OCR_ADDRESS_UP ((uint8_t*)0xC0000000+FPGA_OCR_QSYS_ADDRESS_UP)
//Address of the On-Chip RAM in the FPGA, as seen by DMAC
#define FPGA_OCR_ADDRESS_DMAC 0x0

//DMAC transfer addresses (from processor they are virtual addresses)
#ifdef WRITE_OPERATION
  #define DMA_TRANSFER_SRC_DMAC   ((uint8_t*) FPGA_OCR_ADDRESS_DMAC)
  #define DMA_TRANSFER_SRC_UP     ((uint8_t*) FPGA_OCR_ADDRESS_UP)
  #ifdef  SWITCH_ON_CACHE
    //add 0x80000000 to access through ACP
    #define DMA_TRANSFER_DST_DMAC   ((uint8_t*) &Buffer[0] + 0x80000000)
  #else
    #define DMA_TRANSFER_DST_DMAC   ((uint8_t*) Buffer)
  #endif
  #define DMA_TRANSFER_DST_UP     ((uint8_t*) Buffer)
#else
  #ifdef  SWITCH_ON_CACHE
    //add 0x80000000 to access through ACP
    #define DMA_TRANSFER_SRC_DMAC   ((uint8_t*) &Buffer[0] + 0x80000000)
  #else
    #define DMA_TRANSFER_SRC_DMAC   ((uint8_t*) Buffer)
  #endif
  #define DMA_TRANSFER_SRC_UP     ((uint8_t*) Buffer)
  #define DMA_TRANSFER_DST_DMAC   ((uint8_t*) FPGA_OCR_ADDRESS_DMAC)
  #define DMA_TRANSFER_DST_UP     ((uint8_t*) FPGA_OCR_ADDRESS_UP)
#endif

//GPIO to change AXI AXI_SIGNALS
#define GPIO_QSYS_ADDRESS 0x20000
#define GPIO_ADDRESS ((uint8_t*)0xC0000000 + GPIO_QSYS_ADDRESS)

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
void cache_configuration(int cache_config);
void print_src_dst(uint8_t* src, uint8_t* dst, int size);

/***************************Program entrypoint********************************/
int main(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    uint32_t AXI_SIGNALS;
    uint8_t* gpio_add = GPIO_ADDRESS;

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
  fpga_dma_init(FPGA_DMAC_ADDRESS,
                FPGA_DMA_QUADWORD_TRANSFERS | //128-bit bus
                FPGA_DMA_END_WHEN_LENGHT_ZERO);

  //alligned allocation to the transfer size is needed for reading HPS from FPGA
  void* unalligned_Buffer;
  uint8_t* Buffer = (uint8_t*) align_malloc(DMA_TRANSFER_SIZE, &unalligned_Buffer);

  //------DEFINE AXI SIGNALS THAT CAN AFFECT THE TRANSACTION-------//
  //AXI_SIGNALS[3-0]  = AWCACHE = 0111 (Cacheable write-back, allocate reads only)
  //AXI_SIGNALS[6-4]  = AWPROT = 000 (normal access, non-secure, data)
  //AXI_SIGNALS[11-7] = AWUSER = 00001 (Coherent access)
  //AXI_SIGNALS[19-16]  = ARCACHE = 0111
  //AXI_SIGNALS[22-20]  = ARPROT = 000
  //AXI_SIGNALS[27-23] = ARUSER = 00001
  //AXI_SIGNALS = 0x00870087; //works for WR and RD and gives fastest accesses
  AXI_SIGNALS = 0x00870087;
  //Write data to the GPIO connected to AXI signals
  *gpio_add = AXI_SIGNALS;

  //--Initialize Buffers with random data and print--//
	printf("INFO: Initializing data in transfer buffers\n\r");
	for(int i = 0; i < DMA_TRANSFER_SIZE; i++)
	{
		DMA_TRANSFER_SRC_UP[i] = (uint8_t)rand();
    DMA_TRANSFER_DST_UP[i] = (uint8_t)rand();
	}

  print_src_dst(DMA_TRANSFER_SRC_UP, DMA_TRANSFER_DST_UP, DMA_TRANSFER_SIZE);

    //--Copy source buffer to destination buffer--//
	printf("CPU: Copying from 0x%08x to 0x%08x size = %d bytes.\n\r",
    (int)DMA_TRANSFER_SRC_UP, (int)DMA_TRANSFER_DST_UP, (int)DMA_TRANSFER_SIZE);
  printf("DMA: Copying from 0x%08x to 0x%08x size = %d bytes.\n\r",
    (int)DMA_TRANSFER_SRC_DMAC, (int)DMA_TRANSFER_DST_DMAC, (int)DMA_TRANSFER_SIZE);

  fpga_dma_config_transfer(FPGA_DMAC_ADDRESS,
                           DMA_TRANSFER_SRC_DMAC,
                           DMA_TRANSFER_DST_DMAC,
                           DMA_TRANSFER_SIZE);

  //Start transfer
  fpga_dma_start_transfer(FPGA_DMAC_ADDRESS);

  while(fpga_dma_transfer_done(FPGA_DMAC_ADDRESS)==0) {}

  //-- Print and compare results--//
  print_src_dst(DMA_TRANSFER_SRC_UP, DMA_TRANSFER_DST_UP, DMA_TRANSFER_SIZE);
  if(memcmp(DMA_TRANSFER_SRC_UP, DMA_TRANSFER_DST_UP,(size_t)DMA_TRANSFER_SIZE)==0)
    printf("Transfer Successful!\n");
  else
    printf("Transfer Failed\n");

  free(unalligned_Buffer);

  return 0;
}


/************************extra fuction definitions****************************/
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
