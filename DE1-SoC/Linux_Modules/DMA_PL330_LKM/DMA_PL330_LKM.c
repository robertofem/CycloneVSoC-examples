/**
 * @file    DMA_PL330_LKM.c
 * @author  Roberto Fernandez-Molanes
 * @date    5 Junio 2017
 * @version 0.1
 * @brief  Module to control HPS DMA Controller PL330 from application.
 * PL330. 
 * 
 * Do the description
 * 
*/
#include <linux/init.h>             // Macros used to mark up functions e.g., __init __exit
#include <linux/module.h>           // Core header for loading LKMs into the kernel
#include <linux/kernel.h>           // Contains types, macros, functions for the kernel
#include <asm/io.h>		    // For ioremap and ioread32 and iowrite32
#include <linux/dma-mapping.h>	    // To use dma_alloc_coherent
#include <linux/slab.h>		    // To use kmalloc 
#include <linux/kobject.h>	    // Using kobjects for the sysfs bindings

#include "hwlib_socal_linux.h"
#include "alt_dma.h"
#include "alt_dma_common.h"

MODULE_LICENSE("GPL");              ///< The license type -- this affects runtime behavior
MODULE_AUTHOR("Roberto Fernandez (robertofem@gmail.com)");      ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("PL330 DMA Controller driver for communication between application and FPGA.");  ///< The description -- see modinfo
MODULE_VERSION("1.0");              ///< The version of the module

//---------VARIABLES AND CONSTANTS FOR BUFFERS-----------------//
//Buffers
//FPGA On-CHip RAM
#define FPGA_OCR_HADDRESS 0xC0000000 //Hardware address
#define FPGA_OCR_SIZE	  0x40000    //Size in bytes=256KB
static void* fpga_ocr_vaddress;       //virtual address
//HPS On-Chip RAM
#define HPS_OCR_HADDRESS 0xFFFF0000 //Hardware address 
#define HPS_OCR_SIZE	 0x10000    //Size in bytes=64kB
static void* hps_ocr_vaddress;       //virtual address 
//Non cached DMAable physically contiguous buffer in main RAM
static void* non_cached_mem_v; 	//virtual address, to be used in module
static dma_addr_t non_cached_mem_h; //hardware address, to be used in hardware
#define NON_CACHED_MEM_SIZE (2*1024*1024) //Size. Max using dma_alloc_coherent in Angstrom and CycloneVSoC is 4MB
//Cached physically contiguous buffer in main RAM. DMAable only through ACP
static void* cached_mem_v; 	//virtual address, to be used in module
static phys_addr_t cached_mem_h; //hardware address, to be used in hardware
#define CACHED_MEM_SIZE (2*1024*1024) //Size. Max using kmalloc in Angstrom and CycloneVSoC is 4MB

//---------VARIABLES TO EXPORT USING SYSFS-----------------//
static void* dma_buff_h = (void*) FPGA_OCR_HADDRESS; //hardware address to use in write and read from application
static int use_acp = 0; //to use acp (add 0x80000000) when accessing processors RAM
static int prepare_microcode_in_open = 0;//microcode program is prepared when opening char driver

//-------------VARIABLES TO DO DMA TRANSFER----------------//
//IMPORTANT!!!!!
//-The address of the DMA program + 16B must always be aligned to a 32B address.
// This is because in DMA_PROG_V and DMA_PROG_H addresses we specify the location of an
// struct containing the actual DMA microcode. The first 16B of this struct holds some
// variables and the real microcode starts in Byte 16 of the struct. So in the end the
// real microcode ends 16B after DMA_PROG_V and DMA_PROG_H. The 32B alignment of the
// microcode is needed by the DMAC to fetch it.
//-The source and destiny addresses must be aligned to a multiple of the transfer size.
// If the transfer size is 32kB the the address of these buffers must be aligned to
// 32kB.The allocation functions automatically do this for us but when using hardware
// buffers like HPS-OCR or FPGA we must take care of this. In our case we aligned the
// FPGA-OCR with the start of the HPS-FPGA bridge that is GB aligned so we ensure a
// problem related to this arises.
static size_t dma_transfer_size; //Size of DMA transfer in Bytes
static void* dma_transfer_src_v;//virtual address of the source buffer
static void* dma_transfer_dst_v;//virtual address of the destiny buffer
static void* dma_transfer_src_h;//hardware address of the source buffer 
static void* dma_transfer_dst_h;//hardware address of the destiny buffer 
#define DMA_PROG_V (hps_ocr_vaddress+16)//virtual address of the DMAC microcode program
#define DMA_PROG_H (HPS_OCR_HADDRESS+16)//hardware address of the DMAC microcode program

