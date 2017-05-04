//PL330-functions.c
//Support functions to program the driver for PL330 DMA controller

#include <linux/kernel.h>    // Contains types, macros, functions for the kernel
#include <asm/io.h>		    // For ioremap and ioread32 and iowrite32
#include "alt_dma.h" 
#include "hwlib_socal_linux.h" 
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
    ALT_DMA_CFG_t dma_config;  
    
    //-----IOMAP DMA to be able to access from kernel space------//
    status = alt_dma_iomap();
    if(status == ALT_E_SUCCESS)
    {
       printk(KERN_INFO "DMA: iomap process success\n");
    }else{
       printk(KERN_INFO "DMA: iomap process failed\n");
       goto error_iomap;
    }
    
    //-----Uninit DMA------//
    status = alt_dma_uninit();
    if(status == ALT_E_SUCCESS)
    {
       printk(KERN_INFO "DMA: DMAC uninit success\n");
    }else{
       printk(KERN_INFO "DMA: DMAC uninit failed\n");
       goto error_uninit;
    }

    //-------Configure everything as defaults.----//    
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

    //------Initialize DMAC--------//
    status = alt_dma_init(&dma_config);
    if(status == ALT_E_SUCCESS)
    {
       printk(KERN_INFO "DMA: DMAC init success\n");
    }else{
       printk(KERN_INFO "DMA: DMAC init failed\n");
       goto error_uninit;
    }
    
    return ALT_E_SUCCESS;
 error_uninit:
     alt_dma_iounmap();
 error_iomap:  
    return ALT_E_ERROR;
}

/******************************************************************************/
/*!
 * System Cleanup
 *
 * \return      result of the function
 */
ALT_STATUS_CODE  PL330_uninit(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    printk("DMA LKM: DMA Controller shutdown.\n");
    
    //-----Uninit DMA------//
    status = alt_dma_uninit();
    if(status == ALT_E_SUCCESS)
    {
       printk(KERN_INFO "DMA: DMAC uninit success\n");
    }else{
       printk(KERN_INFO "DMA: DMAC uninit failed\n");
    }
    
    //-----DMA unmap------//
    status = alt_dma_iounmap();
    if(status == ALT_E_SUCCESS)
    {
       printk(KERN_INFO "DMA: DMAC iounmap success\n");
    }else{
       printk(KERN_INFO "DMA: DMAC iounmap failed\n");
    }
    
    return status;
}
