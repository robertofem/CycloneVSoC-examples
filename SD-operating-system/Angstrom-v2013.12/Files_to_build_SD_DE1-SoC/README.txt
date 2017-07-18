####Files to build an Angstrom 2013.12 SDCard for DE1-SoC board

* put preloader-mkpimage.bin (using dd) in partition 3. It searches for 
* u-boot in FAT32 partition (partition 1), not in partition 3.
* u-boot.img->u-boot image. Just copy it in FAT32 partition (partition 1).
* u-boot.script (loads fpga files, enables bridges and loads OS)
* u-boot.src->compiled u-boot.script. Copy it in n FAT32 partition (partition 1)
* soc_system.rbf->FPGA configuration file. Obtained transforming .sof 
(obtained after DE1-SOC GHRD compilation using Quartus 14.1). Copy it
in n FAT32 partition (partition 1).
* zImages-> COntain 3.10lts and 3.18 kernel images of Linux (Obtained 
cross-compiling Angstrom 2013.12 in Debian 8 computer. Copy one of the 
in n FAT32 partition (partition 1).
* DTB: 2 .dtb files that properly work with 3.10ltsi kernel. They also 
work with 3.18 kernel but Ethernet is not detected (this is a problem).
Copy a dtb in n FAT32 partition (partition 1).
* Angstrom-console-image-eglibc-ipk-v2013.12-socfpga_cyclone5.rootfs.targz:
it is the file system fo partition 2 (EXT 3 partition). Being root
uncompress this file directly in the partition 3.