/*
//--------------------------Extra functions definition------------------------------//
static void print_src_dst()
{
  int i;
  char* char_ptr_scr = (char*) DMA_TRANSFER_SRC_V;
  char* char_ptr_dst= (char*) DMA_TRANSFER_DST_V;
  
  printk( "Source=[");
  for (i=0; i<DMA_TRANSFER_SIZE; i++)
  {
    printk( "%u", ioread8(char_ptr_scr));
    char_ptr_scr++;
    if (i<(DMA_TRANSFER_SIZE-1)) printk(",");
  }
  printk("]\n");
  
  printk( "Destiny=[");
  for (i=0; i<DMA_TRANSFER_SIZE; i++)
  {
    printk("%u", ioread8(char_ptr_dst));
    char_ptr_dst++;
    if (i<(DMA_TRANSFER_SIZE-1)) printk(",");
  }
  printk( "]\n");
}

static void init_src_dst(char char_val_src, char char_val_dst)
{
  int i;
  char* char_ptr_scr = (char*) DMA_TRANSFER_SRC_V;
  char* char_ptr_dst= (char*) DMA_TRANSFER_DST_V;
  
  for (i=0; i<DMA_TRANSFER_SIZE; i++)
  {
    iowrite8(char_val_src,char_ptr_scr);
    iowrite8(char_val_dst,char_ptr_dst);
    char_ptr_scr++;
    char_ptr_dst++;
  }
}

static ALT_STATUS_CODE compare_src_dst()
{
  int i;
  char* char_ptr_scr = (char*) DMA_TRANSFER_SRC_V;
  char* char_ptr_dst= (char*) DMA_TRANSFER_DST_V;
  ALT_STATUS_CODE error = ALT_E_SUCCESS;
  
  for (i=0; i<DMA_TRANSFER_SIZE; i++)
  {
    if ( ioread8(char_ptr_scr) != ioread8(char_ptr_dst))
    {
    	error = ALT_E_ERROR;
	printk("i:%d, src:%u, dst%u\n", i, ioread8(char_ptr_scr), ioread8(char_ptr_dst));
    }
    char_ptr_scr++;
    char_ptr_dst++;
  }
  return error;
}

static void print_dma_program()
{
    int i;
    char* char_ptr = (char*) DMA_PROG_V;
    for (i = 0; i < sizeof(ALT_DMA_PROGRAM_t); i++)
    {
        
	printk("%02X ",  ioread8(char_ptr)); // gives 12AB
        if (i==5) printk(" code_size:");
        if (i==7) printk(" ");
        if (i==15) printk(" prog:");
	char_ptr++;
    }
    printk("\n");
}
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

static ssize_t dma_buff_h_store(struct kobject *kobj, struct kobj_attribute *attr,
                                   const char *buf, size_t count){
  sscanf(buf, "%du", &dma_buff_h);
  return count;
}

static ssize_t dma_buff_h_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
   return sprintf(buf, "%d\n", dma_buff_h);
}

static ssize_t use_acp_store(struct kobject *kobj, struct kobj_attribute *attr,
                                   const char *buf, size_t count){
  sscanf(buf, "%du", &use_acp);
  return count;
}

static ssize_t use_acp_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
   return sprintf(buf, "%d\n", use_acp);
}

static ssize_t prepare_microcode_in_open_store(struct kobject *kobj, struct kobj_attribute *attr,
                                   const char *buf, size_t count){
  sscanf(buf, "%du", &prepare_microcode_in_open);
  return count;
}

static ssize_t prepare_microcode_in_open_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
   return sprintf(buf, "%d\n", prepare_microcode_in_open);
}

/**  Use these helper macros to define the name and access levels of the kobj_attributes
 *  The kobj_attribute has an attribute attr (name and mode), show and store function pointers
 */
