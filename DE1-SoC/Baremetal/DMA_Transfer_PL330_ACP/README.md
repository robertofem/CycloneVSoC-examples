DMA_Transfer_PL330_ACP
======================

Description
-----------
This is a complete example on moving data using the HPS Direct Memory Access Controller (DMAC) PL330. This example also shows how to switch on the cache memories L1 and L2 and how to configure the ACP port to access cache memories from L3.
 
This example moves data between a source buffer in processor´s memory to a destiny buffer in processor´s memory or in the FPGA:
* When the macro USE_FPGA is not defined the destiny is in processors memory.
* When the macro USE_FPGA is defined the destiny is a buffer in the FPGA in the address 0xC0000000, the beginning of the HPS-FPGA bridge. Therefore when selecting this option there should be a memory in the FPGA with enough space to do the transfer. In the folder https://github.com/robertofem/CycloneVSoC_Examples/tree/master/DE1-SoC/FPGA_Hardware/FPGA_OCR_256K you can find a hardware project implementing a 256KB On-Chip RAM memory in the FPGA for this purpose.
    
To control the cache behaviour there is the macro SWITCH_ON_CACHE.
* When SWITCH_ON_CACHE is not defined the cache system L1 and L2 are switched off. In this case the DMAC accesses the processor RAM using the direct conection between L3 interconnect and SDRAM controller. The reader can modify transfer pointers to access processor memories  through ACP but this is slower because ACP will access L2 cache controller and L2 cache controller will finally access RAM to get the data because cache is off.
* When SWITCH_ON_CACHE is defined the cache L1 and L2 are switched on with optimizations using Legup functions. In this case the DMAC accesses the processor cached memory in a coherent way using the ACP. This is very much faster than when the cache is off because: a) the  processor instructions are cached and therefore executed faster, b) the data is in cache that is faster than RAM. 
      However for big buffers (bigger than cache size) this strategy could be counterproductive because the DMA will be writing /reading cache through APC but data is not there so L2 controller will need to access external RAM anyway. In this case (not treated in this example) it is better to directly access to external RAM through the RAM Controller (like when cache is OFF). In this case, to maintain the coherency of the data the user should flush the cache after the transfer (when writing to the processor RAM) or before the transfer (when readingfrom processor RAM).

This example was programmed modifying the "HPS DMA Example" from Altera (File name: Altera-SoCFPGA-HardwareLib-DMA-CV-GNU.tar).
Legup functions to configure cache in Cyclone V SoC defined in arm_cache.c were slightly modified. Thats why we created the files arm_cache_modified.h and arm_cache_modified.c. The modification is only in arm_cache.c: 
* L1_NORMAL_111_11 constant was changed from its original value  0x00007c0e to 0x00017c0e. This change sets S bit in table  descriptors definyng normal memory region attributes, making  normal memory shareable (coherent) for ACP accesses.

alt_dma_modified.c also contains modifications to the original alt_dma.c from hwlib. The changes are:
* ALT_DMA_CCR_OPT_SC_DEFAULT and ALT_DMA_CCR_OPT_DC_DEFAULT so the DMA does cacheable accesses. 
* Other change is the split of alt_dma_memory_to_memory() into 2 functions alt_dma_memory_to_memory_only_prepare_program() and alt_dma_channel_exec(). This way the program and its execution can run separately. The program can be prepared during initializations only once calling alt_dma_memory_to_memory_only_prepare_program() and transefer time is later reduced just calling alt_dma_channel_exec().

The io.c file gives support to the printf function to print messages in console. 

The main() function is in dma_demo.c file 

The rest of the .c files were directly copied from hwlib without modifications.

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
https://github.com/robertofem/CycloneVSoC_Examples/tree/master/DE1-SoC/Baremetal/Build_baremetal_SD.
