====================
CycloneVSoC_Examples
====================
Examples using the FPSoC chip Cyclone V SoC

DE1-SoC
-------
Contains examples for the Terasic´s DE1-SoC board. Most of them are easily ported to other boards using CycloneVSoC chips because they do not interact with the hardware in the board. The folder contains:

* Baremetal:

* Linux_Applications:

* Linux_Modules: Linux Loadable Kernel Modules (LKM). Most of them come with test apps.
** DMA_PL330_LKM: module to make transfers between application and FPGA using PLL330 DMAC.

* Useful_Scripts: Linux shell scripts to ease configuration (fixed IP, DHCP, etc.).
	* fixed_mac_dhcp.sh: fixes MAC and asks IP using DHCP protocol.
