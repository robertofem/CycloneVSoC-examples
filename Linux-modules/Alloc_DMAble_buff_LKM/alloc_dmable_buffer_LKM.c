/**
 * @file    FPGA_DMA_LKM.c
 * @author  Roberto Fernandez-Molanes
 * @date    23 Agosto 2018
 * @version 0.1
 * @brief  Module to allocate physically contiguous (DMAble) buffers to be
 * used from application space.
 *
 * When inserting the driver 5 entries to access kernel space buffers and
 * are created. When opening these entries a caceable buffer is allocated
 * is allocated with the size of its parameter <buff_name>_size in /sys.
 * If parameter <buff_name>_uncached is set to 1 before opening the buffer
 * created is uncached. Cached must be used when accessing it from ACP.
 * Uncached when accessing to SDRAM directly. Once created the variable
 * <buff_name>_phys in /sys provides the physical address of the buffer.
 * Using read() and write() functions the buffer can be read or written.
 * The cloce() function frees the buffer.
 *
*/
#include <linux/init.h> // Macros used to mark up functions e.g., __init __exit
#include <linux/module.h>  // Core header for loading LKMs into the kernel
#include <linux/kernel.h> // Contains types, macros, functions for the kernel
#include <asm/io.h>		    // For ioremap and ioread32 and iowrite32
#include <linux/dma-mapping.h>// To use dma_alloc_coherent
#include <linux/slab.h>		  // To use kmalloc
#include <linux/kobject.h>	// Using kobjects for the sysfs bindings
#include <linux/device.h>   // Header to support the kernel Driver Model
#include <linux/fs.h>       // Header for the Linux file system support
#include <asm/uaccess.h>    // Required for the copy to user function

#include "hwlib_socal_linux.h"
#include "alt_address_space.h" //ACP configuration

//data available with modinfo command
MODULE_LICENSE("GPL");//< The license type
MODULE_AUTHOR("Roberto Fernandez (robertofem@gmail.com)");
MODULE_DESCRIPTION("Driver to allocate cached and uncached buffers in kernel space.");
MODULE_VERSION("1.0");

#define DRIVER_NAME "alloc_dmable_buffers"
#define CLASS_NAME "alloc_dmable_buffers"
#define DEV_NAME "dmable_buff"
#define NUM_BUFF 5 //Number of buffers

//---------VARIABLES AND CONSTANTS-----------------//
//SDRAM
//SDRAMC beginning address
#define SDRAMC_REGS 0xFFC20000
#define SDRAMC_REGS_SPAN 0x20000 //128kB
#define FPGAPORTRST 0x5080

// Device driver variables
static int majorNumber;
static struct class* class = NULL;
static struct device* buff[NUM_BUFF] = {NULL, NULL, NULL, NULL, NULL};

//Virtual and physical addresses of the buffers
static void* virt_buff[NUM_BUFF];
static dma_addr_t phys_buff[NUM_BUFF];

// Char device interface function prototypes
static int dmable_buff_open(struct inode *, struct file *);
static int dmable_buff_release(struct inode *, struct file *);
static ssize_t dmable_buff_read(struct file *, char *, size_t, loff_t *);
static ssize_t dmable_buff_write(struct file *, const char *, size_t, loff_t *);
static struct file_operations fops = {
    .open = dmable_buff_open,
    .read = dmable_buff_read,
    .write = dmable_buff_write,
    .release = dmable_buff_release,
};

//Flag if a buffer is open()
int isopen[NUM_BUFF] = {0, 0, 0, 0, 0};

// Sysfs variables
static int buff_size[] = {1024, 1024, 1024, 1024, 1024};
static int buff_uncached[] = {0, 0, 0, 0, 0};

