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
#include <linux/device.h>           // Header to support the kernel Driver Model
#include <linux/fs.h>             // Header for the Linux file system support
#include <asm/uaccess.h>          // Required for the copy to user function

#include "hwlib_socal_linux.h"
#include "alt_dma.h"
#include "alt_dma_common.h"
#include "alt_address_space.h" //ACP configuration


MODULE_LICENSE("GPL");              ///< The license type -- this affects runtime behavior
MODULE_AUTHOR("Roberto Fernandez (robertofem@gmail.com)");      ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("PL330 DMA Controller driver for communication between application and FPGA.");  ///< The description -- see modinfo
MODULE_VERSION("1.0");              ///< The version of the module

//---------VARIABLES AND CONSTANTS FOR BUFFERS-----------------//
//Buffers
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
#define DMA_PROG_WR_V (hps_ocr_vaddress+16)//virtual address of the DMAC microcode program
#define DMA_PROG_WR_H (HPS_OCR_HADDRESS+16)//hardware address of the DMAC microcode program
#define DMA_PROG_RD_V (hps_ocr_vaddress+256+16)//virtual address of the DMAC microcode program
#define DMA_PROG_RD_H (HPS_OCR_HADDRESS+256+16)//hardware address of the DMAC microcode program
static ALT_DMA_CHANNEL_t Dma_Channel; //dma channel to be used in transfers

//---------VARIABLES TO EXPORT USING SYSFS-----------------//
static int use_acp = 0; //to use acp (add 0x80000000) when accessing processors RAM
static int prepare_microcode_in_open = 0;//microcode program is prepared when opening char driver
//when prepare_microcode_in_open the following vars are used to prepare DMA microcodes in open() func
static void* dma_buff_padd = (void*) 0xC0000000;//physical address of buff to use in write and read from application
static int dma_transfer_size = 0; //transfer size in Bytes of the DMA transaction

//------VARIABLES TO IMPLEMENT THE CHAR DEVICE DRIVER INTERFACE--------//
#define  DEVICE_NAME "dma_pl330"    ///< The device will appear at /dev/dma_ch0 using this value
#define  CLASS_NAME  "dma_class"        ///< The device class -- this is a character device driver
static int    majorNumber;                  ///< Stores the device number -- determined automatically
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  dma_Class  = NULL; ///< The device-driver class struct pointer
static struct device* dma_Device = NULL; ///< The device-driver device struct pointer
//The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
//available operations on char device driver: open, read, write and close
static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};

//-----------------FUNCTIONS TO INITIALIZE DMA-------------------//
ALT_STATUS_CODE PL330_init(void)
{
  int i;
  ALT_STATUS_CODE status = ALT_E_SUCCESS;
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


//--------------FUNCTIONS AND STRUCTS TO EXPORT SOME VARIABLE WITH SYSFS---------//
static ssize_t dma_buff_padd_store(struct kobject *kobj, struct kobj_attribute *attr,
                                   const char *buf, size_t count){
  unsigned int temp;
  sscanf(buf, "%u", &temp);
  dma_buff_padd = (void*) temp;
  return count;
}

static ssize_t dma_buff_padd_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
  unsigned int temp = (unsigned int) dma_buff_padd;
  return sprintf(buf, "%u\n", temp);
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

static ssize_t dma_transfer_size_store(struct kobject *kobj, struct kobj_attribute *attr,
                                   const char *buf, size_t count){
  sscanf(buf, "%du", &dma_transfer_size);
  return count;
}

static ssize_t dma_transfer_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
   return sprintf(buf, "%d\n", dma_transfer_size);
}

/**  Use these helper macros to define the name and access levels of the kobj_attributes
 *  The kobj_attribute has an attribute attr (name and mode), show and store function pointers
 */
static struct kobj_attribute dma_buff_padd_attr = __ATTR(dma_buff_padd, 0666, dma_buff_padd_show, dma_buff_padd_store);
static struct kobj_attribute use_acp_attr = __ATTR(use_acp, 0666, use_acp_show, use_acp_store);
static struct kobj_attribute prepare_microcode_in_open_attr = __ATTR(prepare_microcode_in_open, 0666, 
  prepare_microcode_in_open_show, prepare_microcode_in_open_store);
