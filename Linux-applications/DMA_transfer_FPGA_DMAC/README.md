DMA_transfer_FPGA_DMAC
=====================

Introduction
-------------
This application does a simple transfer using a DMA Controller (DMAC) in the FPGA. This example was done to develop an API to control the DMAC. The DMAC used is the simple (non scatter-gather) DMA Controller Core available in Qsys. Its documentation is available at [DMA Controller Core doc](https://www.altera.com/documentation/sfo1400787952932.html#iga1401397703359).

In this particular example a transfer is performed from an On-Chip RAM in the FPGA (FPGA-OCR) and the On-Chip RAM available in the HPS (HPS-OCR) and viceversa.  Since the example does not use SDRAM memory it is not needed to develop a driver for its allocation, easing the debugging of the API.

The program is intended to be used with the hardware project [FPGA_DMA](https://github.com/robertofem/CycloneVSoC-examples/tree/master/FPGA-hardware/DE1-SoC/FPGA_DMA) that contains:

  * The DMA Controller Core. The control port is connected to HPS through HPS-to-FPGA bridge at address  0x10000. The read port, intended to read the source buffer during the DMA transfers, is connected to the FPGA-HPS bridge so it can access HPS-OCR. The write port, intended to write data during DMA transfers, is connected to
  the FPGA-OCR at address 0.

  * The FPGA-OCR: 1kB of double port memory. One port is connected to the HPS-to-FPGA bridge at address 0x0 so the processor has access to the buffer and initialize the content of the memory. The second port is connected to the DMA read port, as commented above.

By default the applicaton moves data from HPS-OCR to FPGA-OCR. To change
the direction uncomment the line //#define WRITE_OPERATION and switch
the read and write ports of the DMA Controller in the hardware project
using Qsys.

Description of the code
------------------------

DMA_transfer_FPGA_DMAC first generates the virtual addresses to access FPGA and HPS peripherals using mmap. 1GB of address space is mapped to cover from 0xC0000000
(the beginning of HPS-to-FPGA bridge where FPGA-OCR and control port of DMAC are
connected) until 0xFFFFFFFF (the end of the HPS-OCR). Using this mapping the individual virtual address of each component are calculated.

Later both the FPGA-OCR and HPS-OCR are checked to ensure there is not an access problem like FPGA design not loaded or wrong addresses. Later they are reset to start the experiment with both memories at 0.

Then FPGA-OCR and HPS-OCR are initialized with random values before the transfer
starts. To program the transfer the control register is first loaded indicating
in this case Word (32-bit) Transfers (FPGA_DMA_WORD_TRANSFERS) that end when the lenght of the remaining transfer is 0 (FPGA_DMA_END_WHEN_LENGHT_ZERO). There are other methods to end the transfer like hardware signaling but this is the most common. Using the macros in dpga_dmac_api.h the user can test all the available options. For example if FPGA_DMA_WORD_TRANSFERS is changed by  FPGA_DMA_BYTE_TRANSFERS and FPGA_DMA_READ_CONSTANT_ADDR is added the DMAC will do byte transfers always reading the same first byte of the FPGA-memory and therefore the HPS-OCR will be filled with the same value.

Contents in the folder
----------------------
* DMA_transfer_FPGA_DMAC.c: the previously commented code is here.
* fpga_dmac_api.h and fpga_dmac_api.c: macros and functions to control the DMA Controller Core in the FPGA.
* Makefile: describes compilation process.

Compilation
-----------
Open *SoC EDS Command Shell* (*Intel FPGA SoC EDS* needs to be installed in your system), navigate to the folder of the example and type **_make_**.
This program was tested with Intel *FPGA SoC EDS v16.1*.

The compilation process generates the executable file *DMA_transfer_FPGA_DMAC*.

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

* Copy the executable into the SD card and run the application:
 ```bash
  $ chmod 777 DMA_transfer_FPGA_DMAC
  $ ./DMA_transfer_FPGA_DMAC
```