// Sysfs functions
//size
static ssize_t buff0_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return sprintf(buf, "%dB\n", buff_size[0]);
}
static ssize_t buff0_size_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
  sscanf(buf, "%du", &buff_size[0]);
  return count;
}
static ssize_t buff1_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return sprintf(buf, "%dB\n", buff_size[1]);
}
static ssize_t buff1_size_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
  sscanf(buf, "%du", &buff_size[1]);
  return count;
}
static ssize_t buff2_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return sprintf(buf, "%dB\n", buff_size[2]);
}
static ssize_t buff2_size_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
  sscanf(buf, "%du", &buff_size[2]);
  return count;
}
static ssize_t buff3_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return sprintf(buf, "%dB\n", buff_size[3]);
}
static ssize_t buff3_size_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
  sscanf(buf, "%du", &buff_size[3]);
  return count;
}
static ssize_t buff4_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return sprintf(buf, "%dB\n", buff_size[4]);
}
static ssize_t buff4_size_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
  sscanf(buf, "%du", &buff_size[4]);
  return count;
}
//uncached
static ssize_t buff0_uncached_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", buff_uncached[0]);
}
static ssize_t buff0_uncached_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
  sscanf(buf, "%du", &buff_uncached[0]);
  return count;
}
static ssize_t buff1_uncached_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", buff_uncached[1]);
}
static ssize_t buff1_uncached_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
  sscanf(buf, "%du", &buff_uncached[1]);
  return count;
}
static ssize_t buff2_uncached_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", buff_uncached[2]);
}
static ssize_t buff2_uncached_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
  sscanf(buf, "%du", &buff_uncached[2]);
  return count;
}
static ssize_t buff3_uncached_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", buff_uncached[3]);
}
static ssize_t buff3_uncached_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
  sscanf(buf, "%du", &buff_uncached[3]);
  return count;
}
static ssize_t buff4_uncached_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", buff_uncached[4]);
}
static ssize_t buff4_uncached_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
  sscanf(buf, "%du", &buff_uncached[4]);
  return count;
}
static ssize_t buff0_phys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  unsigned int temp = (unsigned int) phys_buff[0];
  return sprintf(buf, "%u\n", temp);
}
static ssize_t buff1_phys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  unsigned int temp = (unsigned int) phys_buff[1];
  return sprintf(buf, "%u\n", temp);
}
static ssize_t buff2_phys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  unsigned int temp = (unsigned int) phys_buff[2];
  return sprintf(buf, "%u\n", temp);
}
static ssize_t buff3_phys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  unsigned int temp = (unsigned int) phys_buff[3];
  return sprintf(buf, "%u\n", temp);
}
static ssize_t buff4_phys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  unsigned int temp = (unsigned int) phys_buff[4];
  return sprintf(buf, "%u\n", temp);
}

static struct kobj_attribute buff0_size_attribute = __ATTR(buff_size[0], 0660, buff0_size_show, buff0_size_store);
static struct kobj_attribute buff1_size_attribute = __ATTR(buff_size[1], 0660, buff1_size_show, buff1_size_store);
static struct kobj_attribute buff2_size_attribute = __ATTR(buff_size[2], 0660, buff2_size_show, buff2_size_store);
static struct kobj_attribute buff3_size_attribute = __ATTR(buff_size[3], 0660, buff3_size_show, buff3_size_store);
static struct kobj_attribute buff4_size_attribute = __ATTR(buff_size[4], 0660, buff4_size_show, buff4_size_store);
static struct kobj_attribute buff0_uncached_attribute = __ATTR(buff_uncached[0], 0660, buff0_uncached_show, buff0_uncached_store);
static struct kobj_attribute buff1_uncached_attribute = __ATTR(buff_uncached[1], 0660, buff1_uncached_show, buff1_uncached_store);
static struct kobj_attribute buff2_uncached_attribute = __ATTR(buff_uncached[2], 0660, buff2_uncached_show, buff2_uncached_store);
static struct kobj_attribute buff3_uncached_attribute = __ATTR(buff_uncached[3], 0660, buff3_uncached_show, buff3_uncached_store);
static struct kobj_attribute buff4_uncached_attribute = __ATTR(buff_uncached[4], 0660, buff4_uncached_show, buff4_uncached_store);
static struct kobj_attribute buff0_phys_attribute = __ATTR(phys_buff[0], 0660, buff0_phys_show, NULL);
static struct kobj_attribute buff1_phys_attribute = __ATTR(phys_buff[1], 0660, buff1_phys_show, NULL);
static struct kobj_attribute buff2_phys_attribute = __ATTR(phys_buff[2], 0660, buff2_phys_show, NULL);
static struct kobj_attribute buff3_phys_attribute = __ATTR(phys_buff[3], 0660, buff3_phys_show, NULL);
static struct kobj_attribute buff4_phys_attribute = __ATTR(phys_buff[4], 0660, buff4_phys_show, NULL);