static struct kobj_attribute dma_transfer_size_attr = __ATTR(dma_transfer_size, 0666, 
  dma_transfer_size_show, dma_transfer_size_store);

/**  The pl330_lkm_attrs[] is an array of attributes that is used to create the attribute group below.
 *  The attr property of the kobj_attribute is used to extract the attribute struct
 */
static struct attribute *pl330_lkm_attrs[] = {
      &dma_buff_padd_attr.attr,                  
      &use_acp_attr.attr,                  
      &prepare_microcode_in_open_attr.attr, 
      &dma_transfer_size_attr.attr, 
      NULL,
};
 
/**  The attribute group uses the attribute array and a name, which is exposed on sysfs -- in this
 *  case it is gpio115, which is automatically defined in the ebbButton_init() function below
 *  using the custom kernel parameter that can be passed when the module is loaded.
 */
static struct attribute_group attr_group = {
      .name  = "pl330_lkm_attrs",  
      .attrs = pl330_lkm_attrs,    ///< The attributes array defined just above
};
 
static struct kobject *pl330_lkm_kobj;


//-----------------LKM CHAR DEVICE DRIVER INTERFACE FUNCTIONS---------------//

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep){
   void* dma_transfer_src_h;//hardware address of the source buffer 
   void* dma_transfer_dst_h;//hardware address of the destiny buffer 
   ALT_STATUS_CODE status;

   numberOpens++;
   
   //A good module would get a resource de device cannot be open a second time before closing it
   
   if (prepare_microcode_in_open == 1)
   {
      //Prepare program for writes (WR)
      dma_transfer_dst_h = dma_buff_padd; 
      if (use_acp == 0) //not use use_acp
        dma_transfer_src_h = (void*) non_cached_mem_h;
      else //use acp
        dma_transfer_src_h = (void*)((char*)cached_mem_h + 0x80000000);
      
      status = alt_dma_memory_to_memory_only_prepare_program(
        Dma_Channel, 
	       (ALT_DMA_PROGRAM_t*) DMA_PROG_WR_V, 
	       (ALT_DMA_PROGRAM_t*) DMA_PROG_WR_H,
	       dma_transfer_dst_h,
	       dma_transfer_src_h, 
	       (size_t) dma_transfer_size, 
	       false, 
	       (ALT_DMA_EVENT_t)0);
      
      //Prepare program for reads (RD)
      dma_transfer_src_h = dma_buff_padd; 
      if (use_acp == 0) //not use use_acp
	       dma_transfer_dst_h = (void*) non_cached_mem_h;
      else //use acp
	       dma_transfer_dst_h = (void*)((char*)cached_mem_h + 0x80000000);
      
      status = alt_dma_memory_to_memory_only_prepare_program(
	       Dma_Channel, 
	       (ALT_DMA_PROGRAM_t*) DMA_PROG_RD_V, 
	       (ALT_DMA_PROGRAM_t*) DMA_PROG_RD_H,
	       dma_transfer_dst_h,
	       dma_transfer_src_h, 
	       (size_t) dma_transfer_size, 
	       false, 
	       (ALT_DMA_EVENT_t)0);
   }
   
   return 0;
}
 
