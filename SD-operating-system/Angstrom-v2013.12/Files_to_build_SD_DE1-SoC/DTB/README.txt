Use the files inside one of this folders to build a .dtb that works with Angstrom v2013.12
kernel 3.10lts compiled as in 
http://rocketboards.org/foswiki/view/Documentation/AngstromOnSoCFPGA_1

To build it you will need to have a .sopcinfo file. We used the one in DE1-SoC GHRD previous
Quartus II 14.1 compilation.

To compile use the following:
sopc2dts --input socsystem.sopcinfo --output socfpga.dtb --type dtb --board hps_common_board_info.xml --board soc_system_board_info.xml --bridge-removal all --clocks -v