static struct attribute *buff_attributes[] = {
      &buff0_size_attribute.attr,
      &buff1_size_attribute.attr,
      &buff2_size_attribute.attr,
      &buff3_size_attribute.attr,
      &buff4_size_attribute.attr,
      &buff0_uncached_attribute.attr,
      &buff1_uncached_attribute.attr,
      &buff2_uncached_attribute.attr,
      &buff3_uncached_attribute.attr,
      &buff4_uncached_attribute.attr,
      &buff0_phys_attribute.attr,
      &buff1_phys_attribute.attr,
      &buff2_phys_attribute.attr,
      &buff3_phys_attribute.attr,
      &buff4_phys_attribute.attr,
      NULL,
};

static struct attribute_group attribute_group = {
      .name  = "attributes",
      .attrs = buff_attributes,    ///< The attributes array defined just above
};

static struct kobject *alloc_buff_kobj;

//------INIT AND EXIT FUNCTIONS-----//
static int __init dmable_buff_init(void) {
    int i;
    int result;
    char devname[40] = "";
    void* SDRAMC_virtual_address;
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    const uint32_t ARUSER = 0b11111; //acpidmap:coherent cacheable reads
    const uint32_t AWUSER = 0b11111; //acpidmap:coherent cacheable writes
    int var = 0;

    printk(KERN_INFO DRIVER_NAME": Init\n");

    // Dynamically allocate a major number for the device
    majorNumber = register_chrdev(0, DRIVER_NAME, &fops);
    if (majorNumber < 0) {
        printk(KERN_ALERT DRIVER_NAME": Failed to register a major number\n");
        return 1;
    }
    // Register the device class
    class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(class)) {
        printk(KERN_ALERT DRIVER_NAME": Failed to register device class\n");
        goto error_class_create;
    }
    // Register the buffer entry
    for (i=0; i<NUM_BUFF; i++)
    {
      sprintf(devname, "%s%d\0", DEV_NAME, i);
      buff[i] = device_create(class, NULL, MKDEV(majorNumber, i), NULL, devname);
      if (IS_ERR(buff[i])) {
          printk(KERN_ALERT DRIVER_NAME": Failed to create buffer entry %d\n", i);
          goto error_entry_creation;
      }
    }

    // Export sysfs variables
    // kernel_kobj points to /sys/kernel
    alloc_buff_kobj = kobject_create_and_add(DRIVER_NAME, kernel_kobj->parent);
    if (!alloc_buff_kobj) {
        printk(KERN_INFO DRIVER_NAME": Failed to create kobject mapping\n");
        goto error_entry_creation;
    }
    // add the attributes to /sys/uvispace_camera/attributes
    result = sysfs_create_group(alloc_buff_kobj, &attribute_group);
    if (result) {
        printk(KERN_INFO DRIVER_NAME": Failed to create sysfs group\n");
        goto error_entry_creation;
    }

    //Remove FPGA-to-SDRAMC ports from reset so FPGA can access SDRAM from them
    SDRAMC_virtual_address = ioremap(SDRAMC_REGS, SDRAMC_REGS_SPAN);
    if (SDRAMC_virtual_address == NULL)
    {
      printk(KERN_INFO DRIVER_NAME": error doing SDRAMC ioremap\n");
      goto error_sdram;
    }
    *((unsigned int *)(SDRAMC_virtual_address + FPGAPORTRST)) = 0xFFFF;

    //Configure ACP so FPGA can access it
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
      printk(KERN_INFO DRIVER_NAME": ACP ID Mapper successfully configured.\n");
    else
      printk(KERN_INFO
        DRIVER_NAME": Some ERROR configuring ACP ID Mapper. ACP access may fail.\n");
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
      printk(KERN_INFO DRIVER_NAME": PMU access from user space was correctly enabled.\n");
    else
      printk(KERN_INFO DRIVER_NAME": Error when enablin PMU access from user space .\n");

    return 0;