/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
  ALT_STATUS_CODE status;
  ALT_DMA_CHANNEL_STATE_t channel_state;
  ALT_DMA_CHANNEL_FAULT_t fault;
  int error_count = 0;
  void* dma_transfer_src_h;//hardware address of the source buffer 
  void* dma_transfer_dst_h;//hardware address of the destiny buffer 
  
  //Copy data from hardware buffer (FPGA) to the application memory
  if (prepare_microcode_in_open == 1)
  {
    //execute the program prepared in the open
    status = alt_dma_channel_exec(Dma_Channel, (ALT_DMA_PROGRAM_t*) DMA_PROG_RD_H);
  }
  else
  {
    //generate and execute a new program using the len as size 

    //Prepare program for reads (RD)
    dma_transfer_src_h = dma_buff_padd; 
    if (use_acp == 0) //not use use_acp
      dma_transfer_dst_h = (void*)non_cached_mem_h;
    else //use acp
      dma_transfer_dst_h = (void*)((char*)cached_mem_h + 0x80000000);
    
    status = alt_dma_memory_to_memory(
  	Dma_Channel, 
  	(ALT_DMA_PROGRAM_t*) DMA_PROG_RD_V, 
  	(ALT_DMA_PROGRAM_t*) DMA_PROG_RD_H,
  	dma_transfer_dst_h,
  	dma_transfer_src_h, 
  	len, 
  	false, 
  	(ALT_DMA_EVENT_t)0);
  }
  
  //Wait for the transfer to be finished
  channel_state = ALT_DMA_CHANNEL_STATE_EXECUTING;
   
  if (status == ALT_E_SUCCESS)
  {
    while((status == ALT_E_SUCCESS) && (channel_state != ALT_DMA_CHANNEL_STATE_STOPPED))
      {
	status = alt_dma_channel_state_get(Dma_Channel, &channel_state);
	if(channel_state == ALT_DMA_CHANNEL_STATE_FAULTING)
	{
	  alt_dma_channel_fault_status_get(Dma_Channel, &fault);
	  printk(KERN_INFO "DMA LKM: ERROR! DMA Channel Fault: %d\n", (int)fault);
	  return ALT_E_ERROR;
	}
      }
   }
   else
   {
     printk(KERN_INFO "DMA LKM: ERROR! DMA Transfer failed!\n");
     return ALT_E_ERROR;
   }
   
   //Copy the software buffer into user (application) space
   if (use_acp == 0) //not use use_acp
      error_count = copy_to_user(buffer, non_cached_mem_v, len);
   else //use acp
      error_count = copy_to_user(buffer, cached_mem_v, len);
   
   if (error_count!=0){ // if true then have success
      printk(KERN_INFO "DMA LKM: Failed to send %d characters to the user in read function\n", error_count);
      return -EFAULT;  // Failed -- return a bad address message (i.e. -14)
   }
    
  return 0;
}
 
/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
  ALT_STATUS_CODE status;
  ALT_DMA_CHANNEL_STATE_t channel_state;
  ALT_DMA_CHANNEL_FAULT_t fault;
  int error_count = 0;
  void* dma_transfer_src_h;//hardware address of the source buffer 
  void* dma_transfer_dst_h;//hardware address of the destiny buffer 
  int i;
  
  /*if (use_acp==1) printk("offset=%d len=%d\n", (int) offset, (int)len);

  if (use_acp==1){
    printk("Cached buff before copy_from_user=[");
    for (i=0; i<len+2; i++) printk("%d,",*((char*)(cached_mem_v)+i));
    printk("]\n");
  }*/

  //Copy data from user (application) space to a DMAble buffer
   if (use_acp == 0) //not use use_acp
      error_count = copy_from_user(non_cached_mem_v, buffer, len);
   else //use acp
      error_count = copy_from_user(cached_mem_v, buffer, len);
  
   if (error_count!=0){ // if true then have success
      printk(KERN_INFO "DMA LKM: Failed to copy %d characters from the user in write function\n", error_count);
      return -EFAULT;  // Failed -- return a bad address message (i.e. -14)
   }
  
  /*if (use_acp==1){
    printk("Cached buff after copy_from_user=[");
    for (i=0; i<len+2; i++) printk("%d,",*((char*)(cached_mem_v)+i));
    printk("]\n");
  }*/
  
  //Copy data from hardware buffer (FPGA) to the application memory
  if (prepare_microcode_in_open == 1)
  {
    //execute the program prepared in the open
    status = alt_dma_channel_exec(Dma_Channel, (ALT_DMA_PROGRAM_t*) DMA_PROG_WR_H);
  }
  else
  {
    //generate and execute a new program using the len as size 
    //Prepare program for writes (WR)
    dma_transfer_dst_h = dma_buff_padd; 
    if (use_acp == 0) //not use use_acp
      dma_transfer_src_h = (void*) non_cached_mem_h;
    else //use acp
      dma_transfer_src_h = (void*)((char*)cached_mem_h + 0x80000000);
    
    status = alt_dma_memory_to_memory(
    	Dma_Channel, 
    	(ALT_DMA_PROGRAM_t*) DMA_PROG_WR_V, 
    	(ALT_DMA_PROGRAM_t*) DMA_PROG_WR_H,
    	dma_transfer_dst_h,
    	dma_transfer_src_h, 
    	len, 
    	false, 
    	(ALT_DMA_EVENT_t)0);
  }
  
  //Wait for the transfer to be finished
  channel_state = ALT_DMA_CHANNEL_STATE_EXECUTING;
   
  if (status == ALT_E_SUCCESS)
  {
    while((status == ALT_E_SUCCESS) && (channel_state != ALT_DMA_CHANNEL_STATE_STOPPED))
    {
    	status = alt_dma_channel_state_get(Dma_Channel, &channel_state);
    	if(channel_state == ALT_DMA_CHANNEL_STATE_FAULTING)
    	{
    	  alt_dma_channel_fault_status_get(Dma_Channel, &fault);
    	  printk(KERN_INFO "DMA LKM: ERROR! DMA Channel Fault: %d\n", (int)fault);
    	  return ALT_E_ERROR;
    	}
    }
  }
 else
 {
   printk(KERN_INFO "DMA LKM: ERROR! DMA Transfer failed!\n");
   return ALT_E_ERROR;
 }
  
  return 0;
}
/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 */
static int dev_release(struct inode *inodep, struct file *filep){
   //in a decent driver unlock here the driver so the resource is free
   return 0;
}

