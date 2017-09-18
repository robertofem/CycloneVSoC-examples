DMA_PL330_LKM
============

Introduction
-------------
This Loadable Kernel Module (LKM) moves data between a Linux Application running in user space and a memory or other kind of peripheral in the FPGA using the DMA Controller PL330 available in the HPS. For data sizes bigger than 128B this method is faster than moving data with the processor using memcpy().

The module uses the _char driver_ interface to connect application space and the kernel space. It creates a node in _/dev_ called _**/dev/dma_pl330**_ and support for the the typical file functions is given:   open(), close(), write() and read(). This way reading or writing to an FPGA address using the DMA is as easy as reading or writing into a file. The LKM also exports some variables using sysfs in _**/sys/dma_pl330/**_ to control its behaviour. The application [Test_DMA_PL330_LKM](https://github.com/robertofem/CycloneVSoC-examples/tree/master/Linux-applications/Test_DMA_PL330_LKM) is an example on how to use this driver.

Using this driver DMA can move data between processor memories and FPGA in two different ways, depicted in the following figure:

<p align="center">
  <img src="https://github.com/robertofem/CycloneVSoC-examples/raw/master/Linux-modules/DMA_PL330_LKM/DMA_Data_Paths.png" width="750" align="middle" alt="Cyclone V SoC simplified block diagram" />
</p>

When using ACP data is copied into L2 cache controller in coherent way so it is automatically coherent for the processor. When accessing through SDRAM controller data is not coherent between processor and DMA. The processor must flush the cache before (when DMA writes to processor memory) or after (when DMA  reads from processor memory) DMA access processor memory. To solve this problem the operating system has functions to allocate coherent cached or non-cached bufferes so coherency is always ensured and we don´t have to deal with cache flushsing. These functions are:

 * kmalloc(): allocates a physically contiguous buffer cached in memory. This kind of buffer should be used when accessing through ACP.
 * dma_alloc_coherent(): allocates a physically contiguous buffer in un-cached memory. This kind of buffer should be used when accessing processor memories though L3-to-SDRAM Controller port. When accessing this buffer from processor the operating system is in charge of flushing the cache when needed so data is always retrieved from external SDRAM and therefore is coherent with data written or to be read by the DMA. The Operating System does that for us automatically.

This functions are only available in kernel space and that is the main reason why a LKM is needed to do DMA transfers when using Operating System. The malloc() function available from application space is equivalent to vmalloc() in kernel space. These functions alloc memory that is virtually contiguous but maybe not physically contiguous.

Description of the code
------------------------
The LKM contains the following *variables to control* its behaviour. This variables are exported to the file system using sysfs (in /sys/dma_pl330/). This variables control the basic behaviour of the transfer:

* use_acp: When 0 the  PL330 DMAC will will use the port connecting L3 and SDRAMC. When 1 the access is through ACP port.

* prepare_microcode_in_open: PL330 DMA Controller executes a microcode defining the DMA transfer to be done. When prepare_microcode_in_open = 0, the microcode is prepared before every transfer when entering the write() or read() function. When prepare_microcode_in_open = 1 the microcode is prepared when calling the open() function (two microcodes are generated: one for read FPGA and another for write to FPGA). Later when using read() or write() the prepared microcodes are used. This saves the microcode preparation time when doing the transfer. This is important since DMA microcode preparation time goes from DMAC 10% of the transfer time (for data sizes between 128kB and 2MB) to 75% (for data sizes between 2B and 8kB).

* dma_transfer_size: Size of the DMA transfer in Bytes. Only used when prepare_microcode_in_open = 1. Otherwise the size of the DMA transfer is the size passed as argument in read() and write() functions.

* dma_buff_padd: This is the physical address in the FPGA were data is going to be written when using write() or read when using read().

The following variables are also exported through sysfs but give access to advances low-level features that can deteriorate or improve the transfer and other task running in CPU depending on several aspects like data size, CPU task load, etc. It is recommended not to use these features unless you know what you are doing. The advanced sysfs variables are:

* lockdown_cpu: writing to this variable specific ways of the L2 8-way associative cache controller can be locked for CPU0 or CPU1. For example. Writting  0b00000101 in this field will lock ways 0 and 2 of the cache controller. That means that CPU0 and CPU1 wont be able to write in these 2 ways. Read from these ways its allowed. This permits for example to reserve two ways of the cache for exclusive usage by the ACP and whatever the ACP writes in cache is going to reside in cache for sure (unless size is bigger than those two ways). This will make that CPU0 and CP1 can read faster the data ACP is writing because it will be for sure in cache. Otherwise the CPUs could use these two ways and send to external SDRAM data that the ACP is writing.

* lockdown_acp: the same as lockdown_cpu but for ACP port.

* sdramc_priority: represents the mppriority register of the SDRAM Controller. Writing to this register automatically writes into mppriority register in the SDRAM Controller. Using this register the priority of the SDRAM ports can be changed. It can be used to improve the transfer speed when accessing thorugh the L3-SDRAM port. Elevating the priority for this port compared to the CPU ports makes that the L3-SDRAM port is always granted access in case CPU and L3-SDRAM ports access simultaneously to the SDRAM controller. The bit fields in this variable are the same explained in the Cyclone V SoC Handbook for the mppriority register. GO there for more details.

* sdramc_weight0: writing to this variable writes in the mpweight_0_4 register of the SDRAM controller. Changing to this reg the bandwidth granted for ports accessing simultaneously to the SDRAM controller can be changed. Similar to changing port priority. See Cyclone V SOC Handbook for more details.

* sdramc_weight1:  writing to this variable writes in the mpweight_1_4 register of the SDRAM controller.

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

Possible improvements to be done:
 * Lock (when calling dev_open) and unlock (when calling dev_release) so the driver cannot be open more than once at a time.
 * Augment the number of channel used by the DMAC (PL330 has 8 DMA channels that can work simultaneously). One idea could be to use one channel each time an application opens the driver and lock the open when the number of opens reaches 8. This ways up to 8 different applications could be making usage of the DMAC.
 * leave read and write functions before transfer ends so the application calling the driver can keep doing operations with CPU. This could be done leaving functions just after the transfer starts and using interrupts instead of polling to tell the driver that transfer has finished. And a sysfs variable could be used to notify to the application using the driver that the transfer is over.

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
