Cyclone V SoC examples
====================
Examples using the FPSoC chip Cyclone V SoC. All these examples were tested on DE1-SoC board. However most of them are easily ported to other boards using Cyclone V SoC chips because they do not interact with the hardware in the board. This repository contains:

* Baremetal-applications: Stand-alone applications without Operating System.

	* Basic\_transfer\_DMA\_PL330: This is a complete example on moving data using the HPS Direct Memory Access Controller (DMAC) PL330. Data can be moved from a buffer in processor to another buffer in processor or to the FPGA. This example also shows how to switch on the cache memories L1 and L2 and how to configure the ACP port to access cache memories from L3 when caches are on.
	
	*  Second\_counter\_PMU: This example uses a counter in the Performance Monitoring Unit (PMU) timer to measure seconds and build a second counter. It stands as an example on how to use PMU to measure time in baremetal.

* FPGA-hardware: Quartus projects describing the FPGA hardware needed in some of the examples.

	* DE1-SoC:  Hardware for Terasic´s DE1-SoC board.

		* FPGA\_OCR\_256K: this hardware project includes a 256kB On-Chip memory in the FPGA, implemented using 10Mb memory blocks. This memory is hanging at the beginning of the address space of the HPS-to-FPGA bridge.
    
* Linux-applications:
    * Test\_DMA\_PL330\_LKM: it shows how to use the DMA\_PL330\_LKM module.

* Linux-modules: Linux Loadable Kernel Modules (LKM). All come with test apps in Linux_Applicatios folder. 
	* DMA_PL330_LKM_Basic: module to make data transfers using the PL330 DMAC available in HPS. It can move data between: FPGA memory, HPS On-chip RAM, uncached buffer in processor´s RAM and cached buffer in processor´s RAM (through APC).

	* DMA_PL330_LKM: module to make transfers between application and FPGA using PL330 DMAC. It uses char device driver interface to copy the data from application to a uncached or cached (through ACP) buffer in driver´s memory space. Later it uses PL330 DMAC to copy that buffer to FPGA.

* Useful-scripts: Linux shell scripts to ease configuration of the board.
	* fixed_mac_dhcp.sh: fixes MAC and asks IP using DHCP protocol.

Build_Baremetal_SD: This brief tutorial explains how to build a SD card to run the baremetal examples provided in this repository.
    