static struct kobj_attribute dma_buff_h_attr = __ATTR(dma_buff_h, 0666, dma_buff_h_show, dma_buff_h_store);
static struct kobj_attribute use_acp_attr = __ATTR(use_acp, 0666, use_acp_show, use_acp_store);
static struct kobj_attribute prepare_microcode_in_open_attr = __ATTR(prepare_microcode_in_open, 0666, 
  prepare_microcode_in_open_show, prepare_microcode_in_open_store);

/**  The pl330_lkm_attrs[] is an array of attributes that is used to create the attribute group below.
 *  The attr property of the kobj_attribute is used to extract the attribute struct
 */
static struct attribute *pl330_lkm_attrs[] = {
      &dma_buff_h_attr.attr,                  ///< The number of button presses
      &use_acp_attr.attr,                  ///< Is the LED on or off?
      &prepare_microcode_in_open_attr.attr, ///< Time of the last button press in HH:MM:SS:NNNNNNNNN
      NULL,
};
 
/**  The attribute group uses the attribute array and a name, which is exposed on sysfs -- in this
 *  case it is gpio115, which is automatically defined in the ebbButton_init() function below
 *  using the custom kernel parameter that can be passed when the module is loaded.
 */
static struct attribute_group attr_group = {
      .name  = "PL330_LKM_ATTRS",  
      .attrs = pl330_lkm_attrs,    ///< The attributes array defined just above
};
 
static struct kobject *pl330_lkm_kobj;

