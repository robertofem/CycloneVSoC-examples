#DMA_PLL330_LKM

--Author: Roberto Fernández Molanes (robertofem@gmail.com)

--Description:
This module moves data between a Linux Application running in user space and an FPGA memory in the FPGA. 
The module uses the "char driver" interface to connect application space and the driver. It creates a node in /dev a supplies support to the typical file functions:  open, close, write and read. When writing the data is copied using copy_from_user() function to an intermediate dma-able buffer in kernel space. Later on a transfer from the buffer to a memory in the FPGA is programed using the DMA Controller in HPS PL330. To program the PL330 transfer, the functions of the Altera´s hwlib were modified to work in kernel space (they are designed for baremetal apps). Better method would be to use "platform device" API to get information on the DMA from device tree and later use "DMA-engine" API to program the DMA transfer. However those APIs didn´t work and we were forced to do a less generic driver. Probably the DMA-engine options should be activated during compilation of the kernel but I was not able to do it.
