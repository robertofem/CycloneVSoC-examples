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
#include <linux/dma-mapping.h>	    //To obtain DMA-able buffers

#include "PL330-functions.h"
#include "alt_dma.h"
#include "alt_dma_common.h"

MODULE_LICENSE("GPL");              ///< The license type -- this affects runtime behavior
MODULE_AUTHOR("Derek Molloy");      ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux driver for the BBB.");  ///< The description -- see modinfo
MODULE_VERSION("0.1");              ///< The version of the module

static char *name = "world";        ///< An example LKM argument -- default value is "world"
module_param(name, charp, S_IRUGO); ///< Param desc. charp = char ptr, S_IRUGO can be read/not changed
MODULE_PARM_DESC(name, "The name to display in /var/log/kern.log");  ///< parameter description

//Constants 
//FPGA On-CHip RAM
#define FPGA_OCR_HADDRESS 0xC0000000 //Hardware address of the On-Chip RAM in the FPGA
#define FPGA_OCR_SIZE	  0x40000    //Size in bytes=256KB
//HPS On-Chip RAM
#define HPS_OCR_HADDRESS 0xFFFF0000 //Hardware address 
#define HPS_OCR_SIZE	 0x10000    //Size in bytes=64kB

//variables to do the DMA transfer
static void* fpga_ocr_vaddress;       //virtual address of the On-CHip RAM in the FPGA
static void* hps_ocr_vaddress;       //virtual address of the On-CHip RAM in the FPGA
static void* non_cached_mem_v; 	//4k coherent buffer, address to use in module
static dma_addr_t non_cached_mem_h; //4k coherent buffer, address to use in hardware

#define DMA_TRANSFER_SIZE	4 //4 bytes
#define DMA_TRANSFER_SRC_V  hps_ocr_vaddress
#define DMA_TRANSFER_DST_V  (hps_ocr_vaddress+8)
#define DMA_PROG_V	    (hps_ocr_vaddress+16)
#define DMA_TRANSFER_SRC_H  HPS_OCR_HADDRESS
#define DMA_TRANSFER_DST_H  (HPS_OCR_HADDRESS+8)
#define DMA_PROG_H	    (HPS_OCR_HADDRESS+16)	    

//platform device 
static struct device pdev;

static void print_src_dst()
{
  int i;
  char* char_ptr_scr = (char*) DMA_TRANSFER_SRC_V;
  char* char_ptr_dst= (char*) DMA_TRANSFER_DST_V;
  
  printk(KERN_INFO "Source=[");
  for (i=0; i<DMA_TRANSFER_SIZE; i++)
  {
    printk(KERN_INFO "%u", ioread8(char_ptr_scr));
    char_ptr_scr++;
    printk(KERN_INFO ",");
  }
  printk(KERN_INFO "]\n");
  
  printk(KERN_INFO "Destiny=[");
  for (i=0; i<DMA_TRANSFER_SIZE; i++)
  {
    printk(KERN_INFO "%u", ioread8(char_ptr_dst));
    char_ptr_dst++;
    printk(KERN_INFO ",");
  }
  printk(KERN_INFO "]\n");
}

static void init_src_dst(char char_val_src, char char_val_dst)
{
  int i;
  char* char_ptr_scr = (char*) DMA_TRANSFER_SRC_V;
  char* char_ptr_dst= (char*) DMA_TRANSFER_DST_V;
  
  printk(KERN_INFO "Source=[");
  for (i=0; i<DMA_TRANSFER_SIZE; i++)
  {
    iowrite8(char_val_src,char_ptr_scr);
    iowrite8(char_val_dst,char_ptr_dst);
    char_ptr_scr++;
    char_ptr_dst++;
  }
}



