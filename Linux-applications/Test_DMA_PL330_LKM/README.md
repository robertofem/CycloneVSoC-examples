Test_DMA_PL330_LKM
===============

Description
-----------
This application tests the [DMA_PL330_LKM](https://github.com/robertofem/CycloneVSoC-examples/tree/master/Linux-modules/DMA_PL330_LKM) kernel module. DMA_PL330_LKM is a module that permits application-FPGA transfers using the DMA controller PL330 available in HPS.  It uses character driver interface to communicate to application space. The entry /dev/dma_pl330 is created when the driver is inserted and afterwards the FPGA can be accessed as a file with the regular functions open(), read(), write() and close(). 

In the FPGA should be a memory in the address 0xC0000000 from processor address space (0x00000000 from HPS-FPGA bridge address space) with space enough for the transfers. For this purpose the [FPGA_OCR_256K](https://github.com/robertofem/CycloneVSoC-examples/tree/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K) project in this repository can be used. 

Test_DMA_PL330_LKM first generates a virtual address to access FPGA from application space, using mmap(). This is needed to check if the transfers using the driver are being done in proper way. After that the driver is configured using a sysfs entry in /sys/dma_pl330/. Lastly the program copies a buffer from application to the FPGA using write() and copies back the content in  the FPGA to the application using the read() function. Both operations are checked and a ERROR message is shown if the transfer went wrong.

The configuration of the module can be controlled with 4 macros on the top of the program:

* DMA_TRANSFER_SIZE: Size of the DMA transfer to test the DMA_PL330_LKM in Bytes. Only used when PREPARE_MICROCODE_WHEN_OPEN = 1. Otherwise the size of the DMA transfer is the size passed as argument in read or write functions.

* USE_ACP: When 0 the  PL330 DMAC will copy data from/to FPGA to/from an un-cached buffer in processor memory using the port connecting L3 and SDRAMC. When 1 the access is through ACP port using a cached buffer. 

* DMA_BUFF_PADD: This is the physical address in the FPGA were the module is going to copy data from the application space. When using the  [FPGA_OCR_256K](https://github.com/robertofem/CycloneVSoC-examples/tree/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K) project this address is 0xC0000000.

* PREPARE_MICROCODE_WHEN_OPEN: When 0 the microcode that PL330 DMA Controller executes is prepared before every transfer when entering the open() or read() function. When 1 the microcode is prepared when calling the open() function. Two microcodes are generated: one for read FPGA and another for write to FPGA. Later when using read() or write() the prepared microcodes are used. This saves the microcode preparation time  when doing the transfer. 

Contents in the folder
----------------------
* test_DMA_PL330_LKM.c: all code of the program is here.
* Makefile: describes compilation process.

Compilation
-----------
Open *SoC EDS Command Shell* (*Intel FPGA SoC EDS* needs to be installed in your system), navigate to the folder of the example and type **_make_**.
This program was tested with Intel *FPGA SoC EDS v16.1*.
The compilation process generates the executable file *test_DMA_PL330_LKM*.
    
How to test
------------
* Configure MSEL pins:
    *  MSEL[5:0]="000000" position when FPGA will be configured from SD card.
    *  MSEL[5:0]="110010" position when FPGA will be configured from EPCQ device or Quartus programmer.
* Switch on the board.
* Compile and the FPGA hardware ([FPGA_OCR_256K](https://github.com/robertofem/CycloneVSoC-examples/tree/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K)  in example) and load it in the FPGA:
    *  If MSEL[5:0]="000000" FPGA is loaded by the U-boot during start-up. Check  the [tutorials to build a SD card with Operating System](https://github.com/robertofem/CycloneVSoC-examples/tree/master/SD-operating-system) to learn how to configure the SD card so the FPGA is loaded from it. 
    *  If MSEL[5:0]="110010" use Quartus to load the FPGA:
        *  Connect the USB cable (just in one side of power connector).
        *  Open Quartus programmer.
        *  Click Autodetect -> Mode JTAG -> Select 5CSEMA5 (for DE1-SoC and DE0-nano-SoC) if asked -> Right click in 5CSEMA5 -> Change FIle -> Select the .sof file for the project you want to load -> tick Program/Configure -> Click Start.

* Connect the serial console port (the mini-USB port in DE1-SoC) to the computer and open a session with a Seral Terminal (like Putty) at 115200 bauds.
* Copy the [DMA_PL330_LKM](https://github.com/robertofem/CycloneVSoC-examples/tree/master/Linux-modules/DMA_PL330_LKM) module in the SD card and insert it into the kernel using _insmod_ command: 
```bash
  $ insmod DMA_PL330_LKM
```
> Remember. The version of the kernel for which the driver is compiled for should be the same running in the board. This implies that you have to compile the OS you are running in the board and compile the driver with the output files from that compilation. In the [tutorials to build a SD card with Operating System](https://github.com/robertofem/CycloneVSoC-examples/tree/master/SD-operating-system) and in the [DMA_PL330_LKM folder](https://github.com/robertofem/CycloneVSoC-examples/tree/master/Linux-modules/DMA_PL330_LKM) you can find more information about compiling an Operating System and a Loadable Kernel Moduler (LKM) respectively.

* Run the application:
* ```bash
  $ chmod 777 Test_DMA_PL330_LKM
  $ ./Test_DMA_PL330_LKM
```
