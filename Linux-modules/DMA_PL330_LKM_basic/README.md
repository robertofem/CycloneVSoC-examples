DMA_PLL330_LKM_basic
================

Introduction
-------------
This Loadable Kernel Module (LKM) module makes a data transfer using the PL330 DMAC (available in HPS) when inserted into the operating system. It is a stand-alone driver and doesn´t need an application to test it. It is a complete example that can be configured to move data between all kinds of buffers you can find in the system, namely: FPGA memory, HPS On-chip RAM (HPS-OCR), uncached buffer in processor´s RAM and cached buffer in processor´s RAM (through APC). So it can be used as starting point for developing a DMA module for a specific application. This module was programmed before doing [DMA_PL330_LKM](https://github.com/robertofem/CycloneVSoC-examples/tree/master/Linux-modules/DMA_PL330_LKM) to  debug the DMA transfer functions and test DMA transfers with different kinds of buffers. 

When doing a DMA transfer the concepts [virtual and physical address](https://www.kernel.org/doc/Documentation/DMA-API-HOWTO.txt) of a buffer are needed. Physical address is the real address needed by a hardware element (DMA Controller, FPGA component, etc.) to access the buffer. However the module lives in a virtual space and the physical location of its code and variables is not known until it is inserted (_insmod_) and the Operating System assigns a portion of physical memory to it. All variables inside the driver are therefore virtual and when accessing them, the Memory Management Unit (MMU) makes a translation from virtual to physical memory to read/write its value from/to memory. Virtual addresses will be used by this module to access buffers and check if the DMA transfer was correct while physical addresses will be used by the DMA Controller to make the transfer.

Other useful concept is [data coherency](https://en.wikipedia.org/wiki/Cache_coherence). In a system with different memories like Cyclone V SoC the same data can reside simultaneously in several different places (Cache L1, Cache L2, external SDRAM memory, FPGA peripheral, HPS On-Chip RAM(HPS-OCR), etc.). When one component generates data it is important that the consumer of that data accesses to an updated copy of it. When that happens it is said that data coherency exists. Managing data coherency in software is very time consuming and difficult when there are several applications running simultaneously and sharing data. For this reason manufacturers add hardware components that ensure data coherency between some of the parts of the system. In the case of Cyclone V SoC, A Snoop Control Unit (SCU) ensures coherency among the two L1 caches, L2 cache, and SDRAM. However to maintain coherency between processors and other elements there are two methods: 

 * **Using a cached buffer**: the LKM allocates a physically contiguous buffer cached in memory using **kmalloc()**.  To move data from FPGA and this buffer, the DMAC writes or reads  the processor memories using the Acceleration Coherency Port (**ACP**) and data coherency is automatically ensured by the SCU exactly as if other processor is connected to the memory system. 
 * **Using a non-cached buffer**: the LKM uses **dma_alloc_coherent()** to allocate a physically contiguous buffer in un-cached memory (only resides in external SDRAM).  To access this buffer the DMAC should use the L3-to-SDRAMC port. To ensure coherency the processor must flush the caches before (when DMA writes to processor memory) or after (when DMA  reads from processor memory) DMA access processor memory. When using this kind of buffer the processor transparently does that for us. 

To access to a variable in processor memory through ACP, the value 0x80000000 should be added to its physical address. That makes L3 to redirect the bus read or write through ACP instead of using L3-to-SDRAMC port.

kmalloc() and dma_alloc_coherent() are only available in kernel space and that is the main reason why a LKM is needed to do DMA transfers when using Operating System. The malloc() function available from application space is equivalent to vmalloc() in kernel space. These functions allocate memory that is virtually contiguous but maybe not physically contiguous.

To test this LKM using a buffer in FPGA, the [FPGA_OCR_256K](https://github.com/robertofem/CycloneVSoC-examples/tree/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K) can be used.

Description of the code
---------------------------
This LKM only contains the two most basic functions: _init_ and _exit_ functions. They perform the followng tasks:

* DMA_PL330_LKM_init: executed when the module is inserted using _insmod_. It:
 
    * initializes the DMA Controller and reserves Channel 0 to be used in DMA transactions, 
    * [ioremap](http://www.makelinux.net/ldd3/chp-9-sect-4)s the FPGA buffer´s physical address to obtain its virtual address and be able to access from within the driver (this LKM is prepared to work with [FPGA_OCR_256K](https://github.com/robertofem/CycloneVSoC-examples/tree/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K) FPGA hardware and the physical address of the buffer in the FPGA is 0xC0000000),
    * ioremaps HPS On-Chip RAM to obtain a virtual address and be able to access from within the driver (HPS-OCR is used to perform transfer and also to store the microcode used by the DMA Controller to perform the DMA transfers),
    * allocates un-cached buffer using dma_alloc_coherent() (accessed through L3-to-SDRAMC port),
    * allocates cached buffer using kmalloc() (accessed through ACP port),
    * configures ACP,
    * sets different data in source and destination buffers,
    * performs a DMA transfer between source and destination buffers (source and destination can be any of the 4 types of buffer: FPGA buffer, HPS On-chip RAM (HPS-OCR), uncached buffer in processor´s RAM and cached buffer in processor´s RAM),
    * waits for the DMA transfer to be finished polling a bit in the DMAC
    * and compares the content of source and destination buffers to check if the transfer has been correctly done.
 
* DMA_PL330_LKM_exit: executed when using _rmmod_. It reverts all what was done by DMA_PL330_LKM_init so the system remains clean, just exactly the same as before the driver was inserted.

To control the transfer the following macros are used:

```c
#define MESSAGE "32B transfer from Cached buffer in Processor RAM to FPGA using ACP. DMA microcode program in HPS OCR.\n"
#define DMA_TRANSFER_SIZE (32) //Size of DMA transfer in Bytes
#define DMA_TRANSFER_SRC_V (cached_mem_v)  //virtual address of the source buffer
#define DMA_TRANSFER_DST_V (fpga_ocr_vaddress //virtual address of the destiny buffer
#define DMA_PROG_V (hps_ocr_vaddress+16) //virtual address of the DMAC microcode program
#define DMA_TRANSFER_SRC_H (cached_mem_h+0x80000000) //hardware address of the source buffer
#define DMA_TRANSFER_DST_H FPGA_OCR_HADDRESS //hardware address of the destiny buffer 
#define DMA_PROG_H (HPS_OCR_HADDRESS+16) //hardware address of the DMAC microcode program	 
```

In the previous block of code a transfer between a cached buffer in processor memory and a buffer in the FPGA is configured. The MESSAGE is shown before the transfer starts. DMA_TRANSFER_SIZE defines the size in Bytes of the transfer. The rest of the variables represent the virtual (to access from inside the LKM) and physical (to access from the FPGA) adresses for the source and destiny buffers and the buffer that will contain the microcode program used by the DMAC to perform this transfer. As it can be seen, 0x80000000 are added to the physical address of the cached buffer in order the DMAC to access it through ACP. 

Using the previous macros, the main file of this LKM defines 4 different types of example transfers :

 1. 4B transfer from HPS OCR to HPS OCR. DMA microcode program in OCR.
 2. 64B transfer from HPS OCR to FPGA OCR. DMA microcode program in OCR.
 3. 256B transfer from Uncached buffer in Processor RAM to FPGA OCR.  DMA microcode program in OCR.
 4. 32B transfer from Cached buffer in Processor RAM to FPGA OCR using ACP.  DMA microcode program in OCR.

<p align="center">
  <img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/Linux-modules/DMA_PL330_LKM_basic/Four-examples.png" width="800" align="middle" alt="Cyclone V SoC simplified block diagram" />
</p>

The dashed lines in example 4 mean that the this line will be used only sometimes. For example when reading data using ACP, if data is in caches it is inmediately served. However it it is not available the L2 controller needs to access the external SDRAM. The code in the repository comes prepared to run the example number 1 (moves data in the HPS-OCR).  Uncommenting the macros for other examples they can be also tested. 

In all these examples the DMA microcode is stored in HPS-OCR for ease of programming. The reader can locate it in a cached or un-cached buffer in the processor memory (it would be a more logical place for it). 

Contents in the folder
----------------------
* DMA_PL330_LKM_basic.c: main file containing the code just explained before.
* Modifications to the hwlib functions:

    * alt_dma.c and alt_dma.h: functions to control the DMAC (all the functions not used in our program in alt_dma.c were commented to minimize the errors compiling.).
    * alt_dma_common.h: few declarations for DMA.
    * alt_dma_periph_cv_av.h: some macro declarations.
    * alt_dma_program.c and alt_dma_program.h: to generate the microcode program for the DMAC.
    * alt_acpidmap.h, alt_address_space.c and alt_address_map.h: enable the ACP ID Mapper and configure ACP. 
    * hwlib_socal_linux: All the generic files used in the files for all peripherals (hwlib.h, socal.h, etc.) were not copied to the folder of the driver. Copying this files gives a lot of errors that need long time to fix. So instead of fixing generic files we commented the include lines for generic files in the beginning of the files previously enumerated and copied all macros that these files need into one single file called hwlib_socal_linux.h. This file includes definitions from hwlib.h, alt_rstmgr.h, socal/hps.h, socal/alt_sysmgr.h , alt_cache.h and alt_mmu.h. 
 
* Makefile: describes compilation process.

Compilation
-------------
To compile the driver you need to first compile the Operating System (OS) you will use to run the driver, otherwise the console will complain that it cannot insert the driver cause the tag of your module is different to the tag of the OS you are running. It does that to ensure that the driver will work. Therefore:

  * Compile the OS you will use. In [tutorials to build a SD card with Operating System](https://github.com/robertofem/CycloneVSoC-examples/tree/master/SD-operating-system) there are examples on how to compile OS and how to prepare the environment to compile drivers. 
  * Prepare the make file you will use to compile the module. The makefile provided in this example is prepared to compile using the output of the [Angstrom-v2013.12](https://github.com/robertofem/CycloneVSoC-examples/tree/master/SD-operating-system/Angstrom-v2013.12) compilation process. CROSS_COMPILE contains the path of the compilers used to compile this driver. ROOTDIR is the path to the kernel compiled source. It is used by the driver to get access to the header files used in the compilation (linux/module.h or linux/kernel.h in example).
  * Open a regular terminal (I used Debian 8 to compile Angstrom-v2013.12 and its drivers), navigate until the driver folder and type _make_.
 
The output of the compilation is the file _DMA_PL330_basic.ko_.

How to test
------------
* Configure MSEL pins:
    *  MSEL[5:0]="000000" position when FPGA will be configured from SD card.
    *  MSEL[5:0]="110010" position when FPGA will be configured from EPCQ device or Quartus programmer.
* Switch on the board.
* Compile the FPGA hardware ([FPGA_OCR_256K](https://github.com/robertofem/CycloneVSoC-examples/tree/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K)  in example) and load it in the FPGA:
    *  If MSEL[5:0]="000000" FPGA is loaded by the U-boot during start-up. Check  the [tutorials to build a SD card with Operating System](https://github.com/robertofem/CycloneVSoC-examples/tree/master/SD-operating-system) to learn how to configure the SD card so the FPGA is loaded from it. 
    *  If MSEL[5:0]="110010" use Quartus to load the FPGA:
        *  Connect the USB cable (just next to the power connector).
        *  Open Quartus programmer.
        *  Click Autodetect -> Mode JTAG -> Select 5CSEMA5 (for DE1-SoC and DE0-nano-SoC) if asked -> Right click in the line representing the FPGA -> Change FIle -> Select the .sof file for the project you want to load -> tick Program/Configure -> Click Start.

* Connect the serial console port (the mini-USB port in DE1-SoC) to the computer and open a session with a Serial Terminal (like Putty) at 115200 bauds. Now you have access to the board OS console.
* Copy the [DMA_PL330_LKM](https://github.com/robertofem/CycloneVSoC-examples/tree/master/Linux-modules/DMA_PL330_LKM) module in the SD card (using SSH or connecting it to a regular computer running Linux system) and insert it into the kernel using _insmod_ command: 
```bash
  $ insmod DMA_PL330_basic.ko
```
 * The result (fail or success of the transfer) is printed in the kernel log. Depending on the configuration of your operating system the messages from the LKM will be directly printed in screen or you will need to use _dmesg_ to see them:
 ```bash
  $ dmesg
```
