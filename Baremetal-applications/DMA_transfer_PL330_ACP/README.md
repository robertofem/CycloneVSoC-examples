DMA_transfer_PL330_ACP
======================

Description
-----------
This is a complete example on moving data using the HPS Direct Memory Access Controller (DMAC) PL330. This example also shows how to switch on the cache memories L1 and L2 and how to configure the ACP port to access cache memories from L3.
 
This example moves data between a source buffer in processor´s memory to a destiny buffer in processor´s memory or in the FPGA:
* When the macro USE_FPGA is not defined the destiny is in processors memory.
* When the macro USE_FPGA is defined the destiny is a buffer in the FPGA in the address 0xC0000000, the beginning of the HPS-FPGA bridge. Therefore when selecting this option there should be a memory in the FPGA with enough space to do the transfer in address 0 of that bridge. In the folder [https://github.com/robertofem/CycloneVSoC-examples/tree/master/FPGA-hardware/FPGA_OCR_256K](https://github.com/robertofem/CycloneVSoC-examples/tree/master/FPGA-hardware/FPGA_OCR_256K) you can find a hardware project implementing a 256KB On-Chip RAM memory in the FPGA for this purpose.
    
To control the cache behaviour there is the macro SWITCH_ON_CACHE.
* When SWITCH_ON_CACHE is not defined the cache system L1 and L2 are switched off. In this case the DMAC accesses the processor RAM using the direct conection between L3 interconnect and SDRAM controller. You can modify transfer pointers to access processor memories  through ACP but this is slower because ACP will access L2 cache controller and L2 cache controller will finally access RAM to get the data because cache is off. So direct access to RAM is faster in this case.
* When SWITCH_ON_CACHE is defined the cache L1 and L2 are switched on with optimizations using functions devekloped by Legup([http://legup.eecg.utoronto.ca/wiki/doku.php?id=using_arm_caches](http://legup.eecg.utoronto.ca/wiki/doku.php?id=using_arm_caches)). In this case the DMAC accesses the processor cached memory in a coherent way using the ACP. This is very much faster than when the cache is off because: a) the  processor instructions are cached and the preparation of DMAC program is executed faster, b) the data is in cache that is faster than RAM. 
      However for big buffers (more or less for sizes bigger than cache size) this strategy could be counterproductive because the DMA will be writing/reading cache through APC but data is not there so L2 controller will need to access external RAM anyway. In this case (not treated in this example) it is better to directly access to external RAM through the RAM Controller (like when cache is OFF). In this case, to maintain the coherency of the data the programmer should flush the cache after the transfer (when writing to the processor´s RAM) or before the transfer (when readingfrom processor´s RAM).

      
Contents in the folder
----------------------
This example was programmed modifying the "HPS DMA Example" from Altera (File name: Altera-SoCFPGA-HardwareLib-DMA-CV-GNU.tar).
* The main() function is in dma_demo.c file. 
* arm_cache_modified.c and arm_cache_modified.h: cache control functions. Original Legup functions were defined in arm_cache.c and arm_cache.h. Slight changes were done in arm_cache.c. Thats why the files arm_cache_modified.h and arm_cache_modified.c were created. The modifications are only in arm_cache.c: 
    * L1_NORMAL_111_11 constant was changed from its original value  0x00007c0e to 0x00017c0e. This change sets S bit in table  descriptors definyng normal memory region attributes, making  normal memory shareable (coherent) for ACP accesses.

* alt_dma_modified.c  and alt_dma_modified.h describe the DMAC control functions. The original alt_dma.c from hwlib was modified. The changes are:
    * ALT_DMA_CCR_OPT_SC_DEFAULT was changed by ALT_DMA_RC_ON = 0x00003800 in alt_dma.c. ALT_DMA_CCR_OPT_DC_DEFAULT was changed by ALT_DMA_WC_ON =  0x0E000000. These changes make the channel 0 of the DMAC to do cacheable access with its AXI master port.
    * Other change is the split of alt_dma_memory_to_memory() into 2 functions: alt_dma_memory_to_memory_only_prepare_program() and alt_dma_channel_exec(). This way the program preparation and its execution can be run separately. The program can be prepared during initializations only once calling alt_dma_memory_to_memory_only_prepare_program() and transfers performed with alt_dma_channel_exec() passing the prepared program as argument. This transfer will be faster cause the instructions the processor executes to prepare the DMAC program are not executed.

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
