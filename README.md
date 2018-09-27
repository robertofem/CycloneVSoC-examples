Cyclone V SoC examples
====================
Examples using the FPSoC chip [Cyclone V SoC](https://www.altera.com/products/soc/portfolio/cyclone-v-soc/overview.html). All these examples were tested on DE1-SoC board. However most of them are easily ported to other boards including Cyclone V SoC chips because they do not interact with the hardware in the board.

<p align="center">
  <img src="https://github.com/robertofem/CycloneVSoC-examples/raw/master/CycloneVSoC.png" width="450" align="middle" alt="Cyclone V SoC simplified block diagram" />
</p>

This repository contains:

* **Starting-guides**: guides on how to start with Cyclone V SoC boards. Currently
  only a guide for DE1-SoC board is available. However the process to
  start with any Cyclone V SoC board is similar and this guide can be used
  regardless the Cyclone V SoC board used.

* **Baremetal-applications**: Stand-alone applications without Operating System.
	* DMA_transfer_FPGA_DMAC: This example shows how to use a DMA controller in the FPGA to read and write the HPS memories. Transfers can be done with cache switched ON through ACP and with cache switched OFF through the L3-to-SDRAMC port.
  * DMA_transfer_PL330_ACP: This is a complete example on moving data using the HPS Direct Memory Access Controller (DMAC) PL330. Data can be moved from a buffer in processor to another buffer in processor or to the FPGA. This example also shows how to switch on the cache memories L1 and L2 and how to configure the ACP port to access cache memories from L3 when caches are on.
  * Second_counter_PMU: This example uses a counter in the Performance Monitoring Unit (PMU) timer to measure seconds and build a second counter. It stands as an example on how to use PMU to measure time in baremetal.

* **FPGA-hardware**: Quartus projects describing the FPGA hardware needed in some of the examples.
  * DE1-SoC:  Hardware for Terasic´s DE1-SoC board.
    * FPGA_DMA: it implements a DMA controller in the FPGA and a 1kB on-chip memory. Using this DMA it is possible to move data between HPS and FPGA using the FPGA as master.  
    * FPGA_OCR_256K: this hardware project includes a 256kB On-Chip memory in the FPGA, implemented using 10Mb memory blocks. This memory is hanging at the beginning of the address space of the HPS-to-FPGA bridge.

* **Linux-applications**:
    * Test_DMA_PL330_LKM: it shows how to use the DMA\_PL330\_LKM module.
    * DMA_transfer_FPGA_DMAC: It transfers data from an On-Chip RAM in FPGA
    to On-Chip RAM in HPS and viceversa using a DMA Controller in FPGA.
    * DMA_transfer_FPGA_DMAC_driver: It transfers data from an On-Chip RAM in FPGA
    to a Buffer in the application using a DMA Controller in the FPGA and the
    Alloc_DMAble_buff_LKM module.

* **Linux-modules**: Linux Loadable Kernel Modules (LKM).
  * Alloc_DMAble_buff_LKM: This driver allocates up to 5 physically contiguous
  buffers in kernel space and provides its physical addresses through sysfs and
  access to the buffer through character device interface. These
  buffers are intended to work as intermediate buffers in DMA transfers.
  Linux_applications/DMA_transfer_FPGA_DMAC_driver shows how to use it.
  * DMA_PL330_LKM_Basic: stand-alone module that makes a data transfer using the PL330 DMAC (available in HPS) when inserted into the operating system. It can be configured to move data between: FPGA memory, HPS On-chip RAM, uncached buffer in processor´s RAM and cached buffer in processor´s RAM (through APC). It is a complete example that can be used as starting point for developing a DMA module for a specific application.
  * DMA_PL330_LKM: module to make transfers between an application and the FPGA using PL330 DMAC. It uses char device driver interface to copy the data from application to a uncached or cached (through ACP) buffer in driver´s memory space. Later it uses PL330 DMAC to copy that buffer to FPGA. A /dev/dma_pl330 entry is created so writing in the FPGA is so easy as writing to a file. Linux_applications/Test_DMA_PL330_LKM shows how to use it.
  * Enable_PMU_user_space: this module permits access to the Performance Monitoring Unit (PMU) from user space. By default the access from user space is forbidden and a bit must be setting from kernel space to later have access from user space. This module accomplishes that task.

* **Useful-scripts**: Linux shell scripts to ease configuration of the board.
	* fixed_mac_dhcp.sh: fixes MAC and asks IP using DHCP protocol.

* **SD-baremetal**: This brief tutorial explains how to build a SD card to run the baremetal examples provided in this repository.

* **SD-operating-system**: It explains how to build an SD card with Operating System from scratch. All the files needed are also provided to save time. Currently the OS that have been tested are:
    * Angstrom-v2013.12.
    * Angstrom-v2016.12. This tutorial also explains MAC spoofing (to set-up MAC on start-up), custom driver installation and running applications on start-up.
