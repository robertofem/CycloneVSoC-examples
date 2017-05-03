//PL330-functions.c
//Support functions to program the driver for PL330 DMA controller

#include <linux/kernel.h>    // Contains types, macros, functions for the kernel
#include <asm/io.h>		    // For ioremap and ioread32 and iowrite32
#include "alt_dma.h" 
#include "hwlib_socal_linux.h" 
#include "PL330-functions.h"

//PL330 DMAC Hardware Details
#define PL330_HADDRESS_SECURE 	0xffe01000	//hardware secure address
#define PL330_HSIZE		0x1000		//PL330 size=4kB 
//Reset Manager Hardware Details
#define RSTMGR_HADDRESS 	0xffd05000	//hardware secure address
#define RSTMGR_HSIZE		0x100		//PL330 size=256B 

//virtual addresses for components
static void* pl330_vaddress;
static void* rstmgr_vaddress;


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

    //--Prepare virtual addresses for DMA Secure and Reset Manager--//
    //Use ioremap to obtain a virtual address for the DMAC
    pl330_vaddress = ioremap(PL330_HADDRESS_SECURE, PL330_HSIZE);
    if (pl330_vaddress == NULL) 
    {
      printk(KERN_INFO "DMA: error doing DMAC ioremap\n");
      goto error_DMAC_ioremap;
    }
    else
    {
      printk(KERN_INFO "DMA: DMAC ioremap success\n");
    }
    
    //Use ioremap to obtain a virtual address for Reset Manager
    rstmgr_vaddress = ioremap(RSTMGR_HADDRESS, RSTMGR_HSIZE);
    if (rstmgr_vaddress == NULL) 
    {
      printk(KERN_INFO "DMA: error doing RSTMGR ioremap\n");
      goto error_RSTMGR_ioremap;
    }
    else
    {
      printk(KERN_INFO "DMA: RSTMGR ioremap success\n");
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
    //status = alt_dma_init(&dma_config);
    if(status == ALT_E_SUCCESS)
    {
       printk(KERN_INFO "DMA: DMAC init success\n");
    }else{
       printk(KERN_INFO "DMA: DMAC init failed\n");
       goto error_uninit;
    }
    return ALT_E_SUCCESS;
    
    //------Error recovery--------//
 error_uninit:
     iounmap(rstmgr_vaddress); //iounmap the RSTMGR
 error_RSTMGR_ioremap:
     iounmap(pl330_vaddress); //iounmap the DMAC
 error_DMAC_ioremap:   
    return ALT_E_ERROR;
}

/******************************************************************************/
/*!
 * System Cleanup
 *
 * \return      result of the function
 */
ALT_STATUS_CODE PL330_uninit(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    printk("INFO: DMA Controller shutdown.\n\r");
    iounmap(rstmgr_vaddress); //iounmap the RSTMGR
    iounmap(pl330_vaddress); //iounmap the DMAC
    
     //-----Uninit DMA------//
    status = alt_dma_uninit();
    if(status == ALT_E_SUCCESS)
    {
       printk(KERN_INFO "DMA: DMAC uninit success\n");
    }else{
       printk(KERN_INFO "DMA: DMAC uninit failed\n");
    }
    return status;
}
