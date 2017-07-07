/**
 * @file    DMA_PL330_LKM_basic.c
 * @author  Roberto Fernandez-Molanes
 * @date    10 April 2017
 * @version 0.1
 * @brief  A basic version of a module that does transfers using the HPS DMA Controller 
 * PL330. 
 * 
 * In this version only module init and exit functions are implemented. First in 
 * the init function some buffers are mapped using iomap, namely: a 256kB memory in the 
 * FPGA  and  the 64kB in the HPS On-chip RAM. Iomap provides a mapping between 
 * hardware addresses and the virtual addresses used inside the module so this
 * hardware elements can be accessed from within the module code. Later a non cached 
 * buffer in processor memory is allocated using dma_alloc_coherent that provides a
 * DMAable physically contiguous non-cached portion of processor memory. A cached 
 * physically  contiguous cached buffer is allocated with kmalloc(). This buffer can
 * be used in DMA only if the DMAC accesses through ACP accesses coherently to memory. 
 * This example is provided using the HPS on-chip RAM as source and destiny for the 
 * transfers and to store the DMA program. The other buffers are left as example so the 
 * reader can play with the location of buffers and move data from FPGA to processor, 
 * OCR to processor, etc. After memory regions preparation the PL330 is initialized and 
 * a DMA channel is allocated. After that a DMA transfer is done calling to 
 * alt_dma_memory_to_memory() to perform the transfer. 
 * 
 * In the exit function all the mapping and allocation performed in init function is
 * undone to left the OS as it was before the init.
 * 
*/
#include <linux/init.h>             // Macros used to mark up functions e.g., __init __exit
#include <linux/module.h>           // Core header for loading LKMs into the kernel
#include <linux/kernel.h>           // Contains types, macros, functions for the kernel
#include <asm/io.h>		    // For ioremap and ioread32 and iowrite32
#include <linux/dma-mapping.h>	    // To use dma_alloc_coherent
#include <linux/slab.h>		    // To use kmalloc 

#include "hwlib_socal_linux.h"
#include "alt_dma.h"
#include "alt_dma_common.h"
#include "alt_address_space.h" //ACP configuration

MODULE_LICENSE("GPL");              ///< The license type -- this affects runtime behavior
MODULE_AUTHOR("Roberto Fernandez (robertofem@gmail.com)");      ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("PL330 DMA Controller basic driver.");  ///< The description -- see modinfo
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

//--------------VARIABLES TO DEFINE THE TRANSFER-EXAMPLES----------------//
//
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
//

//4B transfer from HPS OCR to HPS OCR. DMA microcode program in OCR.
#define MESSAGE "4B transfer from HPS OCR to HPS OCR. DMA microcode program in HPS OCR.\n"
#define DMA_TRANSFER_SIZE   4 //Size of DMA transfer in Bytes
#define DMA_TRANSFER_SRC_V  hps_ocr_vaddress //virtual address of the source buffer
#define DMA_TRANSFER_DST_V  (hps_ocr_vaddress+8)//virtual address of the destiny buffer
#define DMA_PROG_V	    (hps_ocr_vaddress+16)//virtual address of the DMAC microcode program
#define DMA_TRANSFER_SRC_H  HPS_OCR_HADDRESS //hardware address of the source buffer 
#define DMA_TRANSFER_DST_H  (HPS_OCR_HADDRESS+8)//hardware address of the destiny buffer 
#define DMA_PROG_H	    (HPS_OCR_HADDRESS+16)//hardware address of the DMAC microcode program	    

/*
//64B transfer from HPS OCR to FPGA OCR. DMA microcode program in OCR.
#define MESSAGE "64B transfer from HPS OCR to FPGA OCR. DMA microcode program in HPS OCR.\n"
#define DMA_TRANSFER_SIZE   64 //Size of DMA transfer in Bytes
#define DMA_TRANSFER_SRC_V  (hps_ocr_vaddress+1024) //virtual address of the source buffer
#define DMA_TRANSFER_DST_V  (fpga_ocr_vaddress)//virtual address of the destiny buffer
#define DMA_PROG_V	    (hps_ocr_vaddress+16)//virtual address of the DMAC microcode program
#define DMA_TRANSFER_SRC_H  (HPS_OCR_HADDRESS+1024) //hardware address of the source buffer
#define DMA_TRANSFER_DST_H  (FPGA_OCR_HADDRESS)//hardware address of the destiny buffer 
#define DMA_PROG_H	    (HPS_OCR_HADDRESS+16)//hardware address of the DMAC microcode program	    
*/
/*
//256B transfer from Uncached buffer in Processor RAM to FPGA OCR.  DMA microcode program in OCR.
#define MESSAGE "256B transfer from Uncached buffer in Processor RAM to FPGA. DMA microcode program in HPS OCR.\n"
#define DMA_TRANSFER_SIZE   (256) //Size of DMA transfer in Bytes
#define DMA_TRANSFER_SRC_V  (non_cached_mem_v)  //virtual address of the source buffer
#define DMA_TRANSFER_DST_V  fpga_ocr_vaddress //virtual address of the destiny buffer
#define DMA_PROG_V	    (hps_ocr_vaddress+16)//virtual address of the DMAC microcode program
#define DMA_TRANSFER_SRC_H  non_cached_mem_h//hardware address of the source buffer
#define DMA_TRANSFER_DST_H  FPGA_OCR_HADDRESS//hardware address of the destiny buffer 
#define DMA_PROG_H	    (HPS_OCR_HADDRESS+16)//hardware address of the DMAC microcode program	    
*/
/*
//32B transfer from Cached buffer in Processor RAM to FPGA OCR using ACP.  DMA microcode program in OCR.
//To use ACP the hardware address of a buffer in processors RAM must be added 0x80000000
#define MESSAGE "32B transfer from Cached buffer in Processor RAM to FPGA using ACP. DMA microcode program in HPS OCR.\n"
#define DMA_TRANSFER_SIZE   (32) //Size of DMA transfer in Bytes
#define DMA_TRANSFER_SRC_V  (cached_mem_v)  //virtual address of the source buffer
#define DMA_TRANSFER_DST_V  fpga_ocr_vaddress //virtual address of the destiny buffer
#define DMA_PROG_V	    (hps_ocr_vaddress+16)//virtual address of the DMAC microcode program
#define DMA_TRANSFER_SRC_H  (cached_mem_h+0x80000000)//hardware address of the source buffer
#define DMA_TRANSFER_DST_H  FPGA_OCR_HADDRESS//hardware address of the destiny buffer 
#define DMA_PROG_H	    (HPS_OCR_HADDRESS+16)//hardware address of the DMAC microcode program	    
*/

//---------------------Extra functions-------------------------//
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
   const uint32_t ARUSER = 0b11111; //acpidmap:coherent cacheable reads
   const uint32_t AWUSER = 0b11111; //acpidmap:coherent cacheable writes
  
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
   
   //End of module init
   printk(KERN_INFO "DMA LKM: Module initialization successful!!\n");
   return 0;
   
error_dma_transfer:
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
