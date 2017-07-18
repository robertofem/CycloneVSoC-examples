Angstrom 2013.12 
================
This tutorial explains how to build an Angstrom 2013.12 SD card for Cyclone V SoC from scratch. The folder _Files_to_build_SD_DE1-SoC_ contains the different files generated during this tutorial so the user can save time when building the SD card.Files_to_build_SD_DE1-SoC

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
-----------------------------
First the tools used to build the SD card should be installed. In this case we use a single PC running:

* Windows 7 OS. We install:
    * Quartus II 14.1 to build hardware.
    * Altera SoC EDS 14.1 to run:
        * Altera DS-5: Integrated environment to easily write and debug applications for Altera SoC chips.
        * Altera embedded command shell: console with all environment variables set to run some useful applications like device tree generator or preloader generator.
    * Ubuntu 12.04 32bit virtual machine running inside the previous Windows 7 in a VMWare virtual machine. Used to compile OS for the embedded system and to build the SD Card.
All could be done inside an Ubuntu 12.04 machine installing Quartus II and Altera SoC EDS in it. 

2 - SD card partitions and files
--------------------------------
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
  <img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/SD-operating-system/Angstrom-v2013.12/figs/SD_card_partitions.png" width="200" align="middle" alt="SD card partitions" />
</p>

3 - Booting process
-------------------
The booting process for Cyclone V SoC when OS is present is the following. The preloader (only 64K) is loaded from the bootROM program (saved during chip manufacturing) inside the 64k On-Chip RAM. It configures external RAM memory (1GB in DE1-SoC) and performs other tasks. It loads u-boot in RAM and jumps to it. The u-boot loads some basic drivers needed to launch OS. It executes some commands to initialize the system, reads .dtb file and loads some basic drivers to work with some devices. The u-boot can configure the FPGA as ecplained later. In the end it loads the kernel image in RAM and passes execution to it. Using the u-boot.scr file the default behavior of u-boot can be modified. 

