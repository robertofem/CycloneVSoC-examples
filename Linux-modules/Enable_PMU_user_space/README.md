Enable_PMU_User_Space
=====================

Introduction
-------------
 PMU (Performance Monitoring Unit) is a coprocessor tightly coupled with ARM CPUs. It is in charge of gathering statistics from the processor, i.e. the number of  exceptions, divisions by 0, etc. Each processor core has its own PMU. In case of Cyclone V SoC the processor has two cores and therefore two PMUs. When using Operating System (OS) there are 2 options to use PMU extracted from [[1](http://neocontra.blogspot.com.es/2013/05/user-mode-performance-counters-for.html)] and [[2](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0406c/index.html)]:
 
1.	Access from user space. By default PMU coprocessor is only accessible from kernel mode. To be able to use the PMU from user space a bit in PMU should be activated from kernel space using a kernel module. PMU knows from where it is accessed because there is a bit in the status word of the CPU that flags if the instructions being executed are from kernel or user space.
2.	Access from kernel mode; that is, from a kernel module.

Both options require writing a module for the Linux Kernel. We find that working from user space is faster (because there is no overhead added when calling kernel module functions), easier (writing applications is easier than writing kernel modules) and safer (from kernel space you have access to resources and memory positions that can crash the whole OS) than doing it from kernel space.  For this reasons we decided to write this module that activates the correct bit and permits PMU to be accessed from user space.  
 
 PMU is not  mapped in the address space of the processor. It is accessed through  instructions like the Neon coprocessors. Before writing this drivers we just tried to access PMU with the following program:
 
 ```c
#include <stdio.h>
int main(){

	//Read a register of the PMU
	asm volatile("mrc    p15, 0, r0, c9, c12, 0");

	return 0;
}	 
```
 
 And the result is:
 
 
<p align="center">
  <img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/Linux-modules/Enable_PMU_user_space/access_PMU_user.png" width="400" align="middle" alt="Cyclone V SoC simplified block diagram" />
</p>
 
This completely proofs the need of this module.

We used this module to perform accurate time measuremets from Linux applications. The baremetal project [Second_counter_PMU](https://github.com/robertofem/CycloneVSoC-examples/tree/master/Baremetal-applications/Second_counter_PMU) is an example on how to use the PMU to measure time. The same code can be compiled without changes into a Linux Application. 

Description of the code
---------------------------
From section B6.1.81 of the [[2](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0406c/index.html)]: To access from user space to PMU registers it is needed to set to 1 the bit PMUSERENR.EN.

This LKM only contains the two most basic functions: _init_ and _exit_ functions. They perform the followng tasks:

* mod_init: executed when the module is inserted using _insmod_. It:
 
    * Reads the content of PMUSERENR.EN and prints it.
    * Sets to 1 PMUSERENR.EN to enable access from user space.
    * Reads the content again of PMUSERENR.EN and prints it.
    
* mod_exit: executed when using _rmmod_. Does nothing. 


Contents in the folder
----------------------
* PMU_User_Space_EN.c: main file containing the code just explained before.
* Makefile: describes compilation process.

Compilation
-------------
To compile the driver you need to first compile the Operating System (OS) you will use to run the driver, otherwise the console will complain that it cannot insert the driver cause the tag of your module is different to the tag of the OS you are running. It does that to ensure that the driver will work. Therefore:

  * Compile the OS you will use. In [tutorials to build a SD card with Operating System](https://github.com/robertofem/CycloneVSoC-examples/tree/master/SD-operating-system) there are examples on how to compile OS and how to prepare the environment to compile drivers. 
  * Prepare the make file you will use to compile the module. The makefile provided in this example is prepared to compile using the output of the [Angstrom-v2013.12](https://github.com/robertofem/CycloneVSoC-examples/tree/master/SD-operating-system/Angstrom-v2013.12) compilation process. CROSS_COMPILE contains the path of the compilers used to compile this driver. ROOTDIR is the path to the kernel compiled source. It is used by the driver to get access to the header files used in the compilation (linux/module.h or linux/kernel.h in example).
  * Open a regular terminal (I used Debian 8 to compile Angstrom-v2013.12 and its drivers), navigate until the driver folder and type _make_.
 
The output of the compilation is the file _PMU_User_Space_EN.ko_.

How to test
------------
* Switch on the board.
* Connect the serial console port (the mini-USB port in DE1-SoC) to the computer and open a session with a Serial Terminal (like Putty) at 115200 bauds. Now you have access to the board OS console.
* Copy the _PMU_User_Space_EN.ko_ file in the SD card (using SSH or connecting it to a regular computer running Linux system) and insert it into the kernel using _insmod_ command: 
```bash
  $ insmod PMU_User_Space_EN.ko
```
 * The result is printed in the kernel log that should show say that the value of PMUSERENR.EN is 1. Depending on the configuration of your operating system the messages from the LKM will be directly printed in screen or you will need to use _dmesg_ to see them:
 ```bash
  $ dmesg
```

Inserting the module will activate the PMU of the processor where the code is running. If the application that later uses PMU accesses it runs in a different core the _illegal instruction_ message will appear again. In that case you can remove and insert the module several times until the CPU where the application is running gets enable. Other option is to use [taskset](http://xmodulo.com/run-program-process-specific-cpu-cores-linux.html) utility (not installed in [Angstrom-v2013.12](https://github.com/robertofem/CycloneVSoC-examples/tree/master/SD-operating-system/Angstrom-v2013.12)) to choose the CPU where you want to run the application. You can run the application in one core and if it does not run just try the other.