Second_Counter_PMU
==================

Description
-----------
This example uses a counter in the Performance Monitoring Unit (PMU) timer to
measure seconds and build a second counter. 

PMU is a coprocessor located very close to the processor in ARM Cortex-A9. It
is in charge of gathering statistics from the processor, i.e. the number of 
exceptions, divisions by 0, etc. Each processor core has its own PMU. In case
of Cyclone V the processor has two cores and therefore two PMUs. PMU is not 
mapped in the address space of the processor. It is accessed through 
instructions like the mathematical cores Neon.
The PMU has a CPU clock cycle counter that can be used to measure time. To 
measure time with PMU is not a good practice because PMU counts cycles from 
CPU clock, not time. Therefore if the clock rate of the CPU changes 
(i.e. it is reduced to save energy) the time measurement will be wrong. 
The advantage of using the PMU is that it measures time very precisely.
Therefore if clock rate is stable and we know its rate we can measure time 
very precisely with PMU. In this case we set in Qsys the processors frequency
to 800MHz in Qsys and not modify it during the program execution so we can 
safetly measure time.

This example can be seen as an example on how to access and control PMU and,
because it counts seconds, it can be used to test that the frequency of
the processor defined in Qsys is correct and that we can do time 
measurements with the settings of the PMU used.

When using Operating System the same codecould be used to do a second counter. In that case 
https://github.com/robertofem/CycloneVSoC_Examples/tree/master/DE1-SoC/Linux_Modules/Enable_PMU_user_space 
should be inserted first so the application has access to PMU from user space. 

Contents in the folder
----------------------
* main.c: entry point of the program.
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
* main.axf: can be used to debug
    
How to test
-----------
In the following folder there is an example on how to run baremetal examples available in this repository:
https://github.com/robertofem/CycloneVSoC_Examples/tree/master/DE1-SoC/Baremetal/Build_baremetal_SD.