//------------------------LKM init and exit functions-----------------------//
/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init DMA_PL330_LKM_init(void){
   ALT_STATUS_CODE status;
   int result = 0;
   const uint32_t ARUSER = 0b11111; //acpidmap:coherent cacheable reads
   const uint32_t AWUSER = 0b11111; //acpidmap:coherent cacheable writes
   int var = 0; 

   printk(KERN_INFO "DMA LKM: Initializing module!!\n");
   
   //--Initialize DMA Controller--//
   status = PL330_init();
   if(status == ALT_E_SUCCESS)
   {
       printk(KERN_INFO "DMA LKM: DMAC init was successful\n");
   }else{
       printk(KERN_INFO "DMA LKM: DMAC init failed\n");
       goto error_HPS_ioremap;
   }
   
   //--Allocate DMA Channel--//
   status = alt_dma_channel_alloc_any(&Dma_Channel);
   if (status == ALT_E_SUCCESS)
   {
       printk(KERN_INFO "DMA LKM: DMA channel allocation successful.\n");
   }
   else{
       printk(KERN_INFO "DMA LKM: DMA channel allocation failed.\n");
       goto error_HPS_ioremap;
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
    
   //--Allocate uncached buffer--//
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

   //--Allocate cached buffer--//
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
   
   //--export some variables using sysfs--//
   // create the kobject sysfs entry at /sys/dma_pl330
   pl330_lkm_kobj = kobject_create_and_add("dma_pl330", kernel_kobj->parent); // kernel_kobj points to /sys/kernel
   if(!pl330_lkm_kobj){
      printk(KERN_INFO "DMA LKM:failed to create kobject mapping\n");
      goto error_kobject_mapping;
   }else{
      printk(KERN_INFO "DMA LKM: kobject creation successful\n");
   }
   // add the attributes to /sys/dma_pl330/pl330_lkm_attrs 
   result = sysfs_create_group(pl330_lkm_kobj, &attr_group);
   if(result) {
      printk(KERN_INFO "DMA LKM:failed to create sysfs group\n");
      goto error_kobject_group;
   }else{
      printk(KERN_INFO "DMA LKM: sysfs creation successfull\n");
   }
   
   //--Create the char device driver interface--//
   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_INFO "DMA LKM: failed to register a major number\n");
      goto error_kobject_group;
   }
   printk(KERN_INFO "DMA LKM: char device registered correctly with major number %d\n", majorNumber);
   // Register the device class
   dma_Class = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(dma_Class)){                // Check for error and clean up if there is
      printk(KERN_INFO "DMA LKM: Failed to register device class\n");
      goto error_create_dev_class;
   }
   printk(KERN_INFO "DMA LKM: char device class registered correctly\n");
   // Register the device driver
   dma_Device = device_create(dma_Class, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(dma_Device)){               // Clean up if there is an error
      
      printk(KERN_INFO "DMA LKM: Failed to create the device\n");
      goto error_create_dev;
   }
   printk(KERN_INFO "DMA LKM: device successfully created in node: /dev/%s\n", DEVICE_NAME);
   //printk(KERN_INFO DEVICE_NAME);
   //printk(KERN_INFO "\n"); 

  //--ACP configuration--//
  //Do ioremap to be able to acess hw regs from inside the module
  alt_acpidmap_iomap();
  //print_acpidmap_regs();
  result = 1;
  //Set output ID3 for dynamic reads and ID4 for dynamic writes
  status = alt_acp_id_map_dynamic_read_set(ALT_ACP_ID_OUT_DYNAM_ID_3);
  if (status!=ALT_E_SUCCESS) result = 0;
  status = alt_acp_id_map_dynamic_write_set(ALT_ACP_ID_OUT_DYNAM_ID_4);
  if (status!=ALT_E_SUCCESS) result = 0;
  //Configure the page and user write sideband signal options that are applied 
  //to all write transactions that have their input IDs dynamically mapped.
  status = alt_acp_id_map_dynamic_read_options_set(ALT_ACP_ID_MAP_PAGE_0, ARUSER);
  if (status!=ALT_E_SUCCESS) result = 0;
  status = alt_acp_id_map_dynamic_write_options_set(ALT_ACP_ID_MAP_PAGE_0, AWUSER);
  if (status!=ALT_E_SUCCESS) result = 0;
  //print_acpidmap_regs();
  if (result==1)
    printk(KERN_INFO "DMA LKM: ACP ID Mapper successfully configured.\n");
  else
    printk(KERN_INFO 
      "DMA LKM: Some ERROR configuring ACP ID Mapper. ACP access may fail.\n");
  alt_acpidmap_iounmap();

  //--Enable PMU from user space setting PMUSERENR.EN bit--//
  //read PMUSERENR
  asm volatile("mrc p15, 0, %[value], c9, c14, 0":[value]"+r" (var));
  //pr_info("PMU User Enable register=%d\n", var);//print PMUSERENR
  //Set PMUSERENR.EN 
  var = 1;
  asm volatile("mcr p15, 0, %[value], c9, c14, 0"::[value]"r" (var));
  //read PMUSERENR
  var = 2;
  asm volatile("mrc p15, 0, %[value], c9, c14, 0":[value]"+r" (var));
  //pr_info("PMU User Enable register=%d\n", var);//print PMUSERENR
  if (var == 1)
    printk(KERN_INFO "DMA LKM: PMU access from user space was correctly enabled.\n");
  else
    printk(KERN_INFO "DMA LKM: Error when enablin PMU access from user space .\n");

  //--End of module init--//
  printk(KERN_INFO "DMA LKM: Module initialization successful!!\n");
  return 0;
   