error_sdram:
    kobject_put(alloc_buff_kobj);
    for(i=0; i<NUM_BUFF; i++)
    {
      device_destroy(class, MKDEV(majorNumber, i));
    }
error_entry_creation:
    class_unregister(class);
    class_destroy(class);
error_class_create:
    unregister_chrdev(majorNumber, DRIVER_NAME);
    return -1;
}

static void __exit dmable_buff_exit(void){
   int i;
   //Undo what init did
   for(i=0; i<NUM_BUFF; i++)
   {
     device_destroy(class, MKDEV(majorNumber, i));
   }
   class_unregister(class);
   class_destroy(class);
   unregister_chrdev(majorNumber, DRIVER_NAME);
   kobject_put(alloc_buff_kobj);
   printk(KERN_INFO DRIVER_NAME": Exiting module!!\n");
}

static int dmable_buff_open(struct inode *inodep, struct file *filep) {
  //Find buffer open with the minor number
  int i;
  i = iminor(filep->f_path.dentry->d_inode);

  if (isopen[i] == 0)
  {
    if (buff_uncached[i] == 0)
    {
      //Allocate cached buffer in buff[i]
      virt_buff[i] = kmalloc(buff_size[i], (GFP_DMA | GFP_ATOMIC));
      if (virt_buff[i] == NULL) {
   	    printk(KERN_INFO DRIVER_NAME": allocation of cached buffer %d failed\n", i);
      }else{
         printk(KERN_INFO DRIVER_NAME": allocation of cached buffer %d successful\n", i);
      }
      //get the physical address of this buffer
      phys_buff[i] = virt_to_phys((volatile void*) virt_buff[i]);
    }
    else
    {
      //Allocate uncached buffer in buff[i]
      virt_buff[i] = dma_alloc_coherent(
   		      //&pdev,
   		      NULL,
   		      //PAGE_SIZE,
   		      buff_size[i], ////Max in Angstrom and CycloneVSoC is 4MB
   		      &phys_buff[i], //address to use from DMAC
   		      GFP_KERNEL);
      if (virt_buff[i] == NULL) {
   	    printk(KERN_INFO DRIVER_NAME": allocation of cached buffer %d failed\n", i);
   	    return -1;
      }else{
         printk(KERN_INFO DRIVER_NAME": allocation of cached buffer %d successful\n", i);
      }
    }
    isopen[i] = 1;
  }
  return 0;
}

static ssize_t dmable_buff_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
  int error_count = 0;

  //Find buffer open with the minor number
  int i;
  i = iminor(filep->f_path.dentry->d_inode);

  if (isopen[i] == 1)
  {
    error_count = copy_to_user(buffer, virt_buff[i], len);
    if (error_count!=0){ // if true then have success
       printk(KERN_INFO KERN_INFO DRIVER_NAME": Failed to read %d characters from buffer %d to the user\n", error_count, i);
       return -EFAULT;  // Failed -- return a bad address message (i.e. -14)
    }
  }
  return 0;
}

static ssize_t dmable_buff_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
  int error_count = 0;

  //Find buffer open with the minor number
  int i;
  i = iminor(filep->f_path.dentry->d_inode);

  if (isopen[i] == 1)
  {
    error_count = copy_from_user(virt_buff[i], buffer, len);
    if (error_count!=0){ // if true then have success
       printk(KERN_INFO DRIVER_NAME": Failed to write %d characters from the user to buffer %d\n", error_count, i);
       return -EFAULT;  // Failed -- return a bad address message (i.e. -14)
    }
  }
  return 0;
}

static int dmable_buff_release(struct inode *inodep, struct file *filep) {
  //Find buffer open with the minor number
  int i;
  i = iminor(filep->f_path.dentry->d_inode);

  if (isopen[i] == 1)
  {
    if (buff_uncached[i] == 0)
    {
      kfree(virt_buff[i]);
    }
    else
    {
      dma_free_coherent(NULL, buff_size[i], virt_buff[i], phys_buff[i]);
    }
    isopen[i] = 0;
  }
  return 0;
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time (insmod) and the cleanup function
 * (rmmod).
 */
module_init(dmable_buff_init);
module_exit(dmable_buff_exit);
