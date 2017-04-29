/**
 * @file    DMA_with_FPGA.c
 * @author  Derek Molloy
 * @date    4 April 2015
 * @version 0.1
 * @brief  An introductory "Hello World!" loadable kernel module (LKM) that can display a message
 * in the /var/log/kern.log file when the module is loaded and removed. The module can accept an
 * argument when it is loaded -- the name, which appears in the kernel log files.
 * @see http://www.derekmolloy.ie/ for a full description and follow-up descriptions.
*/

#include <linux/init.h>             // Macros used to mark up functions e.g., __init __exit
#include <linux/module.h>           // Core header for loading LKMs into the kernel
#include <linux/kernel.h>           // Contains types, macros, functions for the kernel
#include <asm/io.h>		    // For ioremap and ioread32 and iowrite32

#include "PL330-functions.h"

MODULE_LICENSE("GPL");              ///< The license type -- this affects runtime behavior
MODULE_AUTHOR("Derek Molloy");      ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux driver for the BBB.");  ///< The description -- see modinfo
MODULE_VERSION("0.1");              ///< The version of the module

static char *name = "world";        ///< An example LKM argument -- default value is "world"
module_param(name, charp, S_IRUGO); ///< Param desc. charp = char ptr, S_IRUGO can be read/not changed
MODULE_PARM_DESC(name, "The name to display in /var/log/kern.log");  ///< parameter description

//PL330 DMAC Hardware Details
#define PL330_HADDRESS_SECURE 	0xFFE01000	//hardware secure address
#define PL330_HSIZE		0x1000		//PL330 size=4kB 	

//DMA parameters
static void *pl330_vaddress; //virtual address of PLL330 DMAC 


/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init helloBBB_init(void){
   ALT_STATUS_CODE status;
   uint32_t dma_status;
  
   // printk(KERN_INFO "DMA: Hello %s from the BBB LKM!\n", name);
   printk(KERN_INFO "DMA: Initializing module\n");
   
   //Use ioremap to obtain a virtual address for the DMAC
   pl330_vaddress = ioremap(PL330_HADDRESS_SECURE, PL330_HSIZE);
   if (pl330_vaddress == NULL) 
   {
      printk(KERN_INFO "DMA: error doing DMAC ioremap\n");
      goto dma_error_doing_ioremap;
   }
   else
   {
      printk(KERN_INFO "DMA: DMAC ioremap success\n");
   }
   
   //ioread32(virtual address)
   dma_status = ioread32(pl330_vaddress);
   printk(KERN_INFO "DMA status: %u\n",dma_status);
   //void iowrite32(u32 value, void *addr);
   
   // Initialize DMA Controller
   status = ALT_E_SUCCESS;
   status = PL330_init();
   if(status == ALT_E_SUCCESS)
   {}
   
   //To use X to allocate a DMA-able buffer
 
dma_error_doing_ioremap:
   return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit helloBBB_exit(void){
   printk(KERN_INFO "DMA: Exiting module!\n", name);
   
   void iounmap(pl330_vaddress); //iounmap the DMAC
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(helloBBB_init);
module_exit(helloBBB_exit);