More about Cyclone V SoC boot in the [Preloader and U-boot Generation for Altera Cyclone V SoC](https://www.youtube.com/watch?v=vS7pvefsbRM) tutorial and in the Cyclone V SoC Handbook.

4 - Compile and u-boot kernel and root file system
--------------------------------------------------
Angstrom is, along with Poky Altera Linux, a distribution directly maintained by Altera. It is a famous distribution used i.e. in BeagleBone boards. Altera maintains the porting of Angstrom for the socfpga architecture used in Cyclone V SoC devices.

One advantage of Angstrom over Poky Altera Linux is its package manager (opkg command) that permits package installation in the system without recompilation of the OS. However software repositories for Cyclone V device are still limited and contain few packages. Software missing in repositories must be compiled from source and the added to the system with opkg command.

To compile Angstrom we have mainly followed the  [Rocketboards Angstrom Tutorial](https://rocketboards.org/foswiki/view/Documentation/AngstromOnSoCFPGA_1#Angstrom_v2013.12) and the [tutorial in the official Angstrom website](http://www.angstrom-distribution.org/building-angstrom/). Both tutorials are very similar. We successfully compiled Angstrom for Cyclone V SoC using Debian 8.1.0-32bit, Debian 8.2.0-64bit and Ubuntu 12.04.4-32bit. The steps to be able to compile in these environments are exactly the same and they are explained below.

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

And then we make in a row the steps in [Rocketboards Angstrom Tutorial](https://rocketboards.org/foswiki/view/Documentation/AngstromOnSoCFPGA_1#Angstrom_v2013.12), adding some changes. We put Terminal in the folder ~ and we do:
```bash
git clone git://git.rocketboards.org/angstrom-socfpga.git
```

First command clones the git repository for Angstrom into your computer. The folder  ~/ angstrom_socfpga is created. Using cd command enter angstrom-socfpga folder and select the branch v2013.12. 
```bash
cd angstrom-socfpga
git checkout -b angstrom-v2013.12-socfpga origin/angstrom-v2013.12-socfpga
```

At this point in time v2014.06 exists but the [Rocketboards Angstrom Tutorial](https://rocketboards.org/foswiki/view/Documentation/AngstromOnSoCFPGA_1#Angstrom_v2013.12) still recommends v2013.12 to be installed. Then we go to the ~/angstrom_socfpga/conf/ and erase or comment (#) the line from the local.conf file: 
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
After solving that problem we compiled the kernel, the u-boot and the root filesystem. With the $PATH still being ~/angstrom_socfpga/ complete the remaining steps in the [Rocketboards Angstrom Tutorial](https://rocketboards.org/foswiki/view/Documentation/AngstromOnSoCFPGA_1#Angstrom_v2013.12) in a row to compile Angstrom.
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
1. Using the provided script [Rocketboards SD Tutotial](http://rocketboards.org/foswiki/view/Documentation/GSRD141SdCard): Having all files to build the SD card you can use a provided Python script to partition the SD card and copy the files in it. Using the commands at the end of [Rocketboards SD Tutotial](http://rocketboards.org/foswiki/view/Documentation/GSRD141SdCard) you can later individually replace each file.
2. Manually partition the SD card and copy files in it (Lab. 5 in the [Altera WorkShop 2 Linux Kernel for Altera SoC Devices](http://rocketboards.org/foswiki/view/Documentation/WS2LinuxKernelIntroductionForAlteraSoCDevices)): You have more control about the size of each partition and you can add files gradually (as you generate them).
3. Download a precompiled image and modify it. You can download a precompiled SD image, as the ones provided by Terasic in DE1-SoC documentation website [Terasic´s DE1-SoC board web site](http://de1-soc.terasic.com/) and modify it using the commands at the end of [Rocketboards SD Tutorial](http://rocketboards.org/foswiki/view/Documentation/GSRD141SdCard) or using graphical interface.

In this tutorial we use the third option. We used the DE1-SoC board so we download the Linux Console image (DE1_SoC_SD.img) from [Terasic´s DE1-SoC board web site](http://de1-soc.terasic.com/) and we write it into the SD card using Win32DiskImager.

<p align="center">
	<img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/SD-operating-system/Angstrom-v2013.12/figs/Win32_disk_imager.png" width="400" align="middle" alt="win32 disk imager screenshoot" />
</p>

Using the Ubuntu Virtual machine we can access to the EXT3 partition (holding the root filesystem) and to the FAT32 partition (storing a .dtb file and a kernel image file called zImage). We erase the content in the FAT partition and EXT3 partition. Be careful if you remove it using Nautilus or other graphical folder explorer because an occult folder called .Trash-0 is created occupying space in the sdcard. Using the console, .Trash-0 folder can be easily removed. The size of each partition in the SD card can be easily modified using GParted utility.

6 - Generate and test the Preloader
-----------------------------------
The preloader is generated using the preloader generator and the information in the Handoff folder that is inside the hardware folder. In our case, we used DE1-SoC board in this tutorial so we use the Golden Hardware Reference Design for DE1-SoC (It comes with the board documentation. We will call to this folder [de1_soc_GHRD] folder in the rest of this tutorial. This tutorial was done using the LAB1 of [Altera WorkShop 2 Linux Kernel for Altera SoC Devices](http://rocketboards.org/foswiki/view/Documentation/WS2LinuxKernelIntroductionForAlteraSoCDevices).

To generate the preloader open the embedded command shell located at <path-to-soceds-tools>/embedded/embedded_command_shell.sh (in /altera/14.1/embedded/embedded_command_shell.sh in our case). Browse to the hardware project, in our case the GHRD for DE1-SoC or [de1_soc_GHRD] folder. Before launch the preloader generator we need to know if our board supports ECC storage or not. Type the following to find out.
```bash
grep "RW_MGR_MEM_DATA_WIDTH" ./hps_isw_handoff/soc_system_hps_0/sequencer_defines.h
```

If your macro for your board is set to 24 or 40, then your board supports ECC storage, and you will need to enable SDRAM scrubbing in the BSP Editor. If your macro for your board is set to 8, 16, or 32, then your board does not support ECC storage, and you will need to disable SDRAM scrubbing in the BSP Editor. Our macro is 32 so we will not enable SDRAM scrubbing.
After that, call the bsp-editor writing the following in the command shell:
```bash
bsp-editor
```

<p align="center">
  <img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/SD-operating-system/Angstrom-v2013.12/figs/preloader1.png" width="700" align="middle" alt="Altera Embedded Command Shell" />
</p> 
 
The bsp-editor GUI appears and a new preloader can be generated. Go to File->New BSP and select the folder [de1_soc_GHRD]/ hps_isw_handoff/soc_system_hps_0 as source for the Hardware. Leave the defaults in the software part and press OK.

<p align="center">
  <img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/SD-operating-system/Angstrom-v2013.12/figs/preloader2.png" width="800" align="middle" alt="BSP-Editor" />
</p>

In the BSP Editor, under Settings->Common we select BOOT_FROM_SDMMC to indicate that boot-loader and OS is stored in a SD card (selected by default). In Settings->Common we select FAT_SUPPORT and we write the value 1 in the field FAT_BOOT_PARTITION to store u-boot in the FAT partiotion of the SD. FAT_LOAD_PAYLOAD_NAME defines the name of the u-boot image file. We leave default name “u-boot.img” as image file name.
 
<p align="center">
  <img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/SD-operating-system/Angstrom-v2013.12/figs/preloader3.png" width="800" align="middle" alt="BSP-Editor 2" />
</p>

Finally, under Settings->Advanced->spl->boot we select SDRAM_SCRUBBING if the board supports ECC storage. In our case, DE1-SoC does not support ECC storage and we leave SDRAM_SCRUBBING unchecked (default).

Then we press Generate button to generate BSP for the Preloader.

<p align="center">
  <img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/SD-operating-system/Angstrom-v2013.12/figs/preloader4.png" width="800" align="middle" alt="BSP-Editor 3" />
</p>
 
We have got some errors because the version of Quartus used to generate this project is older than the one used to create the preloader (14.1). For this reason we have opened the project using Quartus 14.1 and recompiled the project. We have generated the Qsys system again, have added the .quip file to the project (.quip file points to the location of the IP cores used in Qsys), have executed hps_sdram_p0_parameters.tcl and hps_sdram_p0_pin_assignments.tcl files using Tools->Tcl Scripts… (following the indications of My First HPS-FPGA tutorial in DE1-SoC Documentation CD) and have compiled using Quartus II.

If we come back to the bsp-editor again and we press Generate the BSP is generated without errors.
 
<p align="center">
  <img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/SD-operating-system/Angstrom-v2013.12/figs/preloader5.png" width="800" align="middle" alt="BSP-Editor 4" />
</p>
 
Now, exit from the bsp-editor and using the Embedded Command Shell navigate to the newly created folder [de1_soc_GHRD]/software/spl_bsp/ and type make. This command will extract the fixed part of the preloader and using the fixed part and the previously generated part will build the preloader image. The preloader image, called preloader-mkpimage.bin is stored in [de1_soc_GHRD]/software/spl_bsp/. 
We copy the image file into folder [DE1SOC] in the Ubuntu running inside the virtual machine and we put it into the A2 partition (partition3) of the SD card. Use dd command to do it:
```bash
sudo dd if=preloader-mkpimage.bin of=/dev/sdx3 bs=64k seek=0
```

To know the name of the sdcard (named using sdx scheme) run the command cat /proc/partitions before and after inserting the SD card. In our case the name is sdd. Here the result in the Ubuntu console.

<p align="center">
	<img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/SD-operating-system/Angstrom-v2013.12/figs/preloader6.png" width="700" align="middle" alt="Copy preloader into SD card" />
</p>

Finally we can test the preloader in the board. We insert the SD in the board and see the result in the serial console. We should see the preloader trying to load 4 times and always failing to load u-boot image file. This is because it looks for the u-boot image in the FAT32 partition which is now empty.

<p align="center">
	<img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/SD-operating-system/Angstrom-v2013.12/figs/preloader7.png" width="700" align="middle" alt="Copy preloader into SD card" />
</p>

7 - Generate FPGA configuration file
------------------------------------
To program the FPGA from software, .sof FPGA configuration file (generated in quartus project folder during quartus compilation) should be converted to .rbf file using either the embedded console or the Quartus II (like explained in [GUI Compile GSRD for Arrow SoCKit](http://rocketboards.org/foswiki/view/Documentation/GSRDCompileHardwareDesignArrowSoCKitEdition#Converting_.sof_to_.rbf)). In this case we use Quartus II GUI for ease.

From Quartus go to **File->Convert Programming Files**, select .rbf as **Programming Type File** and  **Passive Parallel x8** or **x16** as **Mode**. Click on **SOF Data** and the click on **Add File** and browse the .sof source file. Write in File name the path and name of the .rbf output file. We select the same name as the .sof file (soc_system.rbf) as name and the path is the quartus project folder where the .sof file is located.Finally, click on the **Generate** button.

<p align="center">
	<img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/SD-operating-system/Angstrom-v2013.12/figs/sof_to_rbf.png" width="700" align="middle" alt="Sof to rbf conversion" />
</p>

Copy the generated file into the FAT32 partition of the SD card. It will be later used to configure FPGA from u-boot.

8 - Generate Device Tree Blob
-----------------------------
Device tree blob or DTB (.dtb file) describes hardware for the OS kernel. This way there is no information about the hardware in the compiled kernel image. This is useful to change hardware without recompile the kernel (very useful when having peripherals in FPGA). To generate the DTB file three source files are needed as input to sopc2dts (a piece of software coming with the Altera SoC EDS): .sopc_info file describing the Qsys hardware and two .xml files describing other information not contained in .sopc_info file like drivers to load, information about clocks, information about hardware in the board, etc… .dts file is in human readable form. It can be opened using a text editor. Using the device tree generator or dtc (another piece of software coming with the Altera SoC EDS) and .dts file as source, a .dtb file is finally generated.

This file has created all the problems when trying to change kernel image. All the kernel images we compiled didn´t run because this file. We though the problem was the kernel image but it was this file. When we were able to generate an appropriate DTB we could run all the generated kernel images easily, just changing the zImage file (kernel image) in the FAT32 partition. 

We enumerate now the steps done till we were able to run the Angstrom kernel image  compiled in section 4 in DE1-SoC board:
1.	We downloaded the Console image from the [Terasic´s DE1-SoC board web site](http://de1-soc.terasic.com/) and put it in the SD card using Win32DiskImager. This image correctly runs in DE1-SoC.
2.	We replaced the zImage in partition 1 by the one compiled for Angstrom. It didn´t run. Execution stops when loading kernel.
3.	We replaced the filesystem in partition 2 by the filesystem compiled during angstrom compilation, leaving the Angstom image in partition 1. It didn´t run. Execution stops when loading kernel.
4.	We left the Angstrom filesystem in partition 2 and we put the original zImage (image inside the original Terasic´s Console image) in the SD again. It ran. For this reason we though the problem was the kernel image and we started to compile different versions of Yocto and Angstrom. But it wasn´t. In that moment we started to change the device tree blob.
5.	We put in the partition 1 of the SD card zImage compiled in section 4 and in partition 2 the root filesystem also compiled in section 4. Then we changed the .dtb file in partition 1 by the .dtb contained in the GHRD provided by Terasic in DE1-SoC CD-ROM documentation. It didn´t run. Execution stops when loading kernel again.
6.	Using sopc2dts and dtc utilities we generated again the .dtb file of the GHRD using 14.1 tools. We used hps_clock_info.xml and soc_system_board_info.xml (the only two .xml files in GHRD in Terasic´s DE1-SoC CD ROM documentation) and soc_system.sopc_info as input files for the sop2dts. It didn´t run again. It stopped when running kernel again.
7.	Then we started the [Altera WorkShop 2 Linux Kernel for Altera SoC Devices](http://rocketboards.org/foswiki/view/Documentation/WS2LinuxKernelIntroductionForAlteraSoCDevices), that explains how to generate DTB for several boards (unfortunately not for DE1-SoC).  We generated the DTB for DE0 nano, a very similar board to the DE1-SoC board. The tutorial include the following files: soc_system.sopcinfo, DE0NANO_board_info.xml and hps_common_board_info.xml. Using this three files and the indications in the tutorial we generated the .dts file and later the .dtb file. It didn´t work out. Execution stopped when loading kernel. In this case, it didn´t probably run because the specified hardware (DE0NANO-SoC) was different from the actual one (DE1-SoC).
8.	Then, in a forum, a person said that after version 14 of the Altera software hps_clock_info.xml is no more needed. So we combined the steps 6 and 7. We used soc_system.sopcinfo and soc_system_board_info.xml from GHRD (step 6) and hps_common_board_info.xml from Workshop 2 tutorial (step 7) as source for sopc2dts to generate a .dts file. Then, using dtc utility we generated .dtb file. In this case the kernel launched correctly!!!!

Now we explain the procedure to get a correct .dtb file (the same used in step 8 previously explained). First we take the hps_common_board_info.xml (we provide this file with this document to ease acquire this file from Workshop 2) used in step 8 inside [de1_soc_GHRD] with the rest of the files of the hardware project. Now we open the Altera Embedded Command Shell and navigate to the [de1_soc_GHRD] folder. Then we call the following line to generate .dts file:

```bash
sopc2dts –-input soc_system.sopcinfo\
 –-output socfpga.dtb\
 –-type dtb\
 –-board soc_system_board_info.xml\
 --board hps_common_board_info.xml\
 –-bridge-removal all\
 --clocks
```
 
soc_system_board_info.xml should be provided by Terasic in GHRD folder. soc_system.sopcinfo was generated when compiled the qsys system for the GHRD.
After typing this command a .dtb file should have been generated.

<p align="center">
	<img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/SD-operating-system/Angstrom-v2013.12/figs/dtb_generation.png" width="700" align="middle" alt="dtb generation" />
</p>

Some DTAppend warnings appear. Altera website says they are OK. So we copy the the socfpga.dtb file into the FAT32 partition of the SD. We have called DTB as socfpga.dtb because it is the default name that the u-boot is going to use to find a DTB in the FAT32 partition.
Attached to this document we provide the .xml files needed to compile the .dtb and the .dtb itself to save you time. In addition to this .xml files we provide other .xml files from a developer that we have found in Rocketboards forum. This developer asked Terasic for the .xml files to build the .dtb and Terasic answered him sending these files. They are slightly different from the ones explained before but the system correctly boots and works with them.

9 - Write u-boot script file and test the u-boot
------------------------------------------------
Now it is time to write the u-boot, the second stage during the booting process, into the SDCARD. Using a u-boot.scr file we are also going to modify the default behavior of the u-boot to configure the FPGA from .rbf file, unlock the FPGA-HPS bridges and load the kernel image. This chapter is based on the Lab 2 of the [Altera WorkShop 2 Linux Kernel for Altera SoC Devices](http://rocketboards.org/foswiki/view/Documentation/WS2LinuxKernelIntroductionForAlteraSoCDevices). For more information on u-boot and FPGA programming visit [Das U-Boot official website](http://www.denx.de/wiki/U-Boot) and [Configure FPGA from software](http://rocketboards.org/foswiki/view/Documentation/GSRD131ProgrammingFPGA) respectively.

The default u-boot behavior is looking for a u-boot.scr and execute it. If it does not find the file it loads a .dtb file with the default name socfpga.dtb and loads the kernel image. Loading the kernel implies reading the the image file (with default name zImage), copying it into RAM and passing the execution to the kernel).  One can check that these are the default names that the u-boot is using accessing to the u-boot variables holding this value. When booting, cancel autoboot hitting any key to enter in the u-boot console, then try:
```bash
echo $bootimage # gives the default name for kernel (zImage in our case).
echo $fdtimage  # gives the default name for the device tree file (socfpga.dtb in our case)
```

<p align="center">
	<img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/SD-operating-system/Angstrom-v2013.12/figs/uboot_console.png" width="400" align="middle" alt="dtb generation" />
</p>

To modify the default behavior one can write the so-called u-boot.script (human readable file with u-boot console instructions) and compile it to get the u-boot.scr. Our u-boot.script file looks like:
```bash
fatload mmc 0:1 $fpgadata soc_system.rbf;
fpga load 0 $fpgadata $filesize;
run bridge_enable_handoff;
run mmcload;
run mmcboot;
```
 
The first and second lines load the fpga configuration file and configure the FPGA part, respectively. The name of the file in the first line should match with the actual file in the FAT32 partition. The third line enables the FPGA-HPS bridges. The last two lines load and boot the kernel.

To compile the u-boot.script navigate to the folder where u-boot.script is located using the embedded command shell and type:
```bash
mkimage  -A arm -O linux -T script -C none -a 0 -e 0 -n "U-boot script" -d boot.script u-boot.scr
```

<p align="center">
	<img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/SD-operating-system/Angstrom-v2013.12/figs/uboot_script_generation.png" width="650" align="middle" alt="dtb generation" />
</p>

Now we copy the u-boot.scr file along with the uboot image into the FAT32 partition of the SD card. The u-boot image was generated during the compilation of the u-boot and it is located in the folder with root filesystem and kernel image (see section 3.5.2) with the name of u-boot-socfpga_cyclone5.img. Copy it to the FAT32 partition and change its name to u-boot.img, in order the preloader to be able to find it. Remember that, unlike most of tutorials, u-boot image should be located in FAT32 partition because we said so to the preloader. At this moment the FAT partition on the SD should contain u-boot.img, u-boot.scr, socfpga.dtb and soc_system.rbf. Powering-up the board with this SD card should give the following output.
 
<p align="center">
	<img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/SD-operating-system/Angstrom-v2013.12/figs/uboot_fails_zimage.png" width="350" align="middle" alt="dtb generation" />
</p>

The output shows how the u-boot.scr file is correctly loaded. The soc_system.rbf file is loaded and the FPGA configured. The u-boot correctly reads the .rbf file but does not find the kernel image zImage.


10 - Write kernel and root-file system and test them
----------------------------------------------------
Last step is to copy zImage and filesystem into the SD to have a complete bootable SD card with FPGA configuration by u-boot. 
We have compiled two different versions of the Linux kernel during Amstrong compilation. In the folder containing the compilation images we find:
* Linux kernel 3.10ltsi image: zImage--3.10-r1.2-socfpga_cyclone5-20150925075605.img
* Linux kernel 3.18 image: zImage--3.18-r1.2-socfpga_cyclone5-20150925161328.img

Copy one of the images to the FAT32 partition and change its name to zImage. 

The root file system is compressed in a folder called:
* Angstrom-console-image-eglibc-ipk-v2013.12-socfpga_cyclone5.rootfs.tar.gz

To copy the root file system into the SD is little more tricky. Go to a linux-based OS to be able to see the partition 2 (EXT3 partition). Open the folder navigation program as root and decompress this file into the partition 2. You can either use the console and copy the .img file also containing the filesystem. The file name is: 
* Angstrom-console-image-eglibc-ipk-v2013.12-socfpga_cyclone5.rootfs.img

Using dd command it can be copied to the partition 2 of the SD card: 
```bash
sudo dd if=altera-gsrd-image-socfpga_cyclone5.ext3 of=/dev/sdx2 
```

Booting now DE1-SoC board with the complete SD card gives the following output (for the 3.10 kernel).

<p align="center">
	<img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/master/SD-operating-system/Angstrom-v2013.12/figs/Angstrom_loads_correctly.png" width="600" align="middle" alt="dtb generation" />
</p>

We have tested both, 3.10 and 3.18 kernel. Both of them correctly load. However 3.18 kernel is not does not recognize Ethernet peripheral and cannot be connected to the Internet. This problem is probably related with .dtb files since the conventions to write a .dtb file change from one kernel to another. To solve this problem the best way is to go deeply into .dtb files and learn to write our own. One good starting point could be the basic .dtb files included in the folder where file system, kernel and u-boot images are located after compilation. Because of this problem and without further investigation about its solution we have selected 3.10 kernel for our projects.

    
