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

Description of the code
------------------------

The insertion and removal functions, available in every driver are:

 * dmable_buff_init: executed when the module is inserted using _insmod_. It:

 	* exports the control variables using sysfs in /sys/alloc_dmable_buffers/attributes/,
 	* creates the char device driver interfaces in /dev/dmable_buff<n>/,
 	* configures ACP so it can be used
  * Removes FPGA-to-SDRAM ports from reset so they can be used.
 	* and enables PMU to be accessed from user space (the same performed by [Enable_PMU_user_space](https://github.com/robertofem/CycloneVSoC-examples/tree/master/Linux-modules/Enable_PMU_user_space)).

 * dmable_buff_exit: executed when using _rmmod_. It reverts all what was done by dmable_buff_init so the system remains clean, just exactly the same as before the driver was inserted.

The char device driver interface functions, to be used over /dev/dmable_buff<n> are:
 * dmable_buff_open: called when open() is used. It allocates a buffer with the size and the cache behaviour (cached or uncached) defined by buff_size[n] buff_uncached[n].

 * dmable_buff_write: executed when write() is used. It writes content in the buffer starting at the beginning always.

 * dmable_buff_read: executed when read() is used. It reads content from the buffer starting at the beginning always.

 * dev_release: executed when close() is used. It frees the buffer.

Contents in the folder
----------------------
* alloc_dmable_buffer_LKM.c: main file containing the code just explained before.
* alt_acpidmap.h, alt_address_space.c, alt_address_space.h, hwlib_socal_linux: code needed to enable the ACP. They were copied from [DMA_PL330_LKM](https://github.com/robertofem/CycloneVSoC-examples/tree/master/Linux-modules/DMA_PL330_LKM). More information in the README of that module.
* Makefile: describes compilation process.

Compilation
-------------
To compile the driver you need to first compile the Operating System (OS) you will use to run the driver, otherwise the console will complain that it cannot insert the driver cause the tag of your module is different to the tag of the OS you are running. It does that to ensure that the driver will work. Therefore:

  * Compile the OS you will use. In [tutorials to build a SD card with Operating System](https://github.com/robertofem/CycloneVSoC-examples/tree/master/SD-operating-system) there are examples on how to compile OS and how to prepare the environment to compile drivers.
  * Prepare the make file you will use to compile the module. The makefile provided in this example is prepared to compile using the output of the [Angstrom-v2013.12](https://github.com/robertofem/CycloneVSoC-examples/tree/master/SD-operating-system/Angstrom-v2013.12) compilation process. CROSS_COMPILE contains the path of the compilers used to compile this driver. ROOTDIR is the path to the kernel compiled source. It is used by the driver to get access to the header files used in the compilation (linux/module.h or linux/kernel.h in example).
  * Open a regular terminal (I used Debian 8 to compile Angstrom-v2013.12 and its drivers), navigate until the driver folder and tipe _make_.

The output of the compilation is the file _alloc_dmable_buffer.ko_.

How to test
-----------
The following example shows how to use the driver:  [DMA_transfer_FPGA_DMAC_driver](https://github.com/robertofem/CycloneVSoC-examples/tree/master/Linux-applications/DMA_transfer_FPGA_DMAC_driver).
