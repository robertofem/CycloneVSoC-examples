/******************************************************************************
*
* Author: Roberto Fernandez Molanes (robertofem@gmail.com)
* University of Vigo
*
* 16 March 2016
*
* This is a basic example moving data from one array in CPU cache to 
* another array in CPU cache using DMA Controller and ACP (Aceleration
* Coherency Port).
* To be able to do this example the fuction alt_dma_memory_to_memory()
* from the HWLIB file alt_dma.c was slightly modified. The modification
* is just the last line of the function separating the preparation of
* the DMAC program and its execution. For this reason the files
* alt_dma_modified.c and alt_dma_modified.h were generated.
* 
*
* Programmed modifying the "HPS DMA Example" from altera. File name:
* Altera-SoCFPGA-HardwareLib-DMA-CV-GNU.tar
*
******************************************************************************/

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

#include "arm_cache_modified.h" //to use Legup cache config functions

#define CACHE_CONFIGURATION 9 //with this define select the level of cache optimization
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

/************************extra fuction declarations***********************************/
ALT_STATUS_CODE dmac_init(void);
ALT_STATUS_CODE dmac_uninit(void);
void cache_configuration(int cache_config);
void printf_dmastatus(ALT_DMA_CHANNEL_t channel);

/*******************************Program entrypoint************************************/
int main(void)
{
	printf("Hello man\n\r");
	ALT_STATUS_CODE status = ALT_E_SUCCESS;
	//Some registers
	//DMAC
	unsigned int *CCR0 = (unsigned int *)0xFFE01408;
	unsigned int *SAR0 = (unsigned int *)0xFFE01400;
	unsigned int *DAR0 = (unsigned int *)0xFFE01404;
	//ACP
	unsigned int *VID3RD_S = (unsigned int *)0xFF707038;
	unsigned int *VID4WR_S = (unsigned int *)0xFF707044;
	unsigned int *DYNRD = (unsigned int *)0xFF707058;
	unsigned int *DYNWR = (unsigned int *)0xFF70705C;
	
	//--Configure cache--//
	cache_configuration(CACHE_CONFIGURATION);
	
	//--ACP configuration--//
	printf("\n\rConfiguring ACP\n\r");
	printf("Before acp config VID3RD_S:%#x, VID4WR_S:%#x, DYNRD:%#x, DYNWR:%#x\n\r", *VID3RD_S, *VID4WR_S, *DYNRD, *DYNWR);
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
	
	if(status == ALT_E_SUCCESS)
	{
		printf("ACP ID Mapper configuration was succesful\n\r");
		printf("After acp config VID3RD_S:%#x, VID4WR_S:%#x, DYNRD:%#x, DYNWR:%#x\n\r", *VID3RD_S, *VID4WR_S, *DYNRD, *DYNWR);
	}
	else
	{
		printf("ACP ID Mapper configuration was not succesful\n\r");
		return 1;
	}
	
	//--define some variables to do the transfer--//
	ALT_DMA_CHANNEL_t Dma_Channel; // DMA channel to be used
	ALT_DMA_PROGRAM_t program; // Struct to store the DMA program
	//ALT_DMA_PROGRAM_t* program_ptr = (ALT_DMA_PROGRAM_t*) 0xFFFF0000; // Struct to store the DMA program (in HPS On-Chip RAM)
	ALT_DMA_CHANNEL_FAULT_t fault;
	
	// Maximum size of test data
	#define TEST_BYTES  (1 * 1024)
	// Buffers used for testing 
	//uint8_t Write_Buffer[TEST_BYTES]; //buffer in normal memory
	uint8_t* Write_Buffer = (uint8_t*) 0xFFFF0000+sizeof(ALT_DMA_PROGRAM_t); //buffer in HPS OCR
	uint8_t Read_Buffer[TEST_BYTES]; //buffer in normal memory
	//uint8_t* Read_Buffer = (uint8_t*) 0xFFFF0000+sizeof(ALT_DMA_PROGRAM_t)+TEST_BYTES; //buffer in HPS OCR
	
	//Initialize Buffers with random data
	printf("\n\rInitializing data in transfer buffers\n\r");
	for(int i = 0; i < TEST_BYTES; i++)
	{
		Read_Buffer[i] = 255;//(uint8_t)rand();
		Write_Buffer[i] = 0;//(uint8_t)rand();
	}
	printf("Write:%d,%d,%d,%d,%d,%d,%d,%d Read:%d,%d,%d,%d,%d,%d,%d,%d\n\r", Write_Buffer[0],Write_Buffer[1],Write_Buffer[2],Write_Buffer[3],
	Write_Buffer[4],Write_Buffer[5],Write_Buffer[6],Write_Buffer[7],Read_Buffer[0],Read_Buffer[1],Read_Buffer[2],Read_Buffer[3],Read_Buffer[4],Read_Buffer[5],Read_Buffer[6],Read_Buffer[7]);

    // System init
    if(status == ALT_E_SUCCESS)
    {
        status = dmac_init();
    }

	// Allocate DMA Channel
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Allocating DMA channel.\n\r");
        status = alt_dma_channel_alloc_any(&Dma_Channel);
    }

	printf_dmastatus(Dma_Channel);
	
    // Demo memory to memory DMA transfers
	uint32_t size = TEST_BYTES;
	void* dst=&Write_Buffer[0];
	void* dst_acp=dst+0x80000000; //+0x80000000 to access through ACP
	void* src=&Read_Buffer[0];
	void* src_acp=src+0x80000000;
	ALT_DMA_PROGRAM_t* program_acp = (ALT_DMA_PROGRAM_t*)((void*)(&program)+0x80000000);
	//ALT_DMA_PROGRAM_t* program_acp = (ALT_DMA_PROGRAM_t*)((void*)(program_ptr)+0x80000000);
	
	//Print the real location of buffers and DMA program
	printf("\n\rPrinting some pointers involved in data transfer:\n\r");
	printf("Write Buff: 0x%" PRIXPTR "\n\r", (uintptr_t)Write_Buffer);
	printf("Read Buff: 0x%" PRIXPTR "\n\r", (uintptr_t)Read_Buffer);
	printf("dst: 0x%" PRIXPTR "\n\r", (uintptr_t)dst);
	printf("src: 0x%" PRIXPTR "\n\r", (uintptr_t)src);
	printf("DMA program pointer: 0x%" PRIXPTR "\n\r", (uintptr_t) &program);
	//printf("DMA program pointer: 0x%" PRIXPTR "\n\r", (uintptr_t) program_ptr);
	printf("dst_acp: 0x%" PRIXPTR "\n\r", (uintptr_t)dst_acp);
	printf("src_acp: 0x%" PRIXPTR "\n\r", (uintptr_t)src_acp);
	printf("DMA program pointer acp: 0x%" PRIXPTR "\n\r", (uintptr_t) program_acp);
	
	printf("\n\rStart DMA memory to memory transfer.\n\r");
	printf("DMAC regs before transfer CCR:%#x, SRC:%#x, DST:%#x\n\r", *CCR0, *SAR0, *DAR0);
	
	ALT_DMA_PROGRAM_t program2;
	
	if(status == ALT_E_SUCCESS)
	{
		printf("INFO:Copying from 0x%08x to 0x%08x size = %d bytes.\n\r", (int)src, (int)dst, (int)size);
		// Prepare the program to copy source buffer over destination buffer
		//status = alt_dma_memory_to_memory_only_prepare_program(Dma_Channel, &program, dst_acp, src_acp, size, false, (ALT_DMA_EVENT_t)0);
		status = alt_dma_memory_to_memory_only_prepare_program(Dma_Channel, &program, dst, src_acp, size, false, (ALT_DMA_EVENT_t)0);
		//status = alt_dma_channel_exec(Dma_Channel, program_acp);
		status = alt_dma_channel_exec(Dma_Channel, &program);
	}
	
	// Wait for transfer to complete
	if (status == ALT_E_SUCCESS)
	{
		printf("INFO: Waiting for DMA transfer to complete.\n\r");
		ALT_DMA_CHANNEL_STATE_t channel_state = ALT_DMA_CHANNEL_STATE_EXECUTING;
		while((status == ALT_E_SUCCESS) && (channel_state != ALT_DMA_CHANNEL_STATE_STOPPED))
		{
			//printf_dmastatus(Dma_Channel);
			status = alt_dma_channel_state_get(Dma_Channel, &channel_state);
			if(channel_state == ALT_DMA_CHANNEL_STATE_FAULTING)
			{
				 alt_dma_channel_fault_status_get(Dma_Channel, &fault);
				 printf("ERROR: DMA CHannel Fault: %d\n\r", (int)fault);
				 status = ALT_E_ERROR;
			}
		}
	}
	
	printf_dmastatus(Dma_Channel);
	printf("DMAC regs after transfer CCR0:%#x, SRC0:%#x, DST0:%#x\n\r", *CCR0, *SAR0, *DAR0);
	
	// Compare results
	printf("\n\rCheck the buffers content after the transfer:\n\r");
	printf("Write:%d,%d,%d,%d,%d,%d,%d,%d Read:%d,%d,%d,%d,%d,%d,%d,%d\n\r", Write_Buffer[0],Write_Buffer[1],Write_Buffer[2],Write_Buffer[3],
	Write_Buffer[4],Write_Buffer[5],Write_Buffer[6],Write_Buffer[7],Read_Buffer[0],Read_Buffer[1],Read_Buffer[2],Read_Buffer[3],Read_Buffer[4],Read_Buffer[5],Read_Buffer[6],Read_Buffer[7]);
	
	
	if(status == ALT_E_SUCCESS)
	{
		printf("INFO: Comparing source and destination buffers with memcmp.\n\r");
		if(0  != memcmp(src, dst, size))
		{
			status = ALT_E_ERROR;
		}
	}

	// Display result
	if(status == ALT_E_SUCCESS)
	{
		printf("INFO: Demo DMA memory to memory succeeded.\n\r");
	}
	else
	{
		printf("ERROR: Demo DMA memory to memory failed.\n\r");
	}
	
	printf("\n\r");

    // System uninit
    if(status == ALT_E_SUCCESS)
    {
        status = dmac_uninit();
    }

    // Report status
    if (status == ALT_E_SUCCESS)
    {
        printf("RESULT: All tests successful.\n\r");
        return 0;
    }
    else
    {
        printf("RESULT: Some failures detected.\n\r");
        return 1;
    }
	return 0;
}

