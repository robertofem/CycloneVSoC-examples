Alloc_DMAble_buff_LKM
============

Introduction
-------------
This Loadable Kernel Module (LKM) allocates up to five physically contiguous buffers
in kernel space and provides their physical addresses through sysfs. This buffers are intended to be used as intermediate buffers in DMA transfers. Access to the buffer
from the application is provided through the character device interface.

When inserted 5 entries are created in /dev: /dev/dmable_buff<n> with n from 0 to 4.
Using this entries each buffer can be accessed as a file. When using open()
the buffer is allocated. When using read() and write() the buffer is read or written, respectively. When using close() the buffer is freed.

To control the behaviour of the buffers there is a set of sysfs variables created in /sys/alloc_dmable_buffers/attributes that can be accessed from console or from an application (also using open(), write(), read() and close()):

* buff_size[n] with n from 0 to 4. They define the size of the buffer. Use it before using open() in /dev/dmable_buff<n> because thats the moment the buffer is allocated.
It is 1024 by default.
* buff_uncached[n] with n from 0 to 4. It defines if the buffer is cached (buff_uncached[n] = 0) or uncached (buff_uncached[n] = 1). Use cached to access through
ACP and uncached to directly access the SDRAM controller (L3->SDRAMC or FPGA-SDRAMC ports). Use it before using open() in /dev/dmable_buff<n> because thats the moment the buffer is allocated and its type defined. 0 by default.
* phys_buff[n] with n from 0 to 4. It provides the physical address of the allocated buffer. Use it after using open() in /dev/dmable_buff<n>. This address should be used
by the hardware (i.e. a DMA COntroller in FPGA) to access the buffer. Remember to add 0x80000000 to this address if ACP is used.


-----write after here------


Description of the code
------------------------

The insertion and removal functions, available in every driver are:

 * DMA_PL330_LKM_init: executed when the module is inserted using _insmod_. It:

 	* initializes the DMA Controller and reserves Channel 0 to be used in DMA transactions,
 	* ioremaps HPS On-Chip RAM (is is used to store the DMAC microcode),
 	* allocates uncached buffer using dma_alloc_coherent() (to be used when use_acp=0),
 	* allocates cached buffer using kmalloc(),
 	* exports the control variables using sysfs in /sys/dma_pl330/,
 	* creates the char device driver interface in /dev/dma_pl330/,
 	* configures ACP
 	* and enables PMU to be accessed from user space (the same performed by [Enable_PMU_user_space](https://github.com/robertofem/CycloneVSoC-examples/tree/master/Linux-modules/Enable_PMU_user_space)).

 * DMA_PL330_LKM_exit: executed when using _rmmod_. It reverts all what was done by DMA_PL330_LKM_init so the system remains clean, just exactly the same as before the driver was inserted.

The char device driver interface functions are:
 * dev_open: called when open() is used. It prepares the DMA write and read microcode if prepare_microcode_in_open=1. To prepare the write microcode it uses cached buffer (if use_acp=1) or uncached buffer (use_acp=0) as source,  dma_buff_padd as destiny and dma_transfer_size as transfer size. To prepare the read microcode destiny and source buffers are swapped.

 * dev_write: when  using write() function the data is copied from the application using _copy_from_user()_ function to cached buffer (if use_acp=1) or uncached buffer (use_acp=0). Later a transfer from the buffer to the memory in the FPGA using the PL330 DMA Controller. If prepare_microcode_in_open=1 the microcode programmed in dev_open is used to perform the transfer. If prepare_microcode_in_open=0 a new microcode is prepared using the size parameter passed in dev_write function as size for the DMA transfer.To program the PL330 transfer, the functions of the Altera´s hwlib were modified to work in kernel space (they are designed for baremetal apps so the modification basically consists in ioremap the hardware addresses of the DMA Controller so the functions for baremetal work inside the virtual memory environment used in the LKM). Better method would be to use "platform device" API to get information on the DMA from device tree and later use "DMA-engine" API to program the DMA transfer. However those APIs didn´t work and we were forced to do a less generic driver. Probably the DMA-engine options should be activated during compilation of the kernel but we were not able to do it.

 * dev_read: called when using read() to read from the FPGA. It does the same as write in opossite direction. First the DMA transfer copies data from FPGA into the cached or uncached buffer and then this data is copied to application space using _copy_to_user()_.

 * dev_release: called when callin the close() function from the application. Does nothing.

Contents in the folder
----------------------
* DMA_PL330_LKM.c: main file containing the code just explained before.
* Modifications to the hwlib functions:
    * alt_dma.c and alt_dma.h: functions to control the DMAC (all the functions not used in our program in alt_dma.c were commented to minimize the errors compiling.).
    *  alt_dma_common.h: few declarations for DMA.
    *  alt_dma_periph_cv_av.h: some macro declarations.
    *  alt_dma_program.c and alt_dma_program.h: to generate the microcode program for the DMAC.
    *  alt_acpidmap.h, alt_address_space.c and alt_address_map.h: enable the ACP ID Mapper and configure ACP.
    *  hwlib_socal_linux: All the generic files used in the files for all peripherals (hwlib.h, socal.h, etc.) were not copied to the folder of the driver. Copying this files gives a lot of errors that need long time to fix. So instead of fixing generic files we commented the include lines for generic files in the beginning of the files previously enumerated and copied all macros that these files need into one single file called hwlib_socal_linux.h. This file includes definitions from hwlib.h, alt_rstmgr.h, socal/hps.h, socal/alt_sysmgr.h , alt_cache.h and alt_mmu.h.
* Makefile: describes compilation process.

Compilation
-------------
To compile the driver you need to first compile the Operating System (OS) you will use to run the driver, otherwise the console will complain that it cannot insert the driver cause the tag of your module is different to the tag of the OS you are running. It does that to ensure that the driver will work. Therefore:

  * Compile the OS you will use. In [tutorials to build a SD card with Operating System](https://github.com/robertofem/CycloneVSoC-examples/tree/master/SD-operating-system) there are examples on how to compile OS and how to prepare the environment to compile drivers.
  * Prepare the make file you will use to compile the module. The makefile provided in this example is prepared to compile using the output of the [Angstrom-v2013.12](https://github.com/robertofem/CycloneVSoC-examples/tree/master/SD-operating-system/Angstrom-v2013.12) compilation process. CROSS_COMPILE contains the path of the compilers used to compile this driver. ROOTDIR is the path to the kernel compiled source. It is used by the driver to get access to the header files used in the compilation (linux/module.h or linux/kernel.h in example).
  * Open a regular terminal (I used Debian 8 to compile Angstrom-v2013.12 and its drivers), navigate until the driver folder and tipe _make_.

The output of the compilation is the file _DMA_PL330.ko_.

How to test
-----------
Run the [Test_DMA_PL330_LKM](https://github.com/robertofem/CycloneVSoC-examples/tree/master/Linux-applications/Test_DMA_PL330_LKM) example.
