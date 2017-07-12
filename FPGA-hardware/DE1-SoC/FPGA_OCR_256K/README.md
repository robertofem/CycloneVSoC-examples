FPGA_OCR_256K
===========

Description
------------
This is a very simple project with an On-Chip RAM (OCR) in the FPGA. It is used to perform transfer tests between processor and FPGA. This project is a modification of the DE1-SoC Golden Hardware Reference Design (GHRD) available in the DE1-SoC CD-ROM documentation. Using Qsys, all the FPGA hardware on the GHRD was removed and an OCR was added and PLL were added.

The OCR configured in the FPGA has the following characteristics:

* Implemented using embedded 10kB memory blocks.
* Size = 256kB, the maximum power of two feasible in DE1-SoC board.
*  Data_size is 128bits. The bigger the data size the faster the data rates achieved so 128 bits is selected. It is the maximum of the bridge where it is connected.
*  Connected to the HPS-FPGA bridge configured with data size equal to 128-bit.
* Address of the memory is the position 0x00000000 in the HPS-FPGA bridge address space (0x00000000 in Qsys). Since the HPS-FPGA bridge starts in 0xC0000000 in the processor address space, the FPGA OCR is also in position 0xC000000 in the processor address space. Therefore 0xC0000000 is the address to be used to access the FPGA OCR from the processor.
 
 The PLL has the following characteristics:
 
 * Input frequency: 50MHz from external oscillator in the DE1-SoC board.
 * Output frequency: 100MHz. This clock is used to source the FPGA OCR and the FPGA side of the HPS-FPGA bridge.
 
The following drawing depicts the hardware just described:

<p align="center">
  <img src="https://github.com/robertofem/CycloneVSoC-examples/raw/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K/FPGA_OCR_256K.png" width="450" align="middle" alt="Cyclone V SoC simplified block diagram" />
</p>