/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init helloBBB_init(void){
   ALT_STATUS_CODE status;
   ALT_DMA_CHANNEL_STATE_t channel_state;
   ALT_DMA_CHANNEL_FAULT_t fault;
   static ALT_DMA_CHANNEL_t Dma_Channel; //dma channel to be used in transfers
   static ALT_DMA_PROGRAM_t program;
  
   // printk(KERN_INFO "DMA: Hello %s from the BBB LKM!\n", name);
   printk(KERN_INFO "DMA LKM: Initializing module!!\n");
   
   //--Initialize DMA Controller--//
   status = PL330_init();
   if(status == ALT_E_SUCCESS)
   {
       printk(KERN_INFO "DMA LKM: DMAC init was successful\n");
   }else{
       printk(KERN_INFO "DMA LKM: DMAC init failed\n");
       goto error_PL330_init;
   }
   
   //--Allocate DMA Channel--//
   status = alt_dma_channel_alloc_any(&Dma_Channel);
   if (status == ALT_E_SUCCESS)
   {
       printk(KERN_INFO "DMA LKM: DMA channel allocation successful.\n");
   }
   else{
       printk(KERN_INFO "DMA LKM: DMA channel allocation failed.\n");
       goto error_PL330_init;
  }
     
   //--ioremap FPGA memory--//
   //To ioremap the memory in the FPGA so we can access from kernel space
    fpga_ocr_vaddress = ioremap(FPGA_OCR_HADDRESS, FPGA_OCR_SIZE);
    if (fpga_ocr_vaddress == NULL) 
    {
      printk(KERN_INFO "DMA LKM: error doing FPGA OCR ioremap\n");
      goto error_PL330_init;
    }
    else
    {
      printk(KERN_INFO "DMA: DMAC FPGA OCR ioremap success\n");
    }
    
    //--ioremap HPS On-Chip memory--//
   //To ioremap the OCM in the HPS so we can access from kernel space
    hps_ocr_vaddress = ioremap(HPS_OCR_HADDRESS, HPS_OCR_SIZE);
    if (hps_ocr_vaddress == NULL) 
    {
      printk(KERN_INFO "DMA LKM: error doing HPS OCR ioremap\n");
      goto error_HPS_ioremap;
    }
    else
    {
      printk(KERN_INFO "DMA LKM: HPS OCR ioremap success\n");
    }
    
   
   //kmalloc() function allocates cached memory which is 
   //physically contiguous. Use this pointer with ACP transactions
   
   //The dma_alloc_coherent() function allocates non-cached physically
   //contiguous memory. Accesses to the memory by the CPU are the same 
   //as a cache miss when the cache is used. The CPU does not have to 
   //invalidate or flush the cache which can be time consuming

   non_cached_mem_v = dma_alloc_coherent(
		      &pdev, 
		      PAGE_SIZE,
		      &non_cached_mem_h, //address to use from DMAC
		      GFP_KERNEL);
   if (non_cached_mem_v == NULL) {
	printk(KERN_INFO "DMA LKM: allocation of coherent buffer failed\n");
	goto error_dma_alloc_coherent;
   }else{
      printk(KERN_INFO "DMA LKM: allocation of coherent buffer successful\n");
   }

   //-----TRANSFER------//
   printk(KERN_INFO "\n---TRANSFER----\n ");
   printk(KERN_INFO "Buffers initial state: ");
   print_src_dst();
   init_src_dst(0,0);
   printk(KERN_INFO "Buffers after reset: ");
   print_src_dst();
   init_src_dst(2,25);
   printk(KERN_INFO "Buffers initialized: ");
   print_src_dst();
   
   status = alt_dma_memory_to_memory(
	Dma_Channel, 
	&program, 
	(void*)DMA_TRANSFER_DST_H, 
	(void*)DMA_TRANSFER_SRC_H, 
	DMA_TRANSFER_SIZE, 
	false, 
	(ALT_DMA_EVENT_t)0);
   
   // Wait for transfer to complete
    if (status == ALT_E_SUCCESS)
    {
	printk(KERN_INFO "INFO: Waiting for DMA transfer to complete.\n\r");
	channel_state = ALT_DMA_CHANNEL_STATE_EXECUTING;
	while((status == ALT_E_SUCCESS) && (channel_state != ALT_DMA_CHANNEL_STATE_STOPPED))
	{
	    status = alt_dma_channel_state_get(Dma_Channel, &channel_state);
	    if(channel_state == ALT_DMA_CHANNEL_STATE_FAULTING)
	    {
		alt_dma_channel_fault_status_get(Dma_Channel, &fault);
		printk(KERN_INFO "ERROR: DMA CHannel Fault: %d\n\r", (int)fault);
		status = ALT_E_ERROR;
	    }
	}
    }

   printk(KERN_INFO "Buffers after transfer: ");
   print_src_dst();
   
   printk(KERN_INFO "---TRANSFER END----\n\n ");
   //-------------------//
   
   
   
   //End of module init
   printk(KERN_INFO "DMA LKM: Module initialization successful!!\n");
   return 0;

error_dma_alloc_coherent:  
   iounmap(hps_ocr_vaddress);
error_HPS_ioremap:
   iounmap(fpga_ocr_vaddress);
error_PL330_init:
   return 0;
}



/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit helloBBB_exit(void){
   printk(KERN_INFO "DMA LKM: Exiting module!!\n", name);
   iounmap(hps_ocr_vaddress);
   iounmap(fpga_ocr_vaddress);
   PL330_uninit();
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(helloBBB_init);
module_exit(helloBBB_exit);