/************************extra fuction definitions***********************************/
/******************************************************************************/
// DMA Controller initialization
ALT_STATUS_CODE dmac_init(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    printf("\n\rINFO: Setting up DMA.\n\r");

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

/******************************************************************************/
// DMA Controller uninitialization
ALT_STATUS_CODE dmac_uninit(void)
{
    printf("INFO: System shutdown.\n\r");
    printf("\n\r");
    return ALT_E_SUCCESS;
}

//--------------------------CACHE CONFIGURATION--------------------------//
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

//get status from manager thread and one channel therad
void printf_dmastatus(ALT_DMA_CHANNEL_t channel)
{
	ALT_STATUS_CODE status_code = ALT_E_SUCCESS;
	ALT_DMA_MANAGER_STATE_t manager_state;
	ALT_DMA_MANAGER_FAULT_t manager_fault;
	ALT_DMA_CHANNEL_STATE_t channel_state;
	ALT_DMA_CHANNEL_FAULT_t channel_fault;
	
	//Read DMA Manager status and fault typedef
	status_code = alt_dma_manager_state_get(&manager_state);
	if (status_code == ALT_E_SUCCESS)
	{
		printf("DMAC Manager state: %d\n\r", (int) manager_state);
		if (manager_state == ALT_DMA_MANAGER_STATE_FAULTING)
		{
			status_code = alt_dma_manager_fault_status_get(&manager_fault);
			printf("DMAC Manager fault: %d\n\r", (int) manager_fault);
		}
	}
	else
	{
		printf("Error when getting manager state. Error: %d", (int)status_code);
	}
	
	
	//Read channel status and fault type
	status_code = alt_dma_channel_state_get(channel, &channel_state);									  
	if (status_code == ALT_E_SUCCESS)
	{
		printf("DMAC Channel %d state: %d\n\r", channel, channel_state);
		if ((channel_state == ALT_DMA_CHANNEL_STATE_FAULTING)||
			(channel_state == ALT_DMA_CHANNEL_STATE_FAULTING_COMPLETING))
		{
			status_code = alt_dma_channel_fault_status_get(channel, &channel_fault);
			printf("DMAC Channel %d fault: %d\n\r",(int)channel, (int)channel_fault);
		}
	}
	else
	{
		printf("Error when getting channel %d state. Error: %d",(int)channel, (int)status_code);
	}									  
}


