//PL330-functions.c
//Support functions to program the driver for PL330 DMA controller

#include <linux/kernel.h>           // Contains types, macros, functions for the kernel
#include "alt_dma.h" 
#include "hw_lib_common.h" 
#include "PL330-functions.h"



/******************************************************************************/
/*!
 * System Initialization
 * \return      result of the function
 */
ALT_STATUS_CODE PL330_init(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    int i;

    printk("DMA: Setting up DMA Controller.\n\r");

    // Uninit DMA
    if(status == ALT_E_SUCCESS)
    {
        //status = alt_dma_uninit();
    }

    // Configure everything as defaults.
    if (status == ALT_E_SUCCESS)
    {
        ALT_DMA_CFG_t dma_config;
        dma_config.manager_sec = ALT_DMA_SECURITY_DEFAULT;
        for (i = 0; i < 8; ++i)
        {
            dma_config.irq_sec[i] = ALT_DMA_SECURITY_DEFAULT;
        }
        for (i = 0; i < 32; ++i)
        {
            dma_config.periph_sec[i] = ALT_DMA_SECURITY_DEFAULT;
        }
        for (i = 0; i < 4; ++i)
        {
            dma_config.periph_mux[i] = ALT_DMA_PERIPH_MUX_DEFAULT;
        }

        //status = alt_dma_init(&dma_config);
    }

    printk("\n\r");

    return status;
}

/******************************************************************************/
/*!
 * System Cleanup
 *
 * \return      result of the function
 */
ALT_STATUS_CODE PL330_uninit(void)
{
    printk("INFO: DMA Controller shutdown.\n\r");
    printk("\n\r");
    return 0;
    //return  alt_dma_uninit();;
}
