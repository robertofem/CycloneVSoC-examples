/******************************************************************************
*
* Copyright 2014 Altera Corporation. All Rights Reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. The name of the author may not be used to endorse or promote products
* derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED. IN NO
* EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
*
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include "alt_dma.h"
#include "alt_globaltmr.h"
#include "socal/hps.h"
#include "socal/socal.h"

// Determine the minimum of two values
#define MIN(a, b) ((a) > (b) ? (b) : (a))

// System initialization
ALT_STATUS_CODE system_init(void);

// System shutdown
ALT_STATUS_CODE system_uninit(void);


/******************************************************************************/
/*!
 * System Initialization
 * \return      result of the function
 */
ALT_STATUS_CODE system_init(void)
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

    printf("\n\r");

    return status;
}

/******************************************************************************/
/*!
 * System Cleanup
 *
 * \return      result of the function
 */
ALT_STATUS_CODE system_uninit(void)
{
    printf("INFO: System shutdown.\n\r");
    printf("\n\r");
    return ALT_E_SUCCESS;
}



/******************************************************************************/
/*!
 * Program entrypoint
 *
 * \return result of the program
 */
int main(void)
{
    printf("Hello1!!!\n\r");
	ALT_STATUS_CODE status = ALT_E_SUCCESS;
	
	
	// Maximum size of test data
	#define TEST_BYTES  (1 * 1024)

	// Buffers used for testing
	//uint8_t Write_Buffer[TEST_BYTES];
	uint8_t* Write_Buffer = (uint8_t*) 0xC0000000; //FPGA On-Chip RAM in first position of High Performance HPS-FPGA bridge
	uint8_t Read_Buffer[TEST_BYTES];

	// DMA channel to be used
	ALT_DMA_CHANNEL_t Dma_Channel;
	

	ALT_DMA_PROGRAM_t program;
	ALT_DMA_CHANNEL_FAULT_t fault;

    // System init
    if(status == ALT_E_SUCCESS)
    {
        status = system_init();
    }

	// Allocate DMA Channel
    if (status == ALT_E_SUCCESS)
    {
        printf("INFO: Allocating DMA channel.\n\r");
        status = alt_dma_channel_alloc_any(&Dma_Channel);
    }

    // Demo memory to memory DMA transfers
	uint32_t src_offs = 0;
	uint32_t dst_offs = 0;
	uint32_t size = TEST_BYTES;
	
	//Initialize Buffers with random data
	for(int i = 0; i < TEST_BYTES; i++)
	{
		Read_Buffer[i] = (uint8_t)rand();
		Write_Buffer[i] = (uint8_t)rand();
	}
	
	void* dst=&Write_Buffer[dst_offs];
	void* src=&Read_Buffer[src_offs];
	
	printf("INFO: Demo DMA memory to memory transfer.\n\r");

	// Copy source buffer over destination buffer
	if(status == ALT_E_SUCCESS)
	{
		printf("INFO: Copying from 0x%08x to 0x%08x size = %d bytes.\n\r", (int)src, (int)dst, (int)size);
		status = alt_dma_memory_to_memory(Dma_Channel, &program, dst, src, size, false, (ALT_DMA_EVENT_t)0);
	}

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
				 printf("ERROR: DMA CHannel Fault: %d\n\r", (int)fault);
				 status = ALT_E_ERROR;
			}
		}
	}

	// Compare results
	if(status == ALT_E_SUCCESS)
	{
		printf("INFO: Comparing source and destination buffers.\n\r");
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
        status = system_uninit();
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
