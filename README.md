CycloneVSoC_Examples
====================
Examples using the FPSoC chip Cyclone V SoC

DE1-SoC
-------
Contains examples for the Terasic´s DE1-SoC board. Most of them are easily ported to other boards using CycloneV-SoC chips because they do not interact with the hardware in the board. The folder contains:

* Baremetal: Stand-alone projects without Operating System.
	* Basic_Transfer_DMA_PL330: It transfers a buffer from processors RAM to other buffer in RAM or to a memory in FPGA. It uses the HPS PL330 DMAC and the ALtera´s hwlib library to control it.

* FPGA_Hardware: Quartus projects describing the FPGA hardware needed in some of the examples.
	* FPGA_OCR_256K: this hardware project includes a 256kB On-Chip memory in the FPGA, implemented using 10Mb memory blocks. This memory is hanging at the beginning of the address space of the HPS-to-FPGA bridge.
    
* Linux_Applications:
    * Test_DMA_PL330_LKM: it shows how to use the DMA_PL330_LKM module.

* Linux_Modules: Linux Loadable Kernel Modules (LKM). All come with test apps un Linux_Applicatios folder. 
	* DMA_PL330_LKM_Basic: module to make data transfers using the PL330 DMAC available in HPS. It can move data between: FPGA memory, HPS On-chip RAM, uncached buffer in processor´s RAM and cached buffer in processor´s RAM (through APC).
	* DMA_PL330_LKM: module to make transfers between application and FPGA using PL330 DMAC. It uses char device driver interface to copy the data from application to a uncached or cached (through ACP) buffer in driver´s memory space. Later it uses PL330 DMAC to copy that buffer to FPGA.

* Useful_Scripts: Linux shell scripts to ease configuration (fixed IP, DHCP, etc.).
	* fixed_mac_dhcp.sh: fixes MAC and asks IP using DHCP protocol.
    
