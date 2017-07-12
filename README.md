Cyclone V SoC examples
====================
Examples using the FPSoC chip Cyclone V SoC. All these examples were tested on DE1-SoC board. However most of them are easily ported to other boards including Cyclone V SoC chips because they do not interact with the hardware in the board. 

![alt text](https://github.com/robertofem/CycloneVSoC-examples/raw/master/CycloneVSoC.png =100x20)

<center><img src="https://github.com/robertofem/CycloneVSoC-examples/raw/master/CycloneVSoC.png" width="450" align="middle" alt="Cyclone V SoC simplified block diagram" /></center>

This repository contains:

* **Baremetal-applications**: Stand-alone applications without Operating System.

	* Basic\_transfer\_DMA\_PL330: This is a complete example on moving data using the HPS Direct Memory Access Controller (DMAC) PL330. Data can be moved from a buffer in processor to another buffer in processor or to the FPGA. This example also shows how to switch on the cache memories L1 and L2 and how to configure the ACP port to access cache memories from L3 when caches are on.
	
	*  Second\_counter\_PMU: This example uses a counter in the Performance Monitoring Unit (PMU) timer to measure seconds and build a second counter. It stands as an example on how to use PMU to measure time in baremetal.

* **FPGA-hardware**: Quartus projects describing the FPGA hardware needed in some of the examples.

	* DE1-SoC:  Hardware for Terasic´s DE1-SoC board.

		* FPGA\_OCR\_256K: this hardware project includes a 256kB On-Chip memory in the FPGA, implemented using 10Mb memory blocks. This memory is hanging at the beginning of the address space of the HPS-to-FPGA bridge.
    
* **Linux-applications**:
    * Test_DMA_PL330_LKM: it shows how to use the DMA\_PL330\_LKM module.

* **Linux-modules**: Linux Loadable Kernel Modules (LKM). 
	* DMA_PL330_LKM_Basic: stand-alone module that makes a data transfer using the PL330 DMAC (available in HPS) when inserted into the operating system. It can be configured to move data between: FPGA memory, HPS On-chip RAM, uncached buffer in processor´s RAM and cached buffer in processor´s RAM (through APC). It is a complete example that can be used as starting point for developing an specific application.

	* DMA_PL330_LKM: module to make transfers between an application and the FPGA using PL330 DMAC. It uses char device driver interface to copy the data from application to a uncached or cached (through ACP) buffer in driver´s memory space. Later it uses PL330 DMAC to copy that buffer to FPGA. A /dev/dma_pl330 entry is created so writing in the FPGA is so easy as writing to a file. Linux_applications/Test_DMA_PL330_LKM shows how to use it.

* **Useful-scripts**: Linux shell scripts to ease configuration of the board.
	* fixed_mac_dhcp.sh: fixes MAC and asks IP using DHCP protocol.

* **SD-baremetal**: This brief tutorial explains how to build a SD card to run the baremetal examples provided in this repository.

* **SD-operating-system**: It explains how to build an SD card with Operating System from scratch. All the files needed are also provided to save time. Currently the OS that have been tested are:

    * Angstrom-v2012.12
    
