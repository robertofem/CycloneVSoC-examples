
CycloneVSoC_Examples
====================
Examples using the FPSoC chip Cyclone V SoC

DE1-SoC
-------
Contains examples for the Terasic´s DE1-SoC board. Most of them are easily ported to other boards using CycloneV-SoC chips because they do not interact with the hardware in the board. The folder contains:

* Baremetal:
	* Basic_Transfer_DMA_PL330: It transfers a buffer from processors RAM to other buffer in RAM or to a memory in FPGA. It uses the HPS PL330 DMAC and the ALtera´s hwlib library to control it.

* Linux_Applications:

* Linux_Modules: Linux Loadable Kernel Modules (LKM). Most of them come with test apps.
	* DMA_PL330_LKM: module to make transfers between application and FPGA using PL330 DMAC.

* Useful_Scripts: Linux shell scripts to ease configuration (fixed IP, DHCP, etc.).
	* fixed_mac_dhcp.sh: fixes MAC and asks IP using DHCP protocol.
