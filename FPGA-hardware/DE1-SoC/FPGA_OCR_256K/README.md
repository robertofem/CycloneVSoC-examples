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
  <img src="https://github.com/robertofem/CycloneVSoC-examples/raw/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K/FPGA_OCR_256K.png" width="500" align="middle" alt="Cyclone V SoC simplified block diagram" />
</p>

Compilation instructions
--------------------------

This hardware project was tested on Quartus II and Altera SoC EDS v16.0 Update 2. To compile this project:

* Open Quartus (v16.0 Update 2). **Open project > soc_system.qpf**
* Open Qsys and **load soc_system.qsys**
* On Qsys, Select **Generate > Generate HDL...** De-select “Create block symbol file” option and specify desired HDL language (VHDL our case). Press “Generate” button.
* After generation ends, go to Quartus and press the **Start Analysis & Synthesis** button
* When synthesis ends, go to **Tools > Tcl scripts...** and run the scripts hps_sdram_p0_parameters.tcl and hps_sdram_p0_pin_assignments.tcl. Wait for confirmation pop-up window.
* Perform again the **Analysis & Synthesis** of the project
* Run the **Fitter (Place & Route)** utility
* Run the **Assembler (Generate programming files)** utility

**NOTE:** The last 3 steps could be run altogether pressing the “Start Compilation” button


Generate hardware address map header
-----------------------------------------

To generate the system header file, first open the *SoC EDS Command Shell*. Then, the following instruction can be run from the project root directory, and it will generate a header file describing the HPS address map. It can be used by an HPS C/C++ program to get base addresses and other specifications of the FPGA 
peripherals.
```bash
  $ sopc-create-header-files --single hps_0.h --module hps_0
```
After running it, a header named *hps_0.h* will be generated on the current directory.



