====================
Angstrom build guide
====================
This guide follows the process for building an SD card with an Angstrom 2016.12 image to run in a Cyclone V
based SoC FPGA device.

*Disclaimer: The versions of the software built on this guide may not be the latests releases
available. This is intentional in order to make a guide that will result in an usable system. In
case there are newer versions, you are free to employ them, but we cannot assure the system
will build correctly.*

============
Requirements
============
In order to build an SD card image capable of running Angstrom for a Cyclone V based device,
the following is needed:

- Blank SD card
- Linux PC
- Quartus software
- Access to GitHub
- Terminal emulator application


===============
Hardware design
===============
The hardware design used in this guide is the Golden Hardware Reference Design for the DE1-SoC
board (from now on, GHRD). This design can be obtained from the board system CD (at
'/Demonstrations/SOC_FPGA/de1_soc_GHRD/'). In order to not alter the initial design, make a copy
of the directory to work with.

Once the design is compiled, a .sof file is created (SRAM object file). This file can be used to
the FPGA using a JTAG programmer. However, we will use the bootloader to program the FPGA before
Angstrom starts up. For this purpose, the .sof file has to be coverted to a .rbf file (Raw Binary
File).

In order to obtain the .rbf file, follow the steps below:
1. Within Quartus, open the "Convert Programming Files" window (File -> Convert Programming Files).
2. As "Programming file type", pick "Raw Binary File (.rbf)".
3. As "Mode", pick "Passive Parallel x16" (default mode for programming from Linux).
4. Write a name for the resulting file (in this guide, we'll use "soc_system.rbf").
5. Click on "SOF Data", and then click the "Add File..." button to add the .sof file.
6. Click "Generate" and the .rbf file will be created.


=========
Preloader
=========
The preloader is a piece of software that gets called from the Boot ROM with the purpose of setting
up the system to a point that the actual bootloader can be run. To generate it, we will use a tool
called "BSP Editor".

In order to run this tool, we have to start the embedded command shell:

.. code-block:: bash

   <path-to-soceds-tools>/embedded/embedded_command_shell.sh
   bsp-editor &

Once the window is open, create a new BSP project (File -> New HPS BSP). In the dialog box that
opens, navigate to the handoff directory ('de1_soc_GHRD/hps_iws_handoff/soc_system_hps_0').
The rest of the settings should be automatically populated, so just click "Ok" to accept them.

While the defaults are almost exactly the ones we require, we should make a couple of checks.
Click on "Common" in the tree on the left. In the options that appear on the right, make sure that
"BOOT_FROM_SDMMC" is enabled, since we will be putting U-Boot on our SD Card. Since it will be
placed on a FAT partition, "FAT_SUPPORT" should also be enabled, "FAT_BOOT_PARTITION" set to "1",
and "FAT_LOAD_PAYLOAD_NAME" set to "u-boot.img". The last option that should be enabled is
"SERIAL_SUPPORT", and can be found under "Advanced/spl/performance" in the tree on the left.
Once all options are set, click on "Generate", wait until it has finished, and then click "Exit".

Navigate to 'de1_soc_GHRD/software/spl_bsp', where the generated preloader files are located. To
generate the preloader image, run the makefile:

.. code-block:: bash

   make

After compilation, you’ll find a new “uboot-socfpga” directory which contains a subfolder called
"spl" that contains the code for the secondary program loader that the generated code was merged
with. There’s also a new “preloader-mkpimage.bin” file that contains both the preloader image and
a “Boot ROM Header” that is required for the Boot ROM to recognize this as an actual preloader
image. We will burn this file to an SD card later on.


======
U-Boot
======
The purpose of U-Boot is to get the system up to the point the Linux kernel can be started up (in
the same way the purpose of the preloader was to set up the system to run U-Boot).

To compile UBoot, we are going to use the Linaro GCC toolchain. To obtain it and make it ready
for compilation, run the following commands:

.. code-block:: bash

   wget http://releases.linaro.org/14.09/components/toolchain/binaries/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf.tar.xz
   tar -xvf gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf.tar.xz
   export CROSS_COMPILE=$PWD/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-

In case this release can not be downloaded, the available releases can be checked
`here <https://releases.linaro.org/components/toolchain/binaries/>`_. Make sure you obtain the
latest release of the 4 version, for the ARMv7 architecture, and modify the CROSS_COMPILE
environment variable accordingly your version.

After setting up the compiler, download Altera's version of U-Boot from GitHub:

.. code-block:: bash

   git clone https://github.com/altera-opensource/u-boot-socfpga.git
   cd u-boot-socfpga
   git checkout rel_socfpga_v2013.01.01_15.09.01_pr

To compile U-Boot, execute the following commands:

.. code-block:: bash

   make mrproper                  # Cleans the working directory
   make socfpga_cyclone5_config   # Applies the Cyclone V configuration
   make                           # Compiles U-Boot

The compiled U-Boot image will be called "u-boot.img".

Aside from the image, a boot script is needed in order to run specific u-boot commands on boot up. The script will program the FPGA, load the device tree, load the kernel, and run the kernel
with some boot arguments. If we do not add an -u-boot script along with the u-boot the u-boot will behave in a default way and just load the kernel. To generate a u-boot script create a file challed "boot.script" and type the following commands:

.. code-block:: guess

   echo -- Programming FPGA --
   # Load the FPGA bitstream and place it in RAM
   fatload mmc 0:1 $fpgadata soc_system.rbf;
   fpga load 0 $fpgadata $filesize;
   # Enable HPS-to-FPGA bridges
   run bridge_enable_handoff;

   echo -- Setting Env Variables --
   # Load device tree
   setenv fdtimage soc_system.dtb;
   # Locate root filesystem
   setenv mmcroot /dev/mmcblk0p2;
   # Command for loading kernel
   setenv mmcload 'mmc rescan;${mmcloadcmd} mmc 0:${mmcloadpart} ${loadaddr} ${bootimage};${mmcloadcmd} mmc 0:${mmcloadpart} ${fdtaddr} ${fdtimage};';
   # Command for booting kernel
   setenv mmcboot 'setenv bootargs console=ttyS0,115200 root=${mmcroot} rw rootwait; bootz ${loadaddr} - ${fdtaddr}';

   # Load kernel
   run mmcload;
   # Boot kernel
   run mmcboot;

To run this script, it must be compiled with the following command:

.. code-block:: bash

   mkimage -A arm -O linux -T script -C none -a 0 -e 0 -n "Boot Script Name" -d boot.script u-boot.scr

The output is the file u-boot.src that will be also loaded in the SD-card.


======================
SD Card Image Creation
======================
To boot the device, we need a SD card properly partitioned:

- The first partition (FAT) will contain the U-Boot image, its boot script, the device tree binary
  and the kernel image.
- The second partition (EXT4) will contain the root filesystem (all the files and programs).
- The third partition (RAW) will contain the preloader image.

The following commands assume a 512MB SD Card. In case the card is bigger, it is still recommended
to follow them to speed up the image creation process, and after it is completed, adjust partition
sizes (for example, extending the root filesystem partition) with gparted.

**Be careful when running these commands! "dd" used on the wrong device can destroy the host system!**

.. code-block:: bash

   sudo dd if=/dev/zero of=sdcard.img bs=512M count=1  # Creates an empty card image
   sudo losetup --show –f sdcard.img                   # Sets up a loopback device to manipulate it
   sudo fdisk /dev/loop0                               # Starts partition utility

First we will create the preloader partition. Use the "n" command, and select the following options:

- Partition type: p (primary)
- Partition number: 3
- First sector: default (leave empty)
- Last sector: +1M (1 MB)

By default, fdisk creates "Linux" type partitions. Since we need RAW type, we must
change it with the "t" command. Partition 3 should be autoselected since it is the only one. When
prompted for a new type, time "a2". It will show up as "unknown".

Next we will create the root filesystem partition. Use the "n" command again, and select the
following options:

- Partition type: p (primary)
- Partition number: 2
- First sector: default (leave empty)
- Last sector: +254M (254 MB)

In this case, "Linux" type is the correct one, so nothing else needs to be done.

Now we create the FAT partition to store out boot files. "n" command, and the following options:

- Partition type: p (primary)
- Partition number: 1
- First sector: default (leave empty)
- Last sector: default (leave empty, will take the rest of the space, 256 MB)

After it is created, use the "t" command to change its type. Select partition 1, and use "b"
as partition type. It should show its new filesystem is "W95 FAT32".

In case you want to check everything is correct, use the "p" command to list the partition table.
Lastly, write the changes using the "w" command. In order for the OS to notice the new partitions,
run the following command:

.. code-block:: bash

   sudo partprobe /dev/loop0

With the partitions created, we can create their filesystems (if needed) and start copying files to
them. The preloader partition does not need a filesystem, just copy the preloader image into it:

.. code-block:: bash

   sudo dd if=software/spl_bsp/preloader-mkpimage.bin of=/dev/loop0p3 bs=64k seek=0

Next, create the FAT filesystem in the FAT partition. In order to do this, execute:

.. code-block:: bash

   sudo mkfs –t vfat /dev/loop0p1

Now mount the partition, and copy U-Boot and FPGA files inside this partition.

.. code-block:: bash

   mkdir temp_mount
   sudo mount /dev/loop0p1 ./temp_mount
   sudo cp software/u-boot-socfpga/u-boot.img software/u-boot.scr soc_system.rbf temp_mount
   sync
   sudo umount temp_mount

For the last partition, create the EXT4 filesystem. By default, this filesystem gets created with
an option to support files larger than 2TB. To avoid problems with the Angstrom kernel, we have to
disable this option. The complete commands are the following:

.. code-block:: bash

   sudo mkfs.ext4 /dev/loop0p2           # Create filesystem
   tune2fs -O ^huge_file /dev/loop0p2    # Disable huge_file feature

After all the partitions are set up, it is time to burn the image to an SD card. Plug the SD card,
identify its device name with the "lsblk" command, and run the following commands:

.. code-block:: bash

   sudo dd if=sdcard.img of=/dev/XXX bs=2048
   sync

When the SD card is burned, the initial booting stages can be tested. Connect the device to the
computer via the serial port. Open a putty serial console at 115200 bauds, and start the device. First, the
preloader will print status messages informing of initialization before loading U-Boot. The output
from U-Boot will inform about the load of the FPGA bitstream, and complain about the lack of
device tree blob (soc_system.dtb) and kernel image (zImage).


=================================================
Angstrom (Kernel + Device Tree + Root Filesystem)
=================================================

In order to build Angstrom, we need a new utility, "repo". Run the following commands to download
it:

.. code-block:: bash

   mkdir ~/bin
   PATH=~/bin:$PATH
   curl http://commondatastorage.googleapis.com/git-repo-downloads/repo > ~/bin/repo
   chmod a+x ~/bin/repo

With repo set up, download the angstrom manifests in a new folder:

.. code-block:: bash

   mkdir angstrom-manifests
   cd angstrom-manifests
   repo init -u git://github.com/Angstrom-distribution/angstrom-manifest -b angstrom-v2016.12-yocto2.2

Now, update the meta-altera layer to use a newer one. Open the file '.repo/manifest.xml' in a text
editor and change the meta-altera entry to:

.. code-block:: guess

   <project name="kraj/meta-altera" path="layers/meta-altera" remote="github" revision="0c6e036fdfec58b69903a10e886a9dfae8fe4c9f" upstream="master"/>

Next, download all the layers:

.. code-block:: bash

   repo sync

After the layers are downloaded, it is time to setup the environment. First, indicate the board
you want to build the image for, in out case, Cyclone V.

.. code-block:: bash

   MACHINE=cyclone5 . ./setup-environment

After the general configuration is done, custom modifications can be added via the "conf/local.conf"
file. Open this file in a text editor and add the following lines:

.. code-block:: guess

   KERNEL_DEVICETREE = "socfpga_cyclone5_de0_sockit.dtb"
   IMAGE_FSTYPES += "socfpga-sdimg"
   PREFERRED_PROVIDER_virtual/kernel = "linux-altera"
   PREFERRED_VERSION_linux-altera = "4.7%"

Once the configuration is done, Angstrom can be built. First, build the kernel:

.. code-block:: bash

   bitbake virtual/kernel

Next, build the root file system:

.. code-block:: bash

   bitbake console-image

Last, build the SDK for this system:

.. code-block:: bash

   bitbake console-image -c populate_sdk

Once all builds are complete, the built files can be found in the "/deploy/glibc/images/" folder.
First, copy the kernel image and the device tree blob (remember to rename it "soc_system.dtb" as
indicated in the U-Boot script) to the FAT partition. Next, extract the root filesystem in the
EXT4 folder. If everything has worked as intended, the device should boot correctly.

To use the SDK, run the SDK script, which will extract the SDK files in the location you provide.
This SDK contains all the required files required to develop applications and kernel modules to be run in the
device.


============
MAC Spoofing
============

In order to spoof (change with a custom one) the MAC address of the device, create a file named

"00-default.network" in the folder ""/etc/systemd/network", and copy the following content:


.. code-block:: guess

   [Match]
   Name=eth0

   [Link]
   MACAddress=<custom address>

   [Network]
   DHCP=yes


==========================
Custom driver installation
==========================

To load automatically custom kernel modules on startup, put the module inside the folder
"/lib/modules/<kernel-version>". Now, create a file with the name "<module>.conf" in the folder
"/etc/modules-load.d/", and add a line with the name of the module:

.. code-block:: guess

   # Load dumb-module.ko at boot
   dumb-module


=======================================
Run applications as services on startup
=======================================

In order to run an application as a service on startup, create a file named "<application>.service"
in the folder "/etc/systemd/system/" with the following contents:

.. code-block:: guess

   [Unit]
   Description=Service description
   After=network.target

   [Service]
   User=<user>
   Group=<group>
   ExecStart=<path to application>
   Restart=always

   [Install]
   WantedBy=multi-user.target



==========
References
==========

- Embedded Linux Beginners Guide: https://rocketboards.org/foswiki/Documentation/EmbeddedLinuxBeginnerSGuide
- Angstrom On SoCFPGA: https://rocketboards.org/foswiki/Documentation/AngstromOnSoCFPGA_1
- Yocto SDK: https://www.yoctoproject.org/docs/2.2/sdk-manual/sdk-manual.html
- networkd network files: https://wiki.archlinux.org/index.php/Systemd-networkd#network_files
- systemd unit files: https://wiki.archlinux.org/index.php/systemd#Writing_unit_files
