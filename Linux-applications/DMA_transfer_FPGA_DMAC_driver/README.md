DMA_transfer_FPGA_DMAC_driver
=====================

Introduction
-------------
This application does a simple transfer from FPGA to a Buffer in application space using a DMA Controller (DMAC) in the FPGA. The DMAC used is the simple (non scatter-gather) DMA Controller Core available in Qsys. Its documentation is available at [DMA Controller Core doc](https://www.altera.com/documentation/sfo1400787952932.html#iga1401397703359).
In this example the [Alloc_DMAble_buffer doc](https://github.com/robertofem/CycloneVSoC-examples/tree/master/Linux-modules/Alloc_DMAble_buff_LKM) module is used to allocate the physically contiguous buffers needed
to perform the DMA transfers.

The program is intended to be used with the hardware project [FPGA_DMA](https://github.com/robertofem/CycloneVSoC-examples/tree/master/FPGA-hardware/DE1-SoC/FPGA_DMA) that contains:

  * The DMA Controller Core. The control port is connected to HPS through HPS-to-FPGA bridge at address  0x10000. The read port, intended to read the source buffer during the DMA transfers, is connected to the FPGA-HPS bridge so it can access HPS-OCR. The write port, intended to write data during DMA transfers, is connected to
  the FPGA-OCR at address 0.

  * The FPGA-OCR: 1kB of double port memory. One port is connected to the HPS-to-FPGA bridge at address 0x0 so the processor has access to the buffer and initialize the content of the memory. The second port is connected to the DMA read port, as commented above.

  In this particular example the DMA controller copies a buffer in the FPGA
  On-Chip RAM (FPGA-OCR) to a buffer in the program. Therefore the FPGA-OCR
  must be connected to the read port of the DMAC and the FPGA-to-HPS bridge is
  connected to the write port of the DMAC. This the opossite connection to the
  default connections in the hardware project. Therefore, before testing this
  example switch the connections of the read and write ports of the DMAC
  using Qsys and recompile the FPGA hardware.

Description of the code
------------------------

This program first generates the virtual addresses to access FPGA peripherals using mmap. 1GB of address space is mapped to cover from 0xC0000000
(the beginning of HPS-to-FPGA bridge where FPGA-OCR, the control port of DMAC and the GPIO to modify the AXI signals are connected) until 0xFFFFFFFF. Using this mapping the individual virtual address of each component is calculated.

Later the FPGA-OCR is checked to ensure there is not an access problem like FPGA design not loaded or wrong addresses. Later it is reset to start the experiment with all Bytes of the memory to 0.

After that the AXI signals of the bus connected to the FPGA-to-HPS bridge are
set using a GPIO in the FPGA. Setting this signals to a correct value is mandatory
to perform successgul accesses through ACP.

Then an buffer is allocate with the driver to be used as destiny buffer for the
DMA transfer. The buffer size is defined equal to the DMA transfer size and
cached (so ACP must be used with it).

Lastly the DMA Transfer takes place. Then FPGA-OCR is initialized with random values before the transfer starts. To program the transfer the control register is first loaded indicating in this case Word (32-bit) Transfers (FPGA_DMA_WORD_TRANSFERS) that end when the lenght of the remaining transfer is 0 (FPGA_DMA_END_WHEN_LENGHT_ZERO). There are other methods to end the transfer like hardware signaling but this is the most common. Using the macros in dpga_dmac_api.h the user can test all the available options. For example if FPGA_DMA_WORD_TRANSFERS is changed by  FPGA_DMA_BYTE_TRANSFERS and FPGA_DMA_READ_CONSTANT_ADDR is added the DMAC will do byte transfers always reading the same first byte of the FPGA-memory and therefore the HPS-OCR will be filled with the same value.

In the end of the program the source and destiny buffers are compared to check if the transfer was correct and all buffers and memory mappings are freed.

Contents in the folder
----------------------
* DMA_transfer_FPGA_DMAC.c: the previously commented code is here.
* fpga_dmac_api.h and fpga_dmac_api.c: macros and functions to control the DMA Controller Core in the FPGA.
* Makefile: describes compilation process.

Compilation
-----------
Open *SoC EDS Command Shell* (*Intel FPGA SoC EDS* needs to be installed in your system), navigate to the folder of the example and type **_make_**.
This program was tested with Intel *FPGA SoC EDS v16.1*.

The compilation process generates the executable file *DMA_transfer_FPGA_DMAC_driver*.

How to test
------------
* Configure MSEL pins:
    *  MSEL[5:0]="000000" position when FPGA will be configured from SD card.
    *  MSEL[5:0]="110010" position when FPGA will be configured from EPCQ device or Quartus programmer.
* Switch on the board.
* Compile the FPGA hardware ([FPGA_DMA](https://github.com/robertofem/CycloneVSoC-examples/tree/master/FPGA-hardware/DE1-SoC/FPGA_DMA) in example) and load it in the FPGA):
    *  If MSEL[5:0]="000000" FPGA is loaded by the U-boot during start-up. Check  the [tutorials to build a SD card with Operating System](https://github.com/robertofem/CycloneVSoC-examples/tree/master/SD-operating-system) to learn how to configure the SD card so the FPGA is loaded from it.
    *  If MSEL[5:0]="110010" use Quartus to load the FPGA:
        *  Connect the USB cable (just next to the power connector).
        *  Open Quartus programmer.
        *  Click Autodetect -> Mode JTAG -> Select 5CSEMA5 (for DE1-SoC and DE0-nano-SoC) if asked -> Right click in the line representing the FPGA -> Change FIle -> Select the .sof file for the project you want to load -> tick Program/Configure -> Click Start.

* Connect the serial console port (the mini-USB port in DE1-SoC) to the computer and open a session with a Seral Terminal (like Putty) at 115200 bauds. Now you have access to the board OS console.

* Copy the executable and the driver(alloc_dmable_buff.ko) into the SD card and:
 ```bash
  $ insmod alloc_dmable_buff.ko
  $ chmod 777 DMA_transfer_FPGA_DMAC_driver
  $ ./DMA_transfer_FPGA_DMAC_driver
```
