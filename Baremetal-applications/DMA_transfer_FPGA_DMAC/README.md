DMA_transfer_FPGA_DMAC
======================

Description
-----------
This is a simple example showing how to use a DMA in the FPGA. It is intended to be used with the hardware project [DMA_FPGA](https://github.com/robertofem/CycloneVSoC-examples/tree/master/FPGA-hardware/DE1-SoC/FPGA_DMA). The component for doing DMA is the basic (non scatter-gather) DMA Controller available in Qsys.

In this particular example the DMA controller reads a buffer in processor memory and copies to a FPGA On-Chip RAM (FPGA-OCR) using the FPGA-to-HPS port. Since the FPGA-to-HPS port access L3, FPGA has access to ACP and L3-SDRAMC port, so coherent access to cache and SDRAM access can be performed. In this example
this can be controlled with a macro.

At hardware level the DMAC controller should be connected to HPS using the read port and to FPGA-OCR using the write port, so data can flow in the intended direction. This DMA component is not bidirectional and there are write and read ports with fixed functionality. Using a macro it is possible in this example to change the direction of the transfer and write into HPS memories the data from the FPGA-OCR. In this case the hardware project must be modified to connect the read port of the DMA controller to the FPGA-OCR and the write port to the HPS.

The macros to control the behaviour of the example:

* SWITCH_ON_CACHE: if it is defined the cache is enabled and the DMAC accesses the buffer in processor memory through ACP port. Access is coherent to cache. If it is not defined  the cache is disabled and access is through L3-SDRAMC port. Notice that the combination of cache enablement and port selected ensures coherency of data without cache flushing. In the case of having the cache switched on and FPGA writing to L3-SDRAM port the processor may read data in cache instead the last version living in SDRAM only. In this case processor should flush the cache before reading this data. If the operation is reading the processor should flush cache after writing data to memory so the FPGA reads the latest version of it.

* DMA_TRANSFER_SIZE: size of the transfer in Bytes.

* WRITE_OPERATION: if defined it means that a write operation takes place
  (read FPGA-OCR and write the HPS memories). If not defined (default),
  the read operation is done.

Contents in the folder
----------------------
This example was programmed modifying the "HPS DMA Example" from Altera (File name: Altera-SoCFPGA-HardwareLib-DMA-CV-GNU.tar).
* dma_demo.c: The main() function is in dma_demo.c file.
* fpga_dma_api.c and fpga_dma_api.h: contain macros and functions to ease programming of the DMA Controller in the FPGA.
* arm_cache_modified.c and arm_cache_modified.h: cache control functions. Original Legup functions were defined in arm_cache.c and arm_cache.h. Slight changes were done in arm_cache.c. Thats why the files arm_cache_modified.h and arm_cache_modified.c were created. The modifications are only in arm_cache.c:
    * L1_NORMAL_111_11 constant was changed from its original value  0x00007c0e to 0x00017c0e. This change sets S bit in table  descriptors definyng normal memory region attributes, making  normal memory shareable (coherent) for ACP accesses.

* The io.c file gives support to the printf function to print messages in console.

* The rest of the .c files were directly copied from hwlib without modifications.

* cycloneV-dk-ram-modified.ld: describes the memory regions (stack, heap, etc.).

* Makefile: describes compilation process.

Compilation
-----------
Open SoC EDS Command Shell, navigate to the folder of the example and type make.
This programs was tested with Altera SoC EDS v16.1
The compilation process generates two files:
* baremetalapp.bin: to load the baremetal program from u-boot
* baremetalapp.bin.img: to load the baremetal program from preloader

How to test
-----------
In the following folder there is an example on how to run baremetal examples available in this repository:
[https://github.com/robertofem/CycloneVSoC-examples/tree/master/SD-baremetal](https://github.com/robertofem/CycloneVSoC-examples/tree/master/SD-baremetal).
