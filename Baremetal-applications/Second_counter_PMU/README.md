Second_counter_PMU
==================

Description
-----------
This example uses a counter in the Performance Monitoring Unit (PMU) timer to measure seconds and build a second counter. It prints the a number each second through the serial console. After 60 seconds the application finishes.

PMU is a coprocessor located very close to the processor in ARM Cortex-A9. It is in charge of gathering statistics from the processor, i.e. the number of  exceptions, divisions by 0, etc. Each processor core has its own PMU. In case of Cyclone V the processor has two cores and therefore two PMUs. PMU is not  mapped in the address space of the processor. It is accessed through  instructions like the Neon coprocessors.

The PMU has a CPU clock cycle counter that can be used to measure time. To  measure time with PMU is not a good practice because PMU counts cycles from  CPU clock, not time. Therefore if the clock rate of the CPU changes  (i.e. it is reduced to save energy) the time measurement will be wrong.  The advantage of using the PMU is that it measures time very precisely. Therefore if clock rate is stable and we know its rate we can measure time  very precisely with it. 
In the case of the hardware project used  to test this application (https://github.com/robertofem/CycloneVSoC-examples/tree/master/FPGA-hardware/FPGA_OCR_256K) we set  the processors frequency to 800MHz in Qsys and we do not modify it during the program execution so we can safely measure time.

This example can be seen as an example on how to access and control PMU and, because it counts seconds, it can be used to test that the frequency of
the processor defined in Qsys is correct and that we can do time measurements with the settings of the PMU used.

When using Operating System the same code could be used to do a second counter. In that case  https://github.com/robertofem/CycloneVSoC-examples/tree/master/Linux-modules/Enable_PMU_user_space should be inserted first so the application has access to PMU from user space. Be aware that if the module enabling PMU is launched in one processor and the application to count seconds is launched in the other the application will not have access to the PMU because the PMU where the module was running was the only PMU activated.

Contents in the folder
----------------------
* main.c: entry point of the program. Initializes 
* io.c: gives support to printf so characters can be sent over UART.
* pmu.c and pmu.h: functions to control the PMU timer.
* alt_types.h file copied from hwlib.
* cycloneV-dk-ram-modified.ld: describes the memory regions (stack, heap, etc.).
* Makefile: describes compilation process.

Compilation
-----------
Open SoC EDS Command Shell, navigate to the folder of the example and type make.
This programs was tested with Altera SoC EDS v16.1
The compilation process generates two files:
* baremetalapp.bin: to load the baremetal program from u-boot
* baremetalapp.bin.img: to load the baremetal program from preloader
    
How to test
-----------
In the following folder there is an example on how to run baremetal examples available in this repository:
(https://github.com/robertofem/CycloneVSoC-examples/tree/master/SD-baremetal.