//-----------------LKM init and exit functions----------------//
/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init DMA_PL330_LKM_init(void){
   ALT_STATUS_CODE status;
   ALT_DMA_CHANNEL_STATE_t channel_state;
   ALT_DMA_CHANNEL_FAULT_t fault;
   static ALT_DMA_CHANNEL_t Dma_Channel; //dma channel to be used in transfers
   ALT_DMA_PROGRAM_t* program = (ALT_DMA_PROGRAM_t*) DMA_PROG_V;
   int i;
   int result = 0;
  
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
      printk(KERN_INFO "DMA LKM: DMAC FPGA OCR ioremap success\n");
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
    
   //Allocate uncached buffer
   //The dma_alloc_coherent() function allocates non-cached physically
   //contiguous memory. Accesses to the memory by the CPU are the same 
   //as a cache miss when the cache is used. The CPU does not have to 
   //invalidate or flush the cache which can be time consuming.

   non_cached_mem_v = dma_alloc_coherent(
		      //&pdev,
		      NULL,
		      //PAGE_SIZE,
		      (NON_CACHED_MEM_SIZE), ////Max in Angstrom and CycloneVSoC is 4MB
		      &non_cached_mem_h, //address to use from DMAC
		      GFP_KERNEL);
   if (non_cached_mem_v == NULL) {
	printk(KERN_INFO "DMA LKM: allocation of non-cached buffer failed\n");
	goto error_dma_alloc_coherent;
   }else{
      printk(KERN_INFO "DMA LKM: allocation of non-cached buffer successful\n");
   }

   //Allocate cached buffer
   //kmalloc() function allocates cached memory which is 
   //physically contiguous. Use kmalloc with ACP transactions
   cached_mem_v = kmalloc(CACHED_MEM_SIZE, (GFP_DMA | GFP_ATOMIC));
   if (cached_mem_v == NULL) {
	printk(KERN_INFO "DMA LKM: allocation of cached buffer failed\n");
	goto error_kmalloc;
   }else{
      printk(KERN_INFO "DMA LKM: allocation of cached buffer successful\n");
   }
   //get the physical address of this buffer
   cached_mem_h = virt_to_phys((volatile void*) cached_mem_v);
   
   // create the kobject sysfs entry at /sys/dma_pl330
   pl330_lkm_kobj = kobject_create_and_add("dma_pl330", kernel_kobj->parent); // kernel_kobj points to /sys/kernel
   if(!pl330_lkm_kobj){
      printk(KERN_INFO "DMA LKM:failed to create kobject mapping\n");
      goto error_kobject_mapping;
   }
   // add the attributes to /sys/dma_pl330/ 
   result = sysfs_create_group(pl330_lkm_kobj, &attr_group);
   if(result) {
      printk(KERN_INFO "DMA LKM:failed to create sysfs group\n");
      goto error_kobject_group;
   }
   
   
  /* 
   //-----DMA TRANSFER------//
   printk(KERN_INFO "\n---TRANSFER----\n ");
   printk(KERN_INFO MESSAGE);printk("\n");
   printk( "Buffers initial state: \n");
   print_src_dst();
   init_src_dst(0,0);
   printk(KERN_INFO "Buffers after reset: \n");
   print_src_dst();
   init_src_dst(3,25);
   printk(KERN_INFO "Buffers initialized: \n");
   print_src_dst();
   
   //print_dma_program();//used in debugging
   
   status = alt_dma_memory_to_memory(
	Dma_Channel, 
	(ALT_DMA_PROGRAM_t*) DMA_PROG_V, 
	(ALT_DMA_PROGRAM_t*) DMA_PROG_H,
	(void*)DMA_TRANSFER_DST_H, 
	(void*)DMA_TRANSFER_SRC_H, 
	DMA_TRANSFER_SIZE, 
	false, 
	(ALT_DMA_EVENT_t)0);
   printk(KERN_INFO "DMA Transfer in progress\n");
   
   printk(KERN_INFO "INFO: Waiting for DMA transfer to complete.\n");
   channel_state = ALT_DMA_CHANNEL_STATE_EXECUTING;
   
   if (status == ALT_E_SUCCESS)
   {
      while((status == ALT_E_SUCCESS) && (channel_state != ALT_DMA_CHANNEL_STATE_STOPPED))
      {
	  status = alt_dma_channel_state_get(Dma_Channel, &channel_state);
	  if(channel_state == ALT_DMA_CHANNEL_STATE_FAULTING)
	  {
	    alt_dma_channel_fault_status_get(Dma_Channel, &fault);
	    printk(KERN_INFO "ERROR: DMA Channel Fault: %d\n", (int)fault);
	    status = ALT_E_ERROR;
	  }
      }
   }
   else
   {
     printk(KERN_INFO "ERROR: DMA Transfer failed!\n");
     goto error_dma_transfer;
   }
        
   printk(KERN_INFO "Buffers after transfer:\n");
   print_src_dst();
   
   //print_dma_program();//used in debugging

   //Compare destiny and source buffers
   status = compare_src_dst();
   if(status == ALT_E_SUCCESS) printk(KERN_INFO "Transfer was successful!\n");
   else printk(KERN_INFO "ERROR during the transfer!\n");
   
   printk(KERN_INFO "---TRANSFER END----\n ");
   //-------------------//
   */
  
   //End of module init
   printk(KERN_INFO "DMA LKM: Module initialization successful!!\n");
   return 0;

error_kobject_group:
  kobject_put(pl330_lkm_kobj);   
error_kobject_mapping:
  kfree(cached_mem_v);
error_kmalloc:
  dma_free_coherent(NULL, (NON_CACHED_MEM_SIZE), non_cached_mem_v, non_cached_mem_h);
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
 *  We clean-up memory allocation and mappings here.
 */
static void __exit DMA_PL330_LKM_exit(void){
   printk(KERN_INFO "DMA LKM: Exiting module!!\n");
   kobject_put(pl330_lkm_kobj); 
   kfree(cached_mem_v);
   dma_free_coherent(NULL, (NON_CACHED_MEM_SIZE), non_cached_mem_v, non_cached_mem_h);
   iounmap(hps_ocr_vaddress);
   iounmap(fpga_ocr_vaddress);
   PL330_uninit();
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time (insmod) and the cleanup function 
 * (rmmod). 
 */
module_init(DMA_PL330_LKM_init);
module_exit(DMA_PL330_LKM_exit);
