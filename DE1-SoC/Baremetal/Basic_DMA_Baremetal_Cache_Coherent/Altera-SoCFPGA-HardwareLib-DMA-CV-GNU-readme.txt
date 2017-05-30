Introduction

This HardwareLibs project is meant as an example for using the DMA  APIs.

The following DMA APIs are demonstrated:
- Initializing DMA
- Memory to Memory Transfers
- Zero to Memory Transfers

Please refer to the QSPI example project for the following APIs:
- Peripheral to Memory Transfers
- Memory to Peripheral Transfers

Note: This example was tested against SoC EDS 15.0.1b60.

====

Limitations:
- The memory to memory transfer example has an issue with some lengths and sizes. 
  It was therefore temporarily commented out from this release of the example. This is 
  expected to be fixed in the next version of this example.

====

Target Boards:
- Altera Cyclone V SoC Development Board rev D

=====

Source Files

The following are descriptions of the source and header files contained in this
project:

dma_demo.c

  Contains the functions demonstrating DMA APIs.
  
=====

Building Example

Before running the example, the target executable first needs to be built.

1. In DS-5, build the application:
  1a. Switch to the C/C++ Perspective if not already in that perspective by
      selecting the menu: Window >> Open Perspective >> C/C++.
  1b. In the "Project Explorer" panel, right-mouse-click 
      "Altera-SoCFPGA-HardwareLib-DMA-CV-GNU" and select "Build Project".

The Console panel (bottom of the UI) should detail the progress of the build
and report any warnings or errors.

=====

System Setup

1. Connect the USB to serial bridge to the host computer.
2. Connect the USB-BlasterII to the host computer.
3. Install the USB to serial bridge driver on the host computer if that driver
   is not already present. Consult documentation for the DevKit for
   instructions on installing the USB to serial bridge driver.
4. Install the USB-BlasterII driver on the host computer if that driver is not
   already present. Consult documentation for QuartusII for instructions on
   installing the USB-BlasterII driver.
5. In DS-5, configure the launch configuration.
  5a. Select the menu: Run >> Debug Configurations...
  5b. In the options on the left, expand "DS-5 Debugger" and select
      "Altera-SoCFPGA-HardwareLib-DMA-CV-GNU".
  5c. In the "Connections" section near the bottom, click Browse.
  5d. Select the appropriate USB-BlasterII to use. Multiple items will be
      presented if there is more than one USB-BlasterII connection attached to
      the host computer.
  5e. Click "Apply" then "OK" to apply the USB-BlasterII selection.
  5f. Click "Close" to close the Debug Configuration. Otherwise click "Debug"
      run the example in the debugger.

=====

Running the Example

After building the example and setting up the host computer system, the example
can be run by following these steps.
1. Configure the FPGA using the Quartus II Programmer with the following SOF file:  
cv_soc_devkit_i2c_lpbk/output_files/soc_system.sof.

2. In DS-5, launch the debug configuration.
  1a. Switch to the Debug Perspective if not already in that perspective by
      selecting the menu: Window >> Open Perspective >> DS-5 Debug.
  1b. In the "Debug Control" panel, right-mouse-click
      "Altera-SoCFPGA-HardwareLib-DMA-CV-GNU" and select
      "Connect to Target".

Connecting to the target takes a moment to load the preloader, run the
preloader, load the executable, and run executable. After the debug connection
is established, the debugger will pause the execution at the main() function.
Users can then set additional break points, step into, step out of, or step one
line using the DS-5 debugger. Consult documentation for DS-5 for more
information on debugging operations.

=====

Sample output

The address and size of the test buffers are randomly generated for each test. 

The following is a sample output:

INFO: System Initialization.
INFO: Setting up Global Timer.
INFO: Setting up DMA.
INFO: Allocating DMA channel.
INFO: Using random seed = 0x2a9894f.

INFO: Demo DMA memory to memory transfer.
INFO: Copying from 0x02064187 to 0x02172d59 size = 479653 bytes.
INFO: Waiting for DMA transfer to complete.
INFO: Comparing source and destination buffers.
INFO: Demo DMA memory to memory succeeded.

INFO: Demo DMA zero to memory transfer.
INFO: Zeroing buffer at 0x021649f8, size = 426044 bytes.
INFO: Waiting for DMA transfer to complete.
INFO: Testing destination buffer for zero.
INFO: Demo DMA zero to memory succeeded.

INFO: System shutdown.

RESULT: All tests successful.