error_create_dev:  
   class_destroy(dma_Class); 
error_create_dev_class:
   unregister_chrdev(majorNumber, DEVICE_NAME);
error_kobject_group:
  kobject_put(pl330_lkm_kobj);   
error_kobject_mapping:
  kfree(cached_mem_v);
error_kmalloc:
  dma_free_coherent(NULL, (NON_CACHED_MEM_SIZE), non_cached_mem_v, non_cached_mem_h);
error_dma_alloc_coherent:  
   iounmap(hps_ocr_vaddress);
error_HPS_ioremap:
   return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 *  We clean-up memory allocation and mappings here.
 */
static void __exit DMA_PL330_LKM_exit(void){
   printk(KERN_INFO "Sysfs values: dma_buff_p:%x, use_acp:%d, prepare_microcode_in_open:%d, dma_transfer_size:%d\n", 
    (unsigned int) dma_buff_padd, use_acp, prepare_microcode_in_open, dma_transfer_size);
   printk(KERN_INFO "DMA LKM: Exiting module!!\n");
   //Undo what init did
   device_destroy(dma_Class, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(dma_Class);                          // unregister the device class
   class_destroy(dma_Class);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   kobject_put(pl330_lkm_kobj); 
   kfree(cached_mem_v);
   dma_free_coherent(NULL, (NON_CACHED_MEM_SIZE), non_cached_mem_v, non_cached_mem_h);
   iounmap(hps_ocr_vaddress);         
   PL330_uninit();
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time (insmod) and the cleanup function 
 * (rmmod). 
 */
module_init(DMA_PL330_LKM_init);
module_exit(DMA_PL330_LKM_exit);