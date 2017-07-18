Angstrom 2013.12 
============
This tutorial explains how to build an Angstrom 2012.12 SD card for Cyclone V SoC from scratch.

Table of contents:

1. [Installation of the tools](#1---installation-of-the-tools)
2. [SD card partitions and files](#2---sd-card-partitions-and-files)
3. [Booting process](#3---booting-process)
4. [Compile and u-boot kernel and root file system](#4---compile-and-u-boot-kernel-and-root-file-system)
5. [Create the partitions in the SD card](#5---create-the-partitions-in-the-sd-card)
6. [Generate and test the Preloader](#6---generate-and-test-the-preloader)
7. [Generate FPGA configuration file](#7---generate-fpga-configuration-file)
8. [Generate Device Tree Blob](#8---generate-device-tree-blob)
9. [Write u-boot script file and test the u-boot](#9---write-u-boot-script-file-and-test-the-u-boot)
10. [Write kernel and root-file system and test them](#10---write-kernel-and-root-file-system-and-test-them)

1 - Installation of the tools
---------------------------
First the tools used to build the SD card should be installed. In this case we use a single PC running:

* Windows 7 OS. We install:
    * Quartus II 14.1 to build hardware.
    * Altera SoC EDS 14.1 to run:
        * Altera DS-5: Integrated environment to easily write and debug applications for Altera SoC chips.
        * Altera embedded command shell: console with all environment variables set to run some useful applications like device tree generator or preloader generator.
    * Ubuntu 12.04 32bit virtual machine running inside the previous Windows 7 in a VMWare virtual machine. Used to compile OS for the embedded system and to build the SD Card.
All could be done inside an Ubuntu 12.04 machine installing Quartus II and Altera SoC EDS in it. 

2 - SD card partitions and files
----------------------------
As target board we are using DE1-SoC board from Terasic [4]. 
As hardware project for the FPGA we are using GHRD (Golden Hardware Reference Design) provided by Terasic inside the DE1-SoC CD-ROM. We copy this folder in a location in the Windows 7 system. We will call this folder [de1_soc_GHRD] after this point.
Inside the Ubuntu system we create a folder inside ~ folder called DE1SOC. We will refer to this folder as [DE1SOC]. We will use this folder as reference and all files generated in the Windows 7 system will be copied to this folder to later copied into the SD card.
Before looking at steps needed to complete the SD Card it is useful to watch at how the SD card should resemble at the end of the process.
The SD card contains 3 different partitions:

* Partition 1. FAT 32 partition containing:
    * zImage: image of the kernel of the operating system.
    * soc_system.dtb: Device tree blob defining the hardware to the OS.
    * soc_system.rbf: FPGA image to be loaded by the u-boot. In this case we want that the u-boot configures the FPGA before launching the OS.
    * u-boot.img:  U-boot image. In most tutorials this image goes in Partition 3. In this case we put it in FAT32 partition (this way we can change u-boot from a Windows PC).
o	u-boot.scr:  u-boot script. It is executed by the u-boot. It can change default variables of the u-boot (like the names of the Kernel, FPGA configuration file and Device Tree Blob file) and run some u-boot commands.  
* Partition 2: EXT3 partition to store the root file system of the embedded system.
* Partition 3. RAW (no format) partition containing the preloader image.  In most tutorials u-boot image is located in this partition.

More about the SD card in the SD Card Tutorial from Rocket Boards [5]. 

<p align="center">
  <img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/SD-operating-system/Angstrom-v2012.12/SD_card_partitions.png" width="200" align="middle" alt="Cyclone V SoC simplified block diagram" />
</p>

3 - Booting process
-----------------
The booting process for Cyclone V SoC when OS is present is the following. The preloader (only 64K) is loaded from the bootROM program (saved during chip manufacturing) inside the 64k On-Chip RAM. It configures external RAM memory (1GB in DE1-SoC) and performs other tasks. It loads u-boot in RAM and jumps to it. The u-boot loads some basic drivers needed to launch OS. It executes some commands to initialize the system, reads .dtb file and loads some basic drivers to work with some devices. The u-boot can configure the FPGA as ecplained later. In the end it loads the kernel image in RAM and passes execution to it. Using the u-boot.scr file the default behavior of u-boot can be modified. 

More about Cyclone V SoC boot in the [Preloader and U-boot Generation for Altera Cyclone V SoC](https://www.youtube.com/watch?v=vS7pvefsbRM) tutorial and in the Cyclone V SoC Handbook.

4 - Compile and u-boot kernel and root file system
--------------------------------------------------
Angstrom is, along with Poky Altera Linux, a distribution directly maintained by Altera. It is a famous distribution used i.e. in BeagleBone boards. Altera maintains the porting of Angstrom for the socfpga architecture used in Cyclone V SoC devices.

One advantage of Angstrom over Poky Altera Linux is its package manager (opkg command) that permits package installation in the system without recompilation of the OS. However software repositories for Cyclone V device are still limited and contain few packages. Software missing in repositories must be compiled from source and the added to the system with opkg command.

To compile Angstrom we have mainly followed the  [Rocketboards Tutorial](https://rocketboards.org/foswiki/view/Documentation/AngstromOnSoCFPGA_1#Angstrom_v2013.12) and the [tutorial in the official Angstrom website](http://www.angstrom-distribution.org/building-angstrom/). Both tutorials are very similar. We successfully compiled Angstrom for Cyclone V SoC using Debian 8.1.0-32bit, Debian 8.2.0-64bit and Ubuntu 12.04.4-32bit. The steps to be able to compile in these environments are exactly the same and they are explained below.

The Angstrom buildsystem is using various components from the Yocto Project, most importantly the Openembedded buildsystem, the bitbake task executor and various application and BSP layers. The [tutorial in the official Angstrom website](http://www.angstrom-distribution.org/building-angstrom/) recommends you to visit [OpenEmbedded website](http://www.openembedded.org/wiki/OEandYourDistro) before start compilation, in order to install all the necessary tools. So first step is to install the required compilation tools in Debian and Ubuntu we should do:
```bash
aptitude install sed wget cvs subversion git-core \
 coreutils unzip texi2html texinfo docbook-utils \
 gawk python-pysqlite2 diffstat help2man make gcc build-essential g++ \
 desktop-file-utils chrpath
 
apt-get install libxml2-utils xmlto python-psyco apr
```

If we have problems with dash try the following command to make /bin/sh to simbollically point to bash and not to dash (when question arises select no).
```bash
sudo dpkg-reconfigure dash
```

And then we make in a row the steps in [Rocketboards Tutorial](https://rocketboards.org/foswiki/view/Documentation/AngstromOnSoCFPGA_1#Angstrom_v2013.12), adding some changes. We put Terminal in the folder ~ and we do:
```bash
git clone git://git.rocketboards.org/angstrom-socfpga.git
```

First command clones the git repository for Angstrom into your computer. The folder  ~/ angstrom_socfpga is created. Using cd command enter angstrom-socfpga folder and select the branch v2013.12. 
```bash
cd angstrom-socfpga
git checkout -b angstrom-v2013.12-socfpga origin/angstrom-v2013.12-socfpga
```

At this point in time v2014.06 exists but the [Rocketboards Tutorial](https://rocketboards.org/foswiki/view/Documentation/AngstromOnSoCFPGA_1#Angstrom_v2013.12) still recommends v2013.12 to be installed. Then we go to the ~/angstrom_socfpga/conf/ and erase or comment (#) the line from the local.conf file: 
```bash
# INHERIT += "rm_work"
```

**THIS IS VERY IMPORTANT STEP**. This will prevent Angstrom kernel source from being removed once compilation has finished. Source is needed for kernel module compilation. Then we configure the compilation environment:
```bash
# Setup the Shell Environment
source environment-angstrom-v2013.12
# Configure the build environment for Cyclone5
MACHINE=socfpga_cyclone5 ./oebb.sh config socfpga_cyclone5
```

After doing this step we detect that meta-kde4 was not copied to the computer because it could not be found in the internet. This is because changes in the website hosting meta-kde4 layer (Gitorious was acquired by Gitlab). SO we searched the meta-kde4 layer in Google and we manually downloaded it from https://gitlab.com/cbrx-fw/meta-kde4 and pasted it in …/angstrom_socfpga/sources/. Then we configure the compilation environment again and everything should be correctly configured. 

Other option to solve this problem would be to modify the meta-kde4 line in layers.txt file in ~/angstrom_socfpga/sources/ so the configuration program can correctly find and download meta-kde4 layer from internet.
After solving that problem we compiled the kernel, the u-boot and the root filesystem. With the $PATH still being ~/angstrom_socfpga/ complete the remaining steps in the [Rocketboards Tutorial](https://rocketboards.org/foswiki/view/Documentation/AngstromOnSoCFPGA_1#Angstrom_v2013.12) in a row to compile Angstrom.
```bash
#Build ltsi (long term support) kernel (6 hours)
MACHINE=socfpga_cyclone5 bitbake linux-altera
#Build latest stable kernel (6 hours)
export KERNEL_PROVIDER="linux-altera"
export BB_ENV_EXTRAWHITE="${BB_ENV_EXTRAWHITE} KERNEL_PROVIDER"
MACHINE=socfpga_cyclone5 bitbake linux-altera
#Build boot-loader and pre-loader (10 minutes)
MACHINE=socfpga_cyclone5 bitbake virtual/bootloader
#Build the root filesystem (4 hours)
MACHINE=socfpga_cyclone5 bitbake console-image
```

After every compilation some warnings are expected. 
Tipically we only need to compile one of them: ltsi (kernel v3.10) or stable (v.18). In this case we have compiled both to test them. After compilation the images compiled for kernels, u-boot and root filesystem are stored in the folder:

* ~/angstrom_socfpga/deploy/eglibc/images/socfpga_cyclone5/

The source and configuration files of the kernel, needed to compile modules, are stored (depending on the kernel version) in the following folders:
* ~/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera-ltsi/3.10-r1/git
* ~/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/work/socfpga_cyclone5-angstrom-linux-gnueabi/linux-altera/3.18-r1/git

When compiling, a toolchain (compilers and libraries) is also crated. Compilation in the virtual machine running Ubuntu 32-bit gives the following path for the toolchain:

* ~/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/sysroots/i686-linux/

When the compilation took place in a Debian PC (not using a virtual machine), the path for the toolchain is Ubuntu 32-bit gives the following path for the toolchain:
* ~/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/sysroots/x86_64-linux/

The compilers of the toolchain will be used later to compile kernel modules. Compilers are located in:

* ~/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/sysroots/i686-linux/usr/bin/armv7ahf-vfp-neon-angstrom-linux-gnueabi/
or
* ~/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/sysroots/i386PC-linux/usr/bin/armv7ahf-vfp-neon-angstrom-linux-gnueabi/

5 - Create the partitions in the SD card
----------------------------------------
The SD card partitions can be achieved in different ways:
1. Using the provided script [5]: Having all files to build the SD card you can use a
provided Python script to partition the SD card and copy the files in it. Using the
commands at the end of [5] you can later individually replace each file.
2. Manually partition the SD card and copy files in it (Lab. 5 in [8]): You have more control
about the size of each partition and you can add files gradually (as you generate them).
3. Download a precompiled image and modify it. You can download a precompiled SD
image, as the ones provided by Terasic in DE1-SoC documentation website [4] and
modify it using the commands at the end of [5] or using graphical interface.
In this tutorial we use the third option. We download the Linux Console image
(DE1_SoC_SD.img) from Tersaic´s DE1-SoC website [4] and we write it into the SD card using
Win32DiskImager.
Using the Ubuntu Virtual machine we can access to the EXT3 partition (holding the root
filesystem) and to the FAT32 partition (storing a .dtb file and a kernel image file called zImage).
We erase the content in the FAT partition and EXT3 partition. Be careful if you remove it using
Nautilus or other graphical folder explorer because an occult folder called .Trash-0 is created
occupying space in the sdcard. Using the console, .Trash-0 folder can be easily removed. The
size of each partition in the SD card can be easily modified using GParted utility.

6 - Generate and test the Preloader
------------------------------------

7 - Generate FPGA configuration file
-------------------------------------

8 - Generate Device Tree Blob
-----------------------------

9 - Write u-boot script file and test the u-boot
------------------------------------------------

10 - Write kernel and root-file system and test them
----------------------------------------------------

    
