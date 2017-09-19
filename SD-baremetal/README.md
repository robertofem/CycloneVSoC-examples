SD-baremetal
=============

This brief tutorial explains how to build a SD card to run the baremetal examples provided in this repository.

Ways to configure the FPGA and run the baremetal application
------------------------------------------------------------
I have tested two ways to run a baremetal application:
* Using only preloader: the board powers up, loads the preloader from SD card and the preloader directly loads the baremetal application in RAM and runs it. The problem with this approach is that the FPGA configuration cannot be loaded from the SD card before baremetal application runs. The FPGA can be configured later inside the application using the FPGA configuration Manager or easier from EQCP device or directly loading it from Quartus.
* Using the U-boot: the board powers up, loads preloader from SD card and the preloader loads the U-boot. A U-boot script then tells the u-boot to load the FPGA hardware and later jumps into the baremetal application. This way is better because the FPGA is automatically configured before the baremetal application is executed.

Each way needs its own type of file containing the baremetal app. Thats why the baremetal compilation of all examples generates always two output files:
* baremetalapp.bin.img: to be loaded directly from preloader.
* baremetalapp.bin: to be loaded by the u-boot.

Steps to build the SD card
--------------------------
* Create partitions: Create an SD card with the same partitions needed when using Operating System explained in www.rocketboards.com. The partitions are:
	* Partition 1. FAT 32 partition containing:
		* baremetalapp.bin or baremetalapp.bin.img depending on the boot process.
		* soc_system.rbf: FPGA image to be loaded by the u-boot. Only needed when using the U-boot.
		* u-boot.img: U-boot image. In most tutorials this image goes in Partition 3. In this case I placed it in FAT32 partition (this way we can change u-boot from a Windows PC).
		* u-boot.scr: U-boot script. It is executed by the u-boot. It can change the default behaviour of u-boot. When U-boot finds this file it executes the instructions inside. Otherwise it executes the default behaviour. We use it to load the FPGA configuration and later run the baremetal application.
	
	* Partition 2: EXT3 partition to store the root file system when using OS (not used in baremetal).

	* Partition 3. RAW (no format) partition containing the preloader image. In most tutorials u-boot image is located in this partition too.

* Create a preloader: Create a preloader pointing to the next step in the boot process and write it into the partition 3. Remember to remove watchdog otherwise after few seconds the execution of the baremetal application will stop. The preloader is generated using the preloader generator as explained in www.rocketboards.com. When generating the preloader specify:
	*  "baremetalapp.bin.img" in partition 1 as next step in the boot process when using only preloader boot process.
	*  "u-boot.img" in partition 1 as next step in the boot process when using U-boot in the boot process. In the current folder a preloader with these characteristics is provided with the name preloader-mkpimage_baremetal_watchdogOFF.bin.

* Create U-boot (only needed when using u-boot during the boot process): compile U-boot as explained in www.rocketboards.com. For more information visit the u-boot website: http://www.denx.de/wiki/U-Boot/WebHome. The output of the compilation is "u-boot.img." Save it in partition 1. We provide "u-boot.img" in this folder.
* Create U-boot script (only needed when using u-boot during boot process): First create a u-boot script in human readable form describing the U-boot console instructions that should be executed by the u-boot. We do this in a file named u-boot.script. We configure the FPGA with a file named "soc_system.rbf", enable bridges between FPGA and HPS, copy the baremetal app described in a file called "baremetalapp.bin" to RAM memory and then jump the processor to the address where the baremetal app starts. This script file is compiled into a u-boot.src (visit www. rocketboards.com to lear how) and copied in partition 1. We provide both u-boot.src and u-boot.script in this folder.
* Create FPGA bitstream (only needed when using u-boot during boot process): The typical Quartus compilation outputs a .sof file with the FPGA hardware. To load it from the SD card we first need to convert the .sof file into .rbf file. This is done in Quartus->File->Convert Programming Files. The output should be called soc_system.rbf" to match the name used u-boot script for the FPGA configuration file so the u-boot can find it.
* Copy "baremetalapp.bin.img" or "baremetalapp.bin" to the partition 1, depending on the boot process selected (with preloader or with u-boot).

The following figure shows the aspect of the FAT32 partition when using u-boot:
<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-examples/master/SD-baremetal/FAT32-partition.png" width="600" align="middle" alt="Cyclone V SoC simplified block diagram" />
</p>


How to run
----------
* Put the SD card in the board.
* Set the MSEL switches:
	* into MSEL[5:0]="000000" position when using u-boot so the FPGA can be configured from SD card.
	* into MSEL[5:0]="110010" position when using only preloader so the FPGA can be configured by EPCQ device or Quartus programmer.
* Connect the board USB UART into the computer and Open a Serial Terminal session (using Putty or other program) at 115200bauds.
* Switch on the board.