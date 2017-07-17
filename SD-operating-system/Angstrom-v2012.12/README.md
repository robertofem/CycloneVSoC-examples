Angstrom 2012.12 
============
This tutorial explains how to build an Angstrom 2012.12 SD card for Cyclone V SoC from scratch.

Table of contents:

* [Installation of the tools](#installation-of-the-tools)
* [SD card partitions and files](#SD-card-partitions-and-files)

# Installation of the tools

First the tools used to build the SD card should be installed. In this case we use a single PC running:

* Windows 7 OS. We install:
    * Quartus II 14.1 to build hardware.
    * Altera SoC EDS 14.1 to run:
        * Altera DS-5: Integrated environment to easily write and debug applications for Altera SoC chips.
        * Altera embedded command shell: console with all environment variables set to run some useful applications like device tree generator or preloader generator.
    * Ubuntu 12.04 32bit virtual machine running inside the previous Windows 7 in a VMWare virtual machine. Used to compile OS for the embedded system and to build the SD Card.
All could be done inside an Ubuntu 12.04 machine installing Quartus II and Altera SoC EDS in it. 

SD card partitions and files
----------------------------

As target board we are using DE1-SoC board from Terasic [4]. 
As hardware project for the FPGA we are using GHRD (Golden Hardware Reference Design) provided by Terasic inside the DE1-SoC CD-ROM. We copy this folder in a location in the Windows 7 system. We will call this folder [de1_soc_GHRD] after this point.
Inside the Ubuntu system we create a folder inside ~ folder called DE1SOC. We will refer to this folder as [DE1SOC]. We will use this folder as reference and all files generated in the Windows 7 system will be copied to this folder to later copied into the SD card.
Before looking at steps needed to complete the SD Card it is useful to watch at how the SD card should resemble at the end of the process.
The SD card contains 3 different partitions:
•	Partition 1. FAT 32 partition containing:
o	zImage: image of the kernel of the operating system.
o	soc_system.dtb: Device tree blob defining the hardware to the OS.
o	soc_system.rbf: FPGA image to be loaded by the u-boot. In this case we want that the u-boot configures the FPGA before launching the OS.
o	u-boot.img:  U-boot image. In most tutorials this image goes in Partition 3. In this case we put it in FAT32 partition (this way we can change u-boot from a Windows PC).
o	u-boot.scr:  u-boot script. It is executed by the u-boot. It can change default variables of the u-boot (like the names of the Kernel, FPGA configuration file and Device Tree Blob file) and run some u-boot commands.  
•	Partition 2: EXT3 partition to store the root file system of the embedded system.
•	Partition 3. RAW (no format) partition containing the preloader image.  In most tutorials u-boot image is located in this partition.
More about the SD card in the SD Card Tutorial from Rocket Boards [5]. 
 

The booting process is the following. The preloader (only 64K) is loaded from the bootROM program (saved during chip manufacturing) inside the 64k On-Chip RAM. It configures external RAM memory (1GB in DE1-SoC) and performs other tasks. It loads u-boot in RAM and jumps to it. The u-boot loads some basic drivers needed to launch OS. It executes some commands to initialize the system, reads .rbf file and loads some basic drivers to work with some devices. The u-boot can configure the FPGA. In the end it loads the kernel image in RAM and passes execution to it. Using the u-boot.scr file the default behavior of u-boot can be modified. 
In the following subsections we are going to build an SD card to work with the DE1-SoC board. The steps we have followed are: 
1.	Compile u-boot, kernel and root file system (Yocto Altera Linux)
2.	Compile and u-boot kernel and root file system (Angstrom) 
3.	Create the partitions in the SD card
4.	Generate and test the Preloader
5.	Generate FPGA configuration file
6.	Generate Device Tree Blob
7.	Write u-boot.scr file and test the u-boot
8.	Write kernel and root-file system and test them